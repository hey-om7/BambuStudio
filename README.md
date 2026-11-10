![image](https://user-images.githubusercontent.com/106916061/179006347-497d24c0-9bd6-45b7-8c49-d5cc8ecfe5d7.png)
# BambuStudio
Bambu Studio is a cutting-edge, feature-rich slicing software.  
It contains project-based workflows, systematically optimized slicing algorithms, and an easy-to-use graphic interface, bringing users an incredibly smooth printing experience.

Prebuilt Windows, macOS 64-bit and Linux releases are available through the [github releases page](https://github.com/bambulab/BambuStudio/releases/).

Bambu Studio is based on [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) by Prusa Research, which is from [Slic3r](https://github.com/Slic3r/Slic3r) by Alessandro Ranellucci and the RepRap community.

See the [wiki](https://github.com/bambulab/BambuStudio/wiki) and the [documentation directory](https://github.com/bambulab/BambuStudio/tree/master/doc) for more information.

# What are Bambu Studio's main features?
Key features are:
- Basic slicing features & GCode viewer
- Multiple plates management
- Remote control & monitoring
- Auto-arrange objects
- Auto-orient objects
- Hybrid/Tree/Normal support types, Customized support
- multi-material printing and rich painting tools
- multi-platform (Win/Mac/Linux) support
- Global/Object/Part level slicing parameters

Other major features are:
- Advanced cooling logic controlling fan speed and dynamic print speed
- Auto brim according to mechanical analysis
- Support arc path(G2/G3)
- Support STEP format
- Assembly & explosion view
- Flushing transition-filament into infill/object during filament change

# How to compile
Following platforms are currently supported to compile:
- Windows 64-bit, [Compile Guide](https://github.com/bambulab/BambuStudio/wiki/Windows-Compile-Guide)
- Mac 64-bit, [Compile Guide](https://github.com/bambulab/BambuStudio/wiki/Mac-Compile-Guide)
- Linux, [Compile Guide](https://github.com/bambulab/BambuStudio/wiki/Linux-Compile-Guide)
  - currently we only provide linux appimages on [github releases](https://github.com/bambulab/BambuStudio/releases) for Ubuntu/Fedora, and a [flathub version](https://flathub.org/apps/com.bambulab.BambuStudio) can be used for all the linux platforms

# Report issue
You can add an issue to the [github tracker](https://github.com/bambulab/BambuStudio/issues) if **it isn't already present.**

# License
Bambu Studio is licensed under the GNU Affero General Public License, version 3. Bambu Studio is based on PrusaSlicer by PrusaResearch.

PrusaSlicer is licensed under the GNU Affero General Public License, version 3. PrusaSlicer is owned by Prusa Research. PrusaSlicer is originally based on Slic3r by Alessandro Ranellucci.

Slic3r is licensed under the GNU Affero General Public License, version 3. Slic3r was created by Alessandro Ranellucci with the help of many other contributors.

The GNU Affero General Public License, version 3 ensures that if you use any part of this software in any way (even behind a web server), your software must be released under the same license.

The bambu networking plugin is based on non-free libraries. It is optional to the Bambu Studio and provides extended networking functionalities for users.
By default, after installing Bambu Studio without the networking plugin, you can initiate printing through the SD card after slicing is completed.



I cloned BambuStudio into the workspace and added the ESP32 + DHT22 chamber
  temperature and humidity module on a branch called chamber-sensor. I haven't
  compiled or run it. Building BambuStudio needs its full dependency bundle
  (wxWidgets and others), which isn't on this machine, so do a full build before
  relying on it.
  
  How it works: the ESP32 runs a small web server. BambuStudio sends it a GET
  request every 5 seconds and shows the chamber temperature and humidity on a
  new "Chamber sensor (ESP32)" card. The card sits right under the printer's own
  chamber temperature on the Device page.
  
  You said both "push" and "GET". I went with GET, where BambuStudio asks the
  ESP32 for readings. If you meant the ESP32 should send data to BambuStudio,
  that needs a listener built into the app; tell me and I'll change it.
  
  Keeping upstream updates easy: all the module code is in new files that Bambu
  Lab won't touch:
  - src/slic3r/GUI/ChamberSensor/ChamberSensorPanel.{hpp,cpp}: the card, the GET
  requests and reading the JSON.
  - src/slic3r/GUI/ChamberSensor/ChamberSensor.cmake: adds the new files to the
  build.
  - custom/esp32_dht22_sensor/esp32_dht22_sensor.ino: the ESP32 code.
  - custom/README.md: setup and update instructions.
  
  Only three lines of Bambu Lab's code changed, each tagged with a custom:
  comment:
  - one line in src/slic3r/CMakeLists.txt
  - two lines in src/slic3r/GUI/StatusPanel.cpp
  
  git grep -n "custom: ESP32" finds them. Most Bambu Lab updates will merge
  cleanly. If they rewrite the code right around these lines, you'll get a small
  conflict and just re-add the lines.
  
  To get Bambu Lab's updates:
  
  git fetch upstream && git merge upstream/master
  
  I renamed the remote to upstream and downloaded the full history so merging
  works.
  
  Sensor setup:
  1. Put your Wi-Fi name and password in the .ino file.
  2. Install the Adafruit "DHT sensor library" and "Adafruit Unified Sensor" in
  Arduino.
  3. Connect the DHT22 data pin to GPIO 4.
  4. Flash the board.
  
  The ESP32 then answers GET /sensor with {"temperature":24.5,"humidity":41.2}.
  It returns an error code (503) until the first good reading.
  
  In BambuStudio: click Set URL on the card and enter something like
  http://192.168.1.50/sensor or http://chamber-sensor.local/sensor. The card
  behaves like this:
  - The URL is saved in the app's settings.
  - It only checks the sensor while the Device page is visible.
  - It throws out readings outside the DHT22's range (-40 to 80 °C, 0 to 100 %).
  - It shows "Sensor offline" when the ESP32 doesn't answer, and blanks the
  numbers after 3 failed checks in a row so old readings don't stay on screen.

The repo includes a macOS build script, BuildMac.sh. It produces a .app, and
  you package that into a .dmg yourself afterwards. On your Mac (Apple Silicon,
  macOS 27) you'll need to install two things first.
  
  1. Install tools (one time)
  
  brew install ninja gettext
  
  You already have cmake and git. You only have the Xcode Command Line Tools,
  not the full Xcode app. That's why the commands below use -x, which makes the
  script use Ninja instead of Xcode.
  
  2. Build the dependencies (one time, roughly 30–90 min)
  
  cd /Users/heyom/Documents/Projects/bambustudio-heyom
  ./BuildMac.sh -d -a arm64 -x
  
  This downloads and compiles wxWidgets, Boost, OpenSSL, OpenCV and the other
  libraries. You only redo it if a Bambu Lab update changes deps/.
  
  3. Build the app (about 20–40 min the first time, faster after)
  
  ./BuildMac.sh -s -a arm64 -x
  
  The app ends up at build/arm64/BambuStudio/BambuStudio.app. To test it:
  
  open build/arm64/BambuStudio/BambuStudio.app
  
  When you change only your module later, ./BuildMac.sh -s -a arm64 -x -b
  rebuilds without reconfiguring CMake.
  
  4. Make the .dmg
  
  hdiutil create -volname "BambuStudio" \
    -srcfolder build/arm64/BambuStudio/BambuStudio.app \
    -ov -format UDZO BambuStudio-chamber-sensor.dmg
  
  Notes:
  - Unsigned app: the app isn't signed with an Apple Developer ID. On your own
  Mac, right-click the app and choose Open the first time. If macOS says the app
  is "damaged", run xattr -cr BambuStudio.app. To give it to other people
  without warnings, you'd need a paid Apple Developer account for signing and
  notarization.
  - Intel Macs: use -a universal instead of -a arm64 in both build commands to
  get one app that runs on Intel and Apple Silicon. It takes about twice as
  long.
  - Build errors: a build this size could hit compile errors in my module, since
  I haven't built it yet. Paste the error and I'll fix it.
  
  Want me to run steps 1–3 here in the background and fix anything that breaks?