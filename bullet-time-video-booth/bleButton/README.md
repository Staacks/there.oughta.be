# Simple BLE push button for Pico W

This is the code for the BLE push button used in my bullet time video booth project at https://there.oughta.be/a/bullet-time-video-booth. Initially, I did not release it due to concerns about the license as it is only a slight modification of the [hog_keyboard_demo.c](https://github.com/bluekitchen/btstack/blob/master/example/hog_keyboard_demo.c) which is referenced in the Raspberry Pi Foundation's [pico-examples](https://github.com/raspberrypi/pico-examples) repository, but which is actually part of the Blue Kitchen's examples for their BTStack used on the Pico W - with a more restrictive license.

So, let's talk about that first:

## License

Important! This code is **not** released under the GNU GPL like most of my projects. It retains the original licenses of the original files it is based on. Please check the file headers for details and take particular note of BlueKitchen GmbH's license in blebutton.c as it is more restrictive, for example by prohibiting commercial use.

## Usage

This has been tested to build with the Pico SDK version 2.0.0. So, build a uf2 file from it and put it onto your Pico W.

When powered, it will show up on Bluetooth scans as "OK-Button". To bond with it, you need a PIN which the device generates randomly and dumps to USB serial. So, monitor UART while bonding it for the first time. Once bonded, you can connect and reconnect as you like.

You should connect the button to GPIO5 and GND. If these are bridged, it will send a keypress representing the space bar. That's it. That's all it does.

GPIO5 is defined at the top and can easily be changed. The keycode for the space bar is lazily inserted in line 213 and can be changed there.

As mentioned above, this is just a quick edit of the BLE HID keyboard example.

