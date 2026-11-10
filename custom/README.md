# Custom: ESP32 / DHT22 chamber sensor

Adds a "Chamber sensor (ESP32)" card under the chamber temperature on the
Device page. BambuStudio polls the ESP32 with HTTP GET (default every 5 s) and
shows temperature (°C) and humidity (%).

## Files

All module code lives in new files that upstream doesn't touch:

- `src/slic3r/GUI/ChamberSensor/ChamberSensorPanel.{hpp,cpp}`: the UI panel and HTTP polling
- `src/slic3r/GUI/ChamberSensor/ChamberSensor.cmake`: adds the sources to the build
- `custom/esp32_dht22_sensor/esp32_dht22_sensor.ino`: ESP32 firmware

Upstream files are touched in three lines only, each tagged `// custom:` or `# custom:`:

- `src/slic3r/CMakeLists.txt`: `include(.../ChamberSensor.cmake)`
- `src/slic3r/GUI/StatusPanel.cpp`: one `#include` and one `sizer->Add(new ChamberSensor::ChamberSensorPanel(parent) ...)`

To find them: `git grep -n "custom: ESP32"`

## Sensor contract

`GET <url>` must return HTTP 200 with JSON:

```json
{"temperature": 24.5, "humidity": 41.2}
```

`temp` and `hum` are also accepted. Values outside the DHT22 range
(-40 to 80 °C, 0 to 100 %) are rejected.

## Configuration

Click **Set URL** on the card, e.g. `http://192.168.1.50/sensor` or
`http://chamber-sensor.local/sensor`. The URL is saved in BambuStudio's app config as
`chamber_sensor_url`. You can set `chamber_sensor_interval_s` there too (minimum 2).

## Pulling upstream updates

```sh
git fetch heyom-sensors          # remote "heyom-sensors" = bambulab/BambuStudio
git merge heyom-sensors/master   # or: git rebase heyom-sensors/master
```

If upstream changes the lines around the hooks, re-add the three tagged lines.
