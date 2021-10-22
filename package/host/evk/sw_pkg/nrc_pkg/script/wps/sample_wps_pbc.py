import RPi.GPIO as GPIO
import time
import os
import sys
import argparse

parser = argparse.ArgumentParser(description='WPS Pushbutton sample script')
parser.add_argument('--mode', help='sta|ap')
args = parser.parse_args()

cmd = ''

def usage():
    print('select device mode using --mode [sta/ap] command')
    sys.exit()

def BtnPressedEvent(c):
    print("WPS Button Pressed : " + cmd)
    os.system(cmd)

def init():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(26, GPIO.IN)
    GPIO.add_event_detect(26, GPIO.FALLING, callback=BtnPressedEvent, bouncetime=300)

if __name__ == '__main__':
    # Arg parse
    if(args.mode):
        if(args.mode == 'sta'):
            cmd = 'sudo wpa_cli wps_pbc'
        elif(args.mode == 'ap'):
            cmd = 'sudo hostapd_cli wps_pbc'
        else:
            usage()
    else:
        usage()
    
    # GPIO init
    init()
    
    # waiting 
    while(True):
        time.sleep(1)
        print("[{}]Running...".format(args.mode))