#!/usr/bin/python

import os
import time
import commands
import subprocess

#------------------------------#
#------TEST Configuration------#
#------------------------------#
AP_ETH_IP = '192.165.150.3'
AP_IP = '192.161.100.1'
AP_IP_SUB = '192.161.100'
STA_ETH_IP = '192.165.150.4'
STA_IP = '192.161.100.142'
STA_HOST_NAME = 'pi'
STA_HOST_PW = 'raspberry'
TEST_COUNT = 10
#------------------------------#

STATE_IDLE = 0
STATE_RUN_AP = 1
STATE_RUN_STA = 2
STATE_CHECK_CONNECTION = 3
STATE_RUN_PING = 4
STATE_RMMOD = 5
STATE_FAIL = 6

curr_test_cnt = 0
start_time = 0
state = STATE_IDLE

def run_ap():
	print "[run ap]"
	os.system("sudo killall -9 hostapd")
	os.system("sudo rmmod nrc")
	os.system("sudo insmod /home/pi/nrc/nrc.ko hifspeed=16000000 power_save=1 fw_name=nrc7292_cspi.bin")
	time.sleep(5)
	os.system('python /home/pi/nrc/test/netlink/shell.py run --cmd="phy rxgain 50"')
	time.sleep(1)
	os.system('python /home/pi/nrc/test/netlink/shell.py run --cmd="phy txgain 1"')
	time.sleep(1)
	os.system('python /home/pi/nrc/test/netlink/shell.py run --cmd="set capa_1m off on"')
	time.sleep(1)
	os.system("sudo hostapd /home/pi/nrc/test/ap_halow_ins_rm_test.conf &")
	time.sleep(3)

def run_sta():
	global start_time
	global curr_test_cnt

	print "[run sta]" 

	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo killall -9 wpa_supplicant'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo rmmod nrc'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(1)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo insmod /home/pi/nrc/nrc.ko power_save=0 hifspeed=16000000 fw_name=nrc7292_cspi.bin'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(5)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'python /home/pi/nrc/test/netlink/shell.py run --cmd=\"phy rxgain 50\"'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(1)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'python /home/pi/nrc/test/netlink/shell.py run --cmd=\"phy txgain 1\"'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(1)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'python /home/pi/nrc/test/netlink/shell.py run --cmd=\"set capa_1m off on\"'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(1)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo wpa_supplicant -Dnl80211 -iwlan0 -c /home/pi/nrc/test/sta_halow_ins_rm_test.conf &' &".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	start_time = int(round(time.time()*1000))
	curr_test_cnt += 1
	
def check_connection():
	subcommand = "sudo wpa_cli -iwlan0 status | grep ip_address={0}".format(AP_IP_SUB)
	cmd = ["sshpass", "-p{0}".format(STA_HOST_PW), "ssh", "-o", "StrictHostKeyChecking=no", "{0}@{1}".format(STA_HOST_NAME, STA_ETH_IP), subcommand] 
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	if string != '':
		return True
	else:
		return False

def run_ping():
	print "[run_ping]"
	p = subprocess.Popen(['sudo', 'ping', STA_IP, '-c', '10'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	offset = string.find('rtt')

	if offset > 0:
		ping_result = string[offset:]
		print ping_result
		return True
	else:
		print "Ping Failed."
		return False

def run_rmmod():
	print "[run rmmod]"
	os.system("sudo killall -9 hostapd")
	os.system("sudo rmmod nrc")
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo killall -9 wpa_supplicant'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo rmmod nrc'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))

print "NRC AP/STA insmod and rmmod Test ..."

while True:
	if state == STATE_IDLE:
		if curr_test_cnt >= TEST_COUNT:
			print "Test Done. Test count:{0}".format(curr_test_cnt)
			break
		print "Test Count {0}".format(curr_test_cnt)
		run_ap()
		state = STATE_RUN_STA
	elif state == STATE_RUN_STA:
		run_sta()
		state = STATE_CHECK_CONNECTION
	elif state == STATE_CHECK_CONNECTION:
		res = check_connection()
		elap_time = float(int(round(time.time()*1000)) - start_time)
		if res == True:
			state = STATE_RUN_PING
			time.sleep(1)
		else:
			if elap_time > 30000:
				state = STATE_FAIL
	elif state == STATE_RUN_PING:
		res = run_ping()
		if res == True:
			state = STATE_RMMOD
		else:
			state = STATE_FAIL
	elif state == STATE_RMMOD:
		run_rmmod()
		state = STATE_IDLE
	elif state == STATE_FAIL:
		print "Failed Ping Test. Test Count:{0}".format(curr_test_cnt)
		break
	else:
		time.sleep(1)

