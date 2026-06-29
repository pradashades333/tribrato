Tribrato 1.0.0 - Source Package

This package contains the Tribrato plugin source code and image assets.

Main project files:
- CMakeLists.txt
- Source/
- JUCE/
- tribrato_ui_pngs/
- background.jpeg
- KNOB_NOBG.png
- knob.jpeg
- logo.jpeg
- Installer/
- .github/workflows/build-mac.yml

The images referenced by CMake are:
- background.jpeg
- KNOB_NOBG.png
- tribrato_ui_pngs/tribrato_ui_pngs/knob_highlight.png
- tribrato_ui_pngs/tribrato_ui_pngs/knob_shadow.png
- tribrato_ui_pngs/tribrato_ui_pngs/trigger1_off.png
- tribrato_ui_pngs/tribrato_ui_pngs/trigger1_on.png
- tribrato_ui_pngs/tribrato_ui_pngs/trigger2_off.png
- tribrato_ui_pngs/tribrato_ui_pngs/trigger2_on.png
- tribrato_ui_pngs/tribrato_ui_pngs/trigger3_off.png
- tribrato_ui_pngs/tribrato_ui_pngs/trigger3_on.png
- tribrato_ui_pngs/tribrato_ui_pngs/toggle1_left.png
- tribrato_ui_pngs/tribrato_ui_pngs/toggle1_right.png
- tribrato_ui_pngs/tribrato_ui_pngs/toggle2_left.png
- tribrato_ui_pngs/tribrato_ui_pngs/toggle2_right.png
- tribrato_ui_pngs/tribrato_ui_pngs/toggle3_left.png
- tribrato_ui_pngs/tribrato_ui_pngs/toggle3_right.png

Build notes:
- Requires CMake 3.22 or newer.
- Requires a C++17 compiler.
- On Windows, use Visual Studio 2022 with C++ desktop tools.
- Configure with: cmake -S . -B build
- Build VST3 with: cmake --build build --config Release --target Tribrato_VST3

Generated build outputs and git metadata are not included in this package.
