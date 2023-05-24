import gphoto2 as gp
import multiprocessing
import time
import usb.core
import serial
from itertools import repeat
import os

cameraOrder = ["830470610", "1580834974", "630361156", "1480864197", "1781149516", "1280632053", "1781152732", "2081116207", "730370081", "1480834971", "2081116840",  "330141592"]
cameras = [None]*len(cameraOrder)
triggerPort = None

def connectAndMatchCameras():
    global triggerPort, serial
    try:
        triggerPort = serial.Serial('/dev/serial/by-id/usb-Raspberry_Pi_Pico_E66038B7136AA431-if00')
    except serial.SerialException as e:
        print("could not open serial port: {}".format(e))
        return

    port_info_list = gp.PortInfoList()
    port_info_list.load()
    abilities_list = gp.CameraAbilitiesList()
    abilities_list.load()

    canons = []
    devices = usb.core.find(find_all=True)
    for device in devices:
        try:
            if device.product == "Canon Digital Camera":
                canons.append("usb:" + f'{device.bus:03}' + "," + f'{device.address:03}')
        except:
            pass

    for canon in canons:
        camera = gp.Camera()
        idx = port_info_list.lookup_path(canon)
        camera.set_port_info(port_info_list[idx])
        idx = abilities_list.lookup_model("Canon EOS 400D (PTP mode)")
        camera.set_abilities(abilities_list[idx])

        camera.init()
        cfg = camera.get_config()
        serialnumber = cfg.get_child_by_name('serialnumber').get_value()
        try:
            index = cameraOrder.index(serialnumber)
            cameras[index] = camera
            print("Found " + serialnumber + " as " + str(index+1))
        except:
            print("Unmapped camera with serial " + serialnumber + " found.")
            camera.exit()
        continue
    for i, camera in enumerate(cameras):
        if camera == None:
            print("Missing camera " + str(i))

def trigger():
    triggerPort.write(b"t")

def configure(i, shutterspeed, aperture, iso, whitebalance):
    try:
        config = cameras[i].get_config()
        targetCfg = config.get_child_by_name('capturetarget')
        targetCfg.set_value(targetCfg.get_choice(1))
        config.get_child_by_name('iso').set_value(iso)
        config.get_child_by_name('aperture').set_value(aperture)
        config.get_child_by_name('shutterspeed').set_value(shutterspeed)
        config.get_child_by_name('whitebalance').set_value(whitebalance)
        config.get_child_by_name('syncdatetime').set_value(1)
        cameras[i].set_config(config)
        return None
    except:
        return "Error while configuring camera " + str(i+1)

def configureAll(shutterspeed, aperture, iso, whitebalance):
    n = len(cameras)
    with multiprocessing.Pool(4) as pool:
        for result in pool.starmap(configure , zip(range(n), repeat(shutterspeed), repeat(aperture), repeat(iso), repeat(whitebalance))):
            if result != None:
                raise Exception(result)

def retrieve(i, folder, previous):
    try:
        folders = []
        files = []
        for name, value in cameras[i].folder_list_folders("/store_00010001/DCIM/"):
            folders.append(name)
        last = sorted(folders)[-1]
        for name, value in cameras[i].folder_list_files("/store_00010001/DCIM/" + last + "/"):
            files.append(name)
        target = sorted(files)[-1-previous]

        camera_file = cameras[i].file_get("/store_00010001/DCIM/" + last + "/", target, gp.GP_FILE_TYPE_NORMAL)
        camera_file.save(folder + "/" + f"{i:0>4}.jpg")
        #cameras[i].file_delete(folder + "/" + f"{i:0>4}.jpg")
        return None
    except:
        return "Error while reading from camera " + str(i+1)


def retrieveAll(folder, previous):
    n = len(cameras)
    if not os.path.exists(folder):
        os.makedirs(folder)
    with multiprocessing.Pool(4) as pool:
        for result in pool.starmap(retrieve , zip(range(n), repeat(folder), repeat(previous))):
            if result != None:
                raise Exception(result)

def exitAll():
    triggerPort.close()
    for camera in cameras:
        camera.exit()


