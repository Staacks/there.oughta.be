# there.oughta.be/a/bullet-time-video-booth

This project was presented at https://there.oughta.be/a/bullet-time-video-booth

In this project I created a variant of a photo booth for my cousin's wedding. But instead of taking photos of guests, it records short video clips and uses an array of twelve DSLRs create a bullet time effect (yes, that Matrix effect) to transition between recordings by different guests.

This folder contains the code for different aspects of this project, but since this project and the code are very specific to the exact hardware (camera brands, HDMI grabber etc., see https://there.oughta.be/a/bullet-time-video-booth) I used, it is unlikely that you will be able to use the code as is. You will probably have to make several adjustments for your needs.

## bleButton

The big push buttons that control the bullet time video booth. They are based on a Raspberry Pi Pico W and act as a BLE HID keyboard. Unfortunately, this folder currently only contains a placeholder readme explaining the licencing issue that prevents me from sharing the code.

## bullettimeBooth

The main code for the bullet time booth. This is a Python script that runs a flask server, controls all the cameras and generates previews using ffmpeg. Once running you can show the interface to the users by connecting with a webbrowser.

## pico-controler

A very simple code for a Raspberry Pi Pico. It simply allows pulling down GPIO pins to ground to trigger the focus and shutter of a Canon remote trigger connector. See https://there.oughta.be/a/bullet-time-video-booth for details.

