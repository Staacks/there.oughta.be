# there.oughta.be/an/irrigation-system
This project was presented at https://there.oughta.be/an/irrigation-system

This is the controller for an irrigation system. The code as it is provided can be compiled in the Arduino IDE (with the non-libraries installed, see below) and is supposed to run in the setup described below. You can change some basic things like Wifi credentials, the PIN out and the irrigation program at the top of the Arduino code but changing hardware requires changing the code.

## Standard setup

The code is designed to run on an Arduino Nano 33 IoT with the following additions:
- An OLED-Display attached to the I2C bus.
- Two buttons connected across ground and an input each. Here, we use internal pull-ups and interrupts. Some bounce rejection has been implemented, but since we use interrupts to react to any change, you want to properly debounce the buttons with a capacitor to avoid unintended button events.
- Valves are controlled through a range of digital outputs. Typically, these control relay switches witch in turn switch the voltage supply for the valves.

## Features

- You can set a specific duration to open the main valve without starting an irrigation sequence.
- You can set a specific duration for automated irrigation. The system will open the main valve during this time and split the duration in several sequences of few hours. During each sequence, the valves will be opened for a specified ratio of duration. This way an even irrigation across sprinklers with different output can be achieved.
- Remaining time and currently active sprinkler shown on display.
- Can be controlled via buttons
- Can be controlled via MQTT (also allows for activating specific sprinklers)
- Reports state, remaining time, current sprinkler etc. via MQTT

## Required libraries

The code requires the following non-standard Arduino libraries. However, since WifiNINA and the Adafruit libraries are specifically picked for the Arduino Nano 33 IoT and the OLED display, you might want to substitute these if you opt for a different Microcontroller and/or chose a different display or no display at all.

- ArduinoMqttClient (MQTT Client)
- WifiNINA (Wifi support for the Arduino Nano 33 IoT)
- Adafruit SSD1306 (for the OLED display)
- Adafruit GFX (Grafics library for the display)

## More info

For more information see the blog post at https://there.oughta.be/an/irrigation-system.
