from modules import *

from flask import Flask, render_template, request, jsonify, send_file
from datetime import datetime
import time
import os
from threading import Thread
import signal
import random
import traceback

iso = "400"
aperture = "5.6"
shutterspeed = "1/125"
whitebalance = "Daylight"

videoDuration = 5

class Status:
    IDLE = 0
    COUNTDOWN = 1
    RECORDING = 2
    PROCESSING = 3
    DECISION = 4
    ERROR = 5

app = Flask(__name__)

status = Status.IDLE
timeRef = 0
lastPreviewFinish = 35
folder = ""
randomMapping = {}

@app.route("/", methods = ["GET"])
def interface():
    return render_template('index.html')

@app.route("/status", methods = ["GET"])
def interfaceStatus():
    response = {}
    response["status"] = status
    response["timeRef"] = timeRef
    response["duration"] = videoDuration
    response["finish"] = lastPreviewFinish
    return jsonify(response)

@app.route("/control", methods = ["GET"])
def interfaceControl():
    if "cmd" in request.args:
        if request.args["cmd"] == "ok":
            ok()
        elif request.args["cmd"] == "abort":
            abort()
    return ""

@app.route("/preview", methods = ["GET"])
def filePreview():
    if os.path.isfile(folder + "/preview.mp4"):
        return send_file(folder + "/preview.mp4", mimetype="video/mp4")
    return ""

@app.route("/random", methods = ["GET"])
def fileRandomVideo():
    available = []
    for dataset in sorted(os.listdir("../data")):
        if os.path.isfile("../data/" + dataset + "/final.mp4"):
            available.append("../data/" + dataset + "/final.mp4")
    if len(available) == 0:
        return ""
    if "i" in request.args:
        i = int(request.args["i"])
        if not i in randomMapping:
            randomMapping[i] = random.randrange(len(available))
        return send_file(available[randomMapping[i]], mimetype="video/mp4")
    return ""

def ok():
    global status
    if status == Status.IDLE:
        Thread(target = execute).start()
    elif status == Status.DECISION:
        os.makedirs(folder + "/final")
        status = Status.IDLE

def abort():
    global status
    status = Status.IDLE

def execute():
    global status, timeRef, folder, lastPreviewFinish
    try:
        timeRef = time.time()+6
        status = Status.COUNTDOWN

        folder = "../data/" + datetime.today().strftime('%Y-%m-%d_%H-%M-%S')
        if not os.path.exists(folder):
            os.makedirs(folder)

        while time.time()-timeRef < -1.6:
            if status == Status.IDLE:
                return
        print("Start video recording")
        videoCam.recordVideo(videoDuration*25, folder + "/recording.mjpeg")

        while time.time()-timeRef < 0:
            if status == Status.IDLE:
                return

        bullettimeArray.trigger()
        status = Status.RECORDING
        print(str(time.time()-timeRef) + " Bullettime trigger in")

        print(str(time.time()-timeRef) + " Waiting for recording")
        while time.time()-timeRef < videoDuration:
            if status == Status.IDLE:
                return

        bullettimeArray.trigger()
        print(str(time.time()-timeRef) + " Bullettime trigger out")

        if status == Status.IDLE:
            return
        status = Status.PROCESSING
        time.sleep(3)

        if status == Status.IDLE:
            return
        print(str(time.time()-timeRef) + " Downloading in...")
        bullettimeArray.retrieveAll(folder + "/in", 1)
        print(str(time.time()-timeRef) + " Downloading out...")
        bullettimeArray.retrieveAll(folder + "/out", 0)

        if status == Status.IDLE:
            return
        print(str(time.time()-timeRef) + " Process results")
        proc = processBullettime.generateTransforms(folder, True)
        while proc.poll() == None:
            if status == Status.IDLE:
                proc.terminate()
                return
            time.sleep(0.1)
        proc = processBullettime.combine(folder, folder + "/preview.mp4", videoDuration*25, True)
        while proc.poll() == None:
            if status == Status.IDLE:
                proc.terminate()
                return
            time.sleep(0.1)

        print(str(time.time()-timeRef) + " Done.")
        lastPreviewFinish = time.time()-timeRef
        if status == Status.IDLE:
            return
        status = Status.DECISION
    except:
        tryRecover()

def waitForProcAndPauseIfActive(proc):
    while proc.poll() == None:
        if not (status == Status.IDLE or status == Status.DECISION):
            print("Pausing final generation.")
            proc.send_signal(signal.SIGTSTP)
            while not (status == Status.IDLE or status == Status.DECISION):
                time.sleep(0.5)
            print("Continue final generation.")
            proc.send_signal(signal.SIGCONT)
        time.sleep(0.1)

def generateFinals():
    while True:
        for dataset in sorted(os.listdir("../data")):
            if os.path.exists("../data/" + dataset + "/final") and not os.path.isfile("../data/" + dataset + "/final.mp4"):
                print("Generating final for " + dataset)
                waitForProcAndPauseIfActive(processBullettime.generateTransforms("../data/" + dataset, False))
                waitForProcAndPauseIfActive(processBullettime.combine("../data/" + dataset, "../data/" + dataset + "/final/final.mp4", videoDuration*25, False))
                os.rename("../data/" + dataset + "/final/final.mp4", "../data/" + dataset + "/final.mp4")
                print("Final for " + dataset + " complete.")
        time.sleep(1)

def tryRecover():
    global status
    #Somewhat hacky, but turns out to be more reliable to reset USB on restart if there was a problem (like, the entire USB bus reconnects when I turn on the flourecent lights in my basement).
    status = Status.ERROR
    happy = False
    while not happy:
        try:
            print("Closing old connections.")
            bullettimeArray.exitAll()
            print("Reconnect USB hub.")
            os.system("echo \"2-1\" > /sys/bus/usb/drivers/usb/unbind")
            time.sleep(1)
            os.system("echo \"2-1\" > /sys/bus/usb/drivers/usb/bind")
            time.sleep(10)

            print("Connecting...")
            bullettimeArray.connectAndMatchCameras()
            print("Configure cameras...")
            bullettimeArray.configureAll(shutterspeed=shutterspeed, aperture=aperture, iso=iso, whitebalance=whitebalance)
            videoCam.setupVideo()
            happy = True
        except:
            traceback.print_exc()
            continue

    print("Ready. Hopefully...")
    status = Status.IDLE

if __name__ == "__main__":
    print("Connecting...")
    bullettimeArray.connectAndMatchCameras()
    print("Configure cameras...")
    bullettimeArray.configureAll(shutterspeed=shutterspeed, aperture=aperture, iso=iso, whitebalance=whitebalance)
    videoCam.setupVideo()

    Thread(target = generateFinals).start()
    print("Ready.")
    from waitress import serve
    while True:
        try:
            serve(app, host="0.0.0.0", port=8080, threads=4)
        except:
            pass
    print("Exiting...")
    bullettimeArray.exitAll()

