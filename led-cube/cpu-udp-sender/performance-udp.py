#!/usr/bin/python3
import psutil
import socket
import time

TARGET_IP="192.168.2.45"
TARGET_PORT=1234

while True:
  temperature = 0.0
  time.sleep(0.5)
  temperature += psutil.sensors_temperatures()["k10temp"][0].current
  time.sleep(0.5)
  temperature += psutil.sensors_temperatures()["k10temp"][0].current
  time.sleep(0.5)
  temperature += psutil.sensors_temperatures()["k10temp"][0].current
  time.sleep(0.5)
  temperature += psutil.sensors_temperatures()["k10temp"][0].current
  time.sleep(0.5)
  temperature += psutil.sensors_temperatures()["k10temp"][0].current
  temperature /= 5.0

  cores = psutil.cpu_percent(percpu=True)

  out = str(temperature) + "," + ",".join(map(str, sorted(cores, reverse=True)))
  socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(out.encode("utf-8"), (TARGET_IP, TARGET_PORT))
