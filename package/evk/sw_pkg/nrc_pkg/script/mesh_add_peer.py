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
    print("Usage: mesh_add_peer.py [Interface] [Peer MAC address] [batman]\
          \nExample: ./mesh_add_peer.py wlan0 8c:0f:fa:00:2b:3c batman")
    exit()

def execute(cmd):
    if sys.version_info >= (3, 0):
        return subprocess.check_output(cmd.encode(), shell=True).decode()
    else:
        return subprocess.check_output(cmd, shell=True)

def addPeer(interface, mac, routing):
    found = 0
    print("Waiting for Peer: " + mac)
    while found == 0:
        cmd = "sudo wpa_cli -i " + interface + " list_sta 2> /dev/null"
        peer = execute(cmd).strip().split('\n')
        for i in peer:
            if i == mac:
                found = 1
                print(i + " FOUND!")
                break
        time.sleep(1)
    conn = 0
    print("Waiting for adding new STA: " + mac)
    while conn == 0:
        if routing == 'batman':
            cmd = "sudo batctl n 2> /dev/null | cut -d ' ' -f11"
            sta = execute(cmd).strip().split('\n')
            for i in sta:
                if i == mac:
                    conn = 1
                    print(i + " CONNECTED!")
                    break
            if conn == 1:
                break
            else:
                os.system("sudo wpa_cli -i " + interface + " mesh_peer_add " + mac + " > /dev/null 2>&1")
        else:
            cmd = "sudo iw dev " + interface + " mpath get " + mac + " 2> /dev/null | cut -d ' ' -f1"
            sta = execute(cmd).strip()
            if sta == mac:
                conn = 1
                print(i + " CONNECTED!")
                break
            else:
                os.system("sudo wpa_cli -i " + interface + " mesh_peer_add " + mac + " > /dev/null 2>&1")
        time.sleep(1)

def checkPeer(interface, mac, routing):
    print("Checking connection: " + mac)
    conn = 1
    while True:
        if routing == 'batman':
            cmd = "sudo batctl n 2> /dev/null | cut -d ' ' -f11"
            sta = execute(cmd).strip().split('\n')
            for i in sta:
                if i == mac:
                    conn = 1
                    break
                else:
                    conn = 0
            if conn == 0:
                print(mac + " DISCONNECTED!")
                os.system("sudo wpa_cli -i " + interface + " mesh_peer_remove " + mac + " > /dev/null 2>&1")
                addPeer(interface, mac, routing)
        else:
            cmd = "sudo iw dev " + interface + " mpath get " + mac + " 2> /dev/null | cut -d ' ' -f1"
            sta = execute(cmd).strip()
            if sta != mac:
                if conn == 1:
                    conn = 0
                    print(mac + " DISCONNECTED!")
                    os.system("sudo wpa_cli -i " + interface + " mesh_peer_remove " + mac + " > /dev/null 2>&1")
                addPeer(interface, mac)
            else:
                if conn == 0:
                    conn = 1
        time.sleep(1)

if __name__ == '__main__':
    if len(sys.argv) < 3 or not isMacAddress(sys.argv[2]):
        usage_print()
    elif len(sys.argv) == 3:
        addPeer(sys.argv[1], sys.argv[2], 0)
    elif len(sys.argv) == 4:
        addPeer(sys.argv[1], sys.argv[2], sys.argv[3])

print("Done.")
