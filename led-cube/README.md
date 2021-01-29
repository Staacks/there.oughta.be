# there.oughta.be/an/led-cube
This project was presented at https://there.oughta.be/an/led-cube

In this repository, you will find the source materials for my LED cube:

## 3dprint
This folder contains blend and STL files for the 3d printed case. The cube shown on there.oughta.be consists of three sides that hold a panel each (sides.stl), two sides holding the Raspberry Pi 2 (sides_pi_bottom.stl and sides_pi_side.stl) and one plain solid side (sides_filled.stl).

**Also checkout the README in the 3dprint folder for alternative designs.**

## cpu-udp-sender
This is the Python 3 script running on the PC that sends its CPU status to the cube to be visualized.

**Also checkout the README in the cpu-udp-sender folder for alternative solutions.**

## led-cube
The software running on the LED cube. This is a single c++ file that requires the [rpi-rgb-led-matrix library](https://github.com/hzeller/rpi-rgb-led-matrix) by Henner Zeller. For the setup, presented on there.oughta.be, the [installation script](https://github.com/adafruit/Raspberry-Pi-Installer-Scripts/blob/master/rgb-matrix.sh) by Adadfruit was used. However, at the time this project was developed, a newer version was required than what was checked out by the installation script, so in the installation script the line `COMMIT=21410d2b0bac006b4a1661594926af347b3ce334` has been changed to `COMMIT=4f6fd9a5354f44180a16d767a80915b265191c9c`, which of course is also outdated now. So, you might want to check out the latest version.

This also requires the appropriate OpenGL libraries to be present and linked against. On the Raspbian system on which this has been developed, compilation was done via g++ with the command `g++ -g -o cpu-stats-gl cpu-stats-gl.cpp -std=c++11 -lbrcmEGL -lbrcmGLESv2 -I/opt/vc/include -L/opt/vc/lib -Lrpi-rgb-led-matrix/lib -lrgbmatrix -lrt -lm -lpthread -lstdc++ -Irpi-rgb-led-matrix/include/`. (Obviously, the rpi-rgb-led-matrix library was just installed into a subdirectory.)

## Credits

The led-cube code is based on Matus Novak's [example of how to use OpenGL on a Pi without an X server](https://github.com/matusnovak/rpi-opengl-without-x) and follows the examples from the rpi-rgb-led-matrix library to drive the LED panels. The OpenGL shader has been inspired by the shader ["Shiny Circle" by phil on shadertoy.com](https://www.shadertoy.com/view/ltBXRc).

Please refer to https://there.oughta.be/an/led-cube to learn much more about this project.
