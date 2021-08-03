import requests
import cv2
from collections import deque
import threading
import time

camaddress = "http://192.168.122.1:8080/sony/camera/" #Older models like NEX-5T, a6000, ... (Also make sure that your remote control app on the cam itself is up to date)
#camaddress = "http://192.168.122.1:10000/sony/camera" #Newer models like a6400, a7iii, ... (Note the missing trailing slash)
sharpnessAverageCount = 2000 #Number of frames to average to get the "base sharpness"
relativeTriggerIncrease = 1.25 #Camera is triggered if the sharpness rises by this factor above the base sharpness

#Connect to camera
print("Connecting to camera...")
payload = {"version": "1.0", "id": 1, "method": "startRecMode", "params": []}
r = requests.post(camaddress, json=payload)
if r.status_code != 200:
    print("Could not connect to camera: " + str(r.status_code))
    exit()
print("Response: " + str(r.json()))

#Request preview stream
print("Requesting medium res preview stream...")
payload = {"version": "1.0", "id": 1, "method": "startLiveviewWithSize", "params": ["M"]}
r = requests.post(camaddress, json=payload)
response = r.json()
print("Response: " + str(response))
url = response["result"][0]
print("URL: " + str(url))

running = True

#Thread to check whether a squirrel is in focus or not. Uses opencv to analyze the preview stream from the camera
def analyzeStream(squirrelInSight, url):
    print("Opening video stream...")
    cap = cv2.VideoCapture(url)
    print("Ready for the squirrel show.")
    i = 0
    t = time.time()
    focusQueue = deque([])
    while running:
        ret, frame = cap.read()
        if not ret:
            continue
        i += 1
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        laplacian = cv2.Laplacian(gray, cv2.CV_64F)
        mean, std = cv2.meanStdDev(laplacian)
        focus = std[0][0]*std[0][0]
        focusQueue.append(focus)
        if len(focusQueue) > sharpnessAverageCount:
            focusQueue.popleft()
            focusavg = sum(focusQueue)/len(focusQueue)
            if focus/focusavg > relativeTriggerIncrease:
                #This is it, there is something here. Take a pic!
                if not squirrelInSight.isSet():
                    squirrelInSight.set()
                    print("SQUIRREL!")
            else:
                if squirrelInSight.isSet():
                    squirrelInSight.clear()
                    print("Ain't no squirrel no more :(")
            if i % 25 == 0:
                print("---")
                tnow = time.time()
                print("Analyzing at " + str(25/(tnow-t)) + " fps")
                print("Base sharpness: " + str(focusavg))
                print("Current sharpness: " + str(focus))
                t = tnow
        else:
            if i % 25 == 0:
                print("Collecting sample frames: " + str(len(focusQueue)) + "/" + str(sharpnessAverageCount))
    cap.release()

squirrelInSight = threading.Event()
analyzeThread = threading.Thread(target=analyzeStream, args=[squirrelInSight, url])
analyzeThread.start()

try:
    while running:
        squirrelInSight.wait()
        print("Request camera to take a picture.")
        time.sleep(3)
        payload = {"version": "1.0", "id": 1, "method": "actTakePicture", "params": []}
        r = requests.post(camaddress, json=payload)
        response = r.json()
        print("Response: " + str(response))
        url = response["result"][0][0]
        print("Downloading from URL: " + str(url))
        r = requests.get(url)
        open("squirrels/" + time.strftime("%Y%m%d_%H%M%S") + ".jpg", "wb").write(r.content)
        print("Done.")
except KeyboardInterrupt:
    running = False

#Clean up
analyzeThread.join()

