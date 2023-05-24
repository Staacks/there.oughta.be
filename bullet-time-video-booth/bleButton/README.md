# Code not available due to licence limitations

Unfortunately, I am not allowed to share the code for the Bluetooth buttons of the video booth at the moment. The reason is that the Bluetooth support of the Raspberry Pi Pico W is still very new and my code is based on [hid_keyboard_demo.c](https://github.com/bluekitchen/btstack/blob/master/example/hid_keyboard_demo.c) of the Blue Kitchen Bluetooth stack, which has not (yet?) been properly migrated to the Raspberry Pi Foundation's [pico-examples](https://github.com/raspberrypi/pico-examples) repository. While [some rumors](https://github.com/raspberrypi/pico-sdk/issues/1164#issuecomment-1372677903) speak of an upcoming maker-friendly licence, the demo is currently only referenced by the pico-examples repository with the original Blue Kitchen licence, which explicitly prohibits any redistribution for commercial purposes. Since I receive ad revenues for my projects (i.e. Youtube) I cannot claim that there is no commercial purpose and hence I am not allowed to share my code at this point.

If the code is eventually released under a more permissive licence, please let me know and I will happily share the changes I made.

Until then, it is not too tricky to adapt [hid_keyboard_demo.c](https://github.com/bluekitchen/btstack/blob/master/example/hid_keyboard_demo.c). Connect the push button such that a GPIO pin is pulled to ground, enable the internal pullup of that GPIO pin and modify the demo such that a keypress is sent when the GPIO pin goes low.

