#!/usr/bin/python

import os
import time
import RPi.GPIO as GPIO
import sys


cmd = ""
air_plane_status="off"

def usage_print():
    print("Usage: \n\tairplane_btn_sample.py [rpi_pin]")
    print("Note: \n\tnrc7292EVK : rpi_pin = 26(GP10) \
                  \n\tnrc7394EVK : rpi_pin = 16(GP20)")
    exit()

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
    if int(sys.argv[1]) == 26:
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(int(sys.argv[1]), GPIO.IN, pull_up_down=GPIO.PUD_UP)
        GPIO.add_event_detect(int(sys.argv[1]), GPIO.FALLING, callback=BtnPressedEvent, bouncetime=300)
    elif int(sys.argv[1]) == 16:
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(int(sys.argv[1]), GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
        GPIO.add_event_detect(int(sys.argv[1]), GPIO.RISING, callback=BtnPressedEvent, bouncetime=300)        
    else:
        usage_print()        

if __name__ == '__main__':

    if len(sys.argv) < 2:
        usage_print()
    else:
        # GPIO init
        init()
        # waiting
        while(True):
            time.sleep(1)
            print("[{}]Running...")
