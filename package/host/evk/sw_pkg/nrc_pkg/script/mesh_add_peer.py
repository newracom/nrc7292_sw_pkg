#!/usr/bin/python

import sys
import os
import time
import subprocess

def isMacAddress(s):
    if len(s) == 17 and ':' in s:
        return True
    else:
        return False

def usage_print():
    print("Usage: mesh_add_peer.py [Interface] [Peer MAC address] \
          \nExample: ./mesh_add_peer.py wlan0 8c:0f:fa:00:2b:3c")
    exit()

def addPeer(interface, mac):
    found = 0
    print("Waiting for Peer: " + mac)
    while found == 0:
        cmd = "sudo wpa_cli -i " + interface + " list_sta 2> /dev/null"
        peer = subprocess.check_output(cmd, shell=True).strip().split('\n')
        for i in peer:
            if i == mac:
                found = 1
                print(i + " FOUND!")
                break
        time.sleep(1)
    conn = 0
    print("Waiting for adding new STA: " + mac)
    while conn == 0:
        cmd = "sudo iw dev " + interface + " mpath get " + mac + " 2> /dev/null | cut -d ' ' -f1"
        sta = subprocess.check_output(cmd, shell=True).strip()
        if sta == mac:
            conn = 1
            print(i + " CONNECTED!")
            break
        else:
            os.system("sudo wpa_cli -i " + interface + " mesh_peer_add " + mac + " 2> /dev/null")
        time.sleep(1)

if __name__ == '__main__':
    if len(sys.argv) < 3 or not isMacAddress(sys.argv[2]):
        usage_print()
    addPeer(sys.argv[1], sys.argv[2])

print("Done.")
