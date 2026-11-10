// ChamberSensor module (custom, not part of upstream BambuStudio).
// Polls an external ESP32 + DHT22 sensor over HTTP GET and shows
// chamber temperature and humidity on the Device (dashboard) page.
//
// Kept self-contained so upstream updates can be merged with minimal
// conflicts. See ChamberSensor/README.md for the integration points.
#pragma once

#include <memory>
#include <atomic>
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/stattext.h>
#include <wx/button.h>

namespace Slic3r { namespace GUI { namespace ChamberSensor {

struct Reading
{
    double temperature_c = 0.0;
    double humidity_pct  = 0.0;
};

// Parses {"temperature": 24.5, "humidity": 41.2} (also accepts "temp"/"hum").
// Returns false if the body is not valid sensor JSON.
bool parse_reading(const std::string &body, Reading &out, std::string &err);

class ChamberSensorPanel : public wxPanel
{
public:
    explicit ChamberSensorPanel(wxWindow *parent);
    ~ChamberSensorPanel() override;

    static constexpr const char *CONFIG_KEY_URL      = "chamber_sensor_url";
    static constexpr const char *CONFIG_KEY_INTERVAL = "chamber_sensor_interval_s";

private:
    void on_timer(wxTimerEvent &);
    void on_settings(wxCommandEvent &);
    void poll();
    void restart_timer();
    void show_reading(const Reading &r);
    void show_status(const wxString &msg, bool error);

    std::string url() const;
    int         interval_s() const;

    wxStaticText *m_temp_value  = nullptr;
    wxStaticText *m_hum_value   = nullptr;
    wxStaticText *m_status      = nullptr;
    wxButton     *m_settings_btn = nullptr;
    wxTimer       m_timer;

    // Guards async HTTP callbacks against the panel being destroyed.
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
    std::atomic<bool>     m_in_flight{false};
    int                   m_failures = 0;
};

}}} // namespace Slic3r::GUI::ChamberSensor
