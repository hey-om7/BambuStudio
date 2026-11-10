#include "ChamberSensorPanel.hpp"

#include <algorithm>
#include <cmath>
#include <wx/datetime.h>
#include <wx/sizer.h>
#include <wx/textdlg.h>
#include <wx/app.h>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <nlohmann/json.hpp>

#include "libslic3r/AppConfig.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/I18N.hpp"
#include "slic3r/GUI/Widgets/Label.hpp"
#include "slic3r/Utils/Http.hpp"

namespace Slic3r { namespace GUI { namespace ChamberSensor {

static constexpr int DEFAULT_INTERVAL_S = 5;
static constexpr int MIN_INTERVAL_S     = 2;
// DHT22 physical range; anything outside is a bad read.
static constexpr double TEMP_MIN = -40.0, TEMP_MAX = 80.0;

bool parse_reading(const std::string &body, Reading &out, std::string &err)
{
    try {
        auto j = nlohmann::json::parse(body);
        auto pick = [&j](const char *a, const char *b) -> const nlohmann::json * {
            if (j.contains(a) && j[a].is_number()) return &j[a];
            if (j.contains(b) && j[b].is_number()) return &j[b];
            return nullptr;
        };
        const auto *t = pick("temperature", "temp");
        const auto *h = pick("humidity", "hum");
        if (!t || !h) { err = "missing temperature/humidity"; return false; }
        double tv = t->get<double>(), hv = h->get<double>();
        if (!std::isfinite(tv) || !std::isfinite(hv) || tv < TEMP_MIN || tv > TEMP_MAX || hv < 0.0 || hv > 100.0) {
            err = "value out of range";
            return false;
        }
        out.temperature_c = tv;
        out.humidity_pct  = hv;
        return true;
    } catch (const std::exception &e) {
        err = e.what();
        return false;
    }
}

ChamberSensorPanel::ChamberSensorPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY), m_timer(this)
{
    SetBackgroundColour(*wxWHITE);

    auto *title = new wxStaticText(this, wxID_ANY, _L("Chamber sensor (ESP32)"));
    title->SetFont(::Label::Head_13);

    m_settings_btn = new wxButton(this, wxID_ANY, _L("Set URL"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_settings_btn->SetToolTip(_L("Set the HTTP address of the ESP32 sensor, e.g. http://192.168.1.50/sensor"));

    auto *title_row = new wxBoxSizer(wxHORIZONTAL);
    title_row->Add(title, 1, wxALIGN_CENTER_VERTICAL);
    title_row->Add(m_settings_btn, 0, wxALIGN_CENTER_VERTICAL);

    auto make_value = [this](const wxString &label, wxStaticText *&value) {
        auto *col = new wxBoxSizer(wxVERTICAL);
        auto *l   = new wxStaticText(this, wxID_ANY, label);
        l->SetFont(::Label::Body_12);
        l->SetForegroundColour(wxColour(0x6B, 0x6B, 0x6B));
        value = new wxStaticText(this, wxID_ANY, "--");
        value->SetFont(::Label::Head_18);
        col->Add(l, 0, wxALIGN_CENTER_HORIZONTAL);
        col->Add(value, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, FromDIP(2));
        return col;
    };

    auto *values_row = new wxBoxSizer(wxHORIZONTAL);
    values_row->Add(make_value(_L("Temperature"), m_temp_value), 1, wxEXPAND);
    values_row->Add(make_value(_L("Humidity"), m_hum_value), 1, wxEXPAND);

    m_status = new wxStaticText(this, wxID_ANY, "");
    m_status->SetFont(::Label::Body_10);

    auto *root = new wxBoxSizer(wxVERTICAL);
    root->Add(title_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(8));
    root->Add(values_row, 0, wxEXPAND | wxALL, FromDIP(8));
    root->Add(m_status, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
    SetSizer(root);

    Bind(wxEVT_TIMER, &ChamberSensorPanel::on_timer, this);
    m_settings_btn->Bind(wxEVT_BUTTON, &ChamberSensorPanel::on_settings, this);

    if (url().empty())
        show_status(_L("Not configured. Click \"Set URL\"."), false);
    else
        poll();
    restart_timer();
}

ChamberSensorPanel::~ChamberSensorPanel()
{
    m_timer.Stop();
    *m_alive = false;
}

std::string ChamberSensorPanel::url() const
{
    auto *cfg = wxGetApp().app_config;
    std::string u = cfg ? cfg->get(CONFIG_KEY_URL) : std::string();
    boost::algorithm::trim(u);
    return u;
}

int ChamberSensorPanel::interval_s() const
{
    auto *cfg = wxGetApp().app_config;
    if (!cfg) return DEFAULT_INTERVAL_S;
    try {
        int v = std::stoi(cfg->get(CONFIG_KEY_INTERVAL));
        return std::max(v, MIN_INTERVAL_S);
    } catch (...) {
        return DEFAULT_INTERVAL_S;
    }
}

void ChamberSensorPanel::restart_timer()
{
    m_timer.Stop();
    if (!url().empty()) m_timer.Start(interval_s() * 1000);
}

void ChamberSensorPanel::on_timer(wxTimerEvent &)
{
    // Skip network traffic while the Device page is not visible.
    if (!IsShownOnScreen()) return;
    poll();
}

void ChamberSensorPanel::on_settings(wxCommandEvent &)
{
    wxTextEntryDialog dlg(this, _L("ESP32 sensor URL (HTTP GET, returns JSON):"), _L("Chamber sensor"),
                          wxString::FromUTF8(url()));
    if (dlg.ShowModal() != wxID_OK) return;

    std::string u = dlg.GetValue().ToStdString();
    boost::algorithm::trim(u);
    if (!u.empty() && u.rfind("http://", 0) != 0 && u.rfind("https://", 0) != 0) u = "http://" + u;

    wxGetApp().app_config->set(CONFIG_KEY_URL, u);
    m_failures = 0;
    if (u.empty()) {
        m_temp_value->SetLabel("--");
        m_hum_value->SetLabel("--");
        show_status(_L("Not configured. Click \"Set URL\"."), false);
    } else {
        poll();
    }
    restart_timer();
}

void ChamberSensorPanel::poll()
{
    const std::string u = url();
    if (u.empty() || m_in_flight.exchange(true)) return;

    std::weak_ptr<bool> alive = m_alive;
    auto deliver = [this, alive](std::function<void()> fn) {
        wxTheApp->CallAfter([this, alive, fn]() {
            auto a = alive.lock();
            if (!a || !*a) return; // panel was destroyed
            m_in_flight = false;
            fn();
        });
    };

    Http::get(u)
        .timeout_connect(3)
        .timeout_max(5)
        .on_complete([this, deliver](std::string body, unsigned status) {
            Reading r;
            std::string err;
            bool ok = status == 200 && parse_reading(body, r, err);
            if (!ok && err.empty()) err = "HTTP " + std::to_string(status);
            deliver([this, ok, r, err]() {
                if (ok) show_reading(r);
                else {
                    BOOST_LOG_TRIVIAL(warning) << "ChamberSensor: bad response: " << err;
                    ++m_failures;
                    show_status(_L("Invalid sensor data") + ": " + wxString::FromUTF8(err), true);
                }
            });
        })
        .on_error([this, deliver](std::string /*body*/, std::string error, unsigned /*status*/) {
            deliver([this, error]() {
                BOOST_LOG_TRIVIAL(warning) << "ChamberSensor: request failed: " << error;
                ++m_failures;
                show_status(_L("Sensor offline") + ": " + wxString::FromUTF8(error), true);
                if (m_failures >= 3) { // stale values are misleading
                    m_temp_value->SetLabel("--");
                    m_hum_value->SetLabel("--");
                }
            });
        })
        .perform();
}

void ChamberSensorPanel::show_reading(const Reading &r)
{
    m_failures = 0;
    m_temp_value->SetLabel(wxString::Format("%.1f \u00B0C", r.temperature_c));
    m_hum_value->SetLabel(wxString::Format("%.1f %%", r.humidity_pct));
    show_status(_L("Updated") + " " + wxDateTime::Now().FormatISOTime(), false);
}

void ChamberSensorPanel::show_status(const wxString &msg, bool error)
{
    m_status->SetForegroundColour(error ? wxColour(0xD0, 0x1B, 0x1B) : wxColour(0x90, 0x90, 0x90));
    m_status->SetLabel(msg);
    Layout();
}

}}} // namespace Slic3r::GUI::ChamberSensor
