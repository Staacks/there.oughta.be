# there.oughta.be/a/wedding-gift
This project was presented at https://there.oughta.be/a/wedding-gift

Please be aware, that a lot of the code here has been created last minute for a specific wedding. Since I do not have the device any more (it was a gift after all), I cannot maintain the code, fix bugs or clean up the mess that was created under time constraints.

Also note, that all occurences of the original URL for the webserver have been replaced with "MYURL". If you want to use this code, you will need to search for these occurences and replace them with your own URL.

Also please note the following problems:
- The code has been written for one occasion and it has seen many last minute revisions with some bad structure.
- The PHP code has little to now security checks. It was designed to be available for only few days to a very small audience, which mostly consisted of people who asked me if they should enter the URL into WhatsApp to send a message to the guestbook. So, do not run this code on a server that is exposed to people who might want to break things.
- The user interface turned out to be tricky for less tech savvy people (i.e. grand parents). I improved quite a few things in a few tests few days before the wedding, but it turns out that there are more people who do not know how to enter a line break than you might expect.
- The user interface was designed for a German wedding. So, everything is in German and you probably need to translate it.

That being said, you will find three things in here:
- 3dprint: The blend file and the exported STL files for the 3d printed parts
- esp32: The code for the ESP32
- webserver: All the files that need to be hosted to control the project and to provide an interface to create messages

Please refer to https://there.oughta.be/a/wedding-gift to learn much more about this project.
