#!/usr/bin/python

import os
import time
import commands
import RPi.GPIO as GPIO
import sys


cmd = ""
air_plane_status="off"

def BtnPressedEvent(c):
    global air_plane_status

    if (air_plane_status == "off"):
        cmd = "sudo ifconfig wlan0 down"
        air_plane_status = "on"
    else:
        cmd = "sudo ifconfig wlan0 up"
        air_plane_status = "off"

    print("AirPlane Button Pressed : " + cmd)
    os.system(cmd)

def init():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(26, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
    GPIO.add_event_detect(26, GPIO.RISING, callback=BtnPressedEvent, bouncetime=300)

if __name__ == '__main__':

    # GPIO init
    init()
    # waiting
    while(True):
        time.sleep(1)
        print("[{}]Running...")
