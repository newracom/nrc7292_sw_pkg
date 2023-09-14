import subprocess
import re
import sys
import time
import os

'''
[sta_rmmod_test]
This script repeatedly tests the kernel behavior when the STA module is removed in the middle of iperf runnning

[Process]
0. Load/Configure AP
	1. Load STA
	2. Assoc STA to AP
	3. Run Iperf
	4. Remove STA in the middle of iperf
	5. Repeat from (1)
'''
# default setting
TYPE_AP = "ap"
TYPE_STA= "sta"

# test setting
MAX_TRY = 2 # 0 for infinite trials
IPERF_MODE = 't' # 't' for TCP; 'u' for UDP
IP = "192.161.200.1"

# AP setting
AP_NAME = "a2" # nrc-halow-xx (testbed)
AP_SSID = "halow_rmmod_test"
AP_CHAN = 156

# STA setting
STA_NAME= "a3" # nrc-halow-xx (testbed)

# terminal
def terminal(cmd, disp=False):
	try:
		if disp:
			return subprocess.call(cmd, shell=True)
		else:
			return subprocess.check_output(cmd, shell=True)
	except subprocess.CalledProcessError, e:
		return e.output

# sshpass
def sshpass(name, type_, option, disp=False, err=False):
	err_redirect = ' &> /dev/null' if err else ' > /dev/null'
	return terminal("/usr/bin/sshpass -p 'antjslan' ssh pi@nrc-halow-{} python /home/pi/{}-interface/config_{}.py {}{}".format(name, type_, type_, option, err_redirect), disp)

# Reset
def reset(name, type_, retry=10):
	res = terminal("python /home/pi/hub-interface/hub.py --reset="+name, True)
	option = "--run test"
	if res == 0:
		time.sleep(10)
		while (retry > 0):
			res = sshpass(name, type_, option, True, True)
			if res == 0:
				return
			retry -= 1
			time.sleep(3)

	print("Failed to Reset {}".format(name))

# Insert module
def insmod(name, type_, retry=3):
	option = "--run insmod"
	while (retry > 0):
		retry -= 1
		res = sshpass(name, type_, option, disp=True)
		if res == 0:
			return 0

	print("Failed to Insert {} Module".format(type_.upper()))
	return 1

# Remove module
def rmmod(name, type_, retry=3):
	option = "--run rmmod"
	while (retry > 0):
		retry -= 1
		res = sshpass(name, type_, option, disp=True)
		if res == 0:
			return

	print("Failed to Remove {} Module".format(type_.upper()))
	return 1

# Configure AP Setting
def confAP(name, ssid, channel, retry=3):
	option = "--run config --retry {} --option ssid={} channel={}".format(retry, ssid, channel)
	res = sshpass(name, TYPE_AP, option, disp=True)
	if res:
		print("Failed to Configure AP Setting")
		return 1
	return 0

# Network Configuration
def connSTA(name, ssid):
	option = "--run connect --option {}".format(ssid)
	res = sshpass(name, TYPE_STA, option, disp=True)
	if res:
		print("Failed to Configure Network Setting")
		return 1
	return 0

# Check Assoc
def statusSTA(name, retry = 20):
	option = "--retry {} --get status".format(retry)
	res = sshpass(name, TYPE_STA, option, disp=True)
	if res:
		print("Failed to Associate")
		return 1
	return 0

# iperf Server
def iperfServerStart(name_ap):
	option_ap = "--run iperf --mode {}".format(IPERF_MODE)
	res = sshpass(name_ap, TYPE_AP, option_ap, disp=True, err=True)
	if res:
		print("Failed to Start Iperf Server")
		return 1
	return 0

def iperfServerStop(name_ap):
	option_ap = "--stop iperf"
	res = sshpass(name_ap, TYPE_AP, option_ap, disp=True)
	if res:
		print("Failed to Stop Iperf Server")
		return 1
	return 0

# iperf Client
def iperfClientStart(name_sta):
	terminal("/usr/bin/sshpass -p 'antjslan' ssh pi@nrc-halow-{} \"sudo ifconfig wlan0 192.161.200.2\"".format(name_sta))
	res = terminal("/usr/bin/sshpass -p 'antjslan' ssh pi@nrc-halow-{} \"iperf -c {} -t30 &> /dev/null &\"".format(name_sta, IP))
	if res:
		print("Failed to Run Iperf Client")
		return 1
	return 0

def iperfClientKill(name_sta):
	res = terminal("/usr/bin/sshpass -p 'antjslan' ssh pi@nrc-halow-{} \"sudo killall -9 iperf &> /dev/null\"".format(name_sta, IP))
	if res:
		print("Failed to Kill Iperf Client")
		return 1
	return 0

if __name__ == "__main__":
	print("[sta_rmmod_test] This script repeatedly tests the kernel behavior when the STA module is removed in the middle of iperf runnning")
	start_time = time.time()
	init_retry = 0
	print("[Setup]")
	while init_retry < 3:
		ret = insmod(AP_NAME, TYPE_AP)
		ret += confAP(AP_NAME, AP_SSID, AP_CHAN)
		ret += iperfServerStart(AP_NAME)
		if not ret:
			break
		init_retry += 1
		if init_retry == 3:
			sys.exit("Initial Setup Failed")
		print("Retrying...")
		# reset(AP_NAME, TYPE_AP)
	print("AP: {} | STA: {} | SSID: {} | CHANNEL: {} | AP IP: {}".format(AP_NAME, STA_NAME, AP_SSID, AP_CHAN, IP))
	print("Initial Setup Completed\nTotal # of Trials: {}\n{}".format(MAX_TRY,'-'*30))

	print("[Test Result]")
	trial = 0
	fail = 0
	ret = 0
	while 1:
		if (MAX_TRY != 0 and trial == MAX_TRY):
			break
		trial += 1

		iperfClientKill(STA_NAME)
		ret = insmod(STA_NAME, TYPE_STA)
		if ret:
			print("Trial #{0:>6}: FAIL --INSMOD".format(trial))
			break

		connSTA(STA_NAME, AP_SSID)
		ret = statusSTA(STA_NAME)
		if ret:
			print("Trial #{0:>6}: FAIL --ASSOC".format(trial))
			break

		ret = iperfClientStart(STA_NAME)
		if ret:
			print("Trial #{0:>6}: FAIL --IPERF".format(trial))
			break
		time.sleep(10)

		ret = rmmod(STA_NAME, TYPE_STA)
		if ret:
			print("Trial #{0:>6}: FAIL --RMMOD".format(trial))
			break
		else:
			print("Trial #{0:>6}: PASS".format(trial))

	rmmod(AP_NAME, TYPE_AP)
	rmmod(STA_NAME, TYPE_STA)

	if ret:
		sys.exit("{0}\nFailed after {1}/{2} Trials | Elapsed Time: {3:.2f} s".format('-'*30, trial, MAX_TRY, time.time()-start_time))
	else:
		print("{0}\nAll {1} Trials Succeeded | Elapsed Time: {2:.2f} s".format('-'*30, MAX_TRY, time.time()-start_time))
