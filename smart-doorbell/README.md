# there.oughta.be/a/smart-doorbell

This project was presented at https://there.oughta.be/a/smart-doorbell

This PCB adds smart functions to a Siedle 1+n system. It connects to a local Wifi and exposes the following functions via MQTT:
* Disabling the doorbell of the in-house phone
* Notification via MQTT if the doorbell is ringing (for alternative means of notification when the in-house phone is disabled).
* Trigger the door opener

Note, that this is a very specific design for a very specific system. Please make sure to fully understand it before using any part of it.

More details on https://there.oughta.be/a/smart-doorbell
