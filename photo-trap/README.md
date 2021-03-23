# there.oughta.be/a/photo-trap

This project was presented at https://there.oughta.be/a/photo-trap

This is a simple Python script to be run on a Raspberry Pi (or any computer with Wifi) that is already connected to a Sony Alpha camera's Wifi hotspot. The script will request a preview video stream and uses OpenCV to determine the sharpness of the current image. This is supposed to be used on a scene with a blurry background due to a shallow depth of field. If an animal enters the scene in focus, the overal sharpness will increase an the script will trigger a photo and download the preview version of the photo and store it in the folder "quirrels".

In order to run this script, you just need to save it anywhere on your Pi. You only need to install opencv libraries (`sudo apt install python3-opencv`) and create the "squirrels" folder (`mkdir squirrels`). Then just run the script with `python3 autocapture.py`.

The script will average the sharpness over a few minutes. You will have to wait for about 80 seconds before the script is ready to trigger.

More details on https://there.oughta.be/a/photo-trap.
