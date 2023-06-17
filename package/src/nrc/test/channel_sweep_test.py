#!/usr/bin/python

import os
import time
import commands
import subprocess

#------------------------------#
#------TEST Configuration------#
#------------------------------#
AP_ETH_IP = '192.165.150.2'
AP_IP = '192.161.100.1'
STA_ETH_IP = '192.165.150.4'
STA_IP = '192.161.100.143'
STA_HOST_NAME = 'pi'
STA_HOST_PW = 'raspberry'

#IF True, PING, Iperf will be ignored.
EN_CONNECTION_ONLY_TEST = False

EN_PING_TEST = True
EN_FLOOD_PING_TEST = True
EN_IPERF_TEST = True
EN_IPERF_UDP_TEST = True
EN_IPERF_TCP_TEST = True
#------------------------------#

CHANGE_CHANNEL = 0
CHECK_CONNECT = 1
RUN_IPERF = 2
RUN_PING = 3
RUN_FLOOD_PING = 4

state = CHANGE_CHANNEL
curr_channel_idx = 0
check_count = 0
start_time = 0

#only BW 1MHz
#channel_list = [5180, 5185, 5190, 5195, 5200, 5205, 5210, 5215, 5220, 5225, 5230, 5235, 5240, 5745, 5750, 5755, 5760]
#s1g_channel_list = [908.5, 909.5, 910.5, 911.5, 912.5, 913.5, 914.5, 915.5, 916.5, 917.5, 918.5, 919.5, 920.5, 921.5, 922.5, 923.5, 924.5]

#Full Channel
channel_list = [5180, 5185, 5190, 5195, 5200, 5205, 5210, 5215, 5220, 5225, 5230, 5235, 5240, 5745, 5750, 5755, 5760, 5765, 5770, 5775, 5780, 5785, 5790, 5795, 5800, 5805, 5810, 5815, 5820, 5825]
s1g_channel_list = [908.5, 909.5, 910.5, 911.5, 912.5, 913.5, 914.5, 915.5, 916.5, 917.5, 918.5, 919.5, 920.5, 921.5, 922.5, 923.5, 924.5, 909, 911, 913, 915, 917, 919, 921, 923, 925, 910, 914, 918, 922]

#Customize Channel
#channel_list = [5200, 5215, 5230, 5745, 5770, 5780, 5795, 5815, 5825]
#s1g_channel_list = [912.5, 915.5, 918.5, 921.5, 911, 915, 921, 914, 922]
#channel_list = [5200, 5825]
#s1g_channel_list = [912.5, 922]

result_success_list = []
result_success_time = []
result_flood_ping = []
result_ping = []
result_iperf_udp_ap = []
result_iperf_tcp_ap = []
result_iperf_udp_sta = []
result_iperf_tcp_sta = []


def run_ping():
	print "[run_ping]"
	p = subprocess.Popen(['sudo', 'ping', STA_IP, '-c', '10'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	offset = string.find('rtt')

	if offset > 0:
		ping_result = string[offset:]
		print ping_result
	else:
		print "Ping Failed."
		result_ping.append(0)
		return

	ping_tmp = ping_result.split('/')
	ping_avg = float(ping_tmp[4])

	result_ping.append(ping_avg)

def run_flood_ping():
	print "[run_flood_ping]"
	p = subprocess.Popen(['sudo', 'ping', '-f', STA_IP, '-c', '1000'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	offset = string.find('rtt')

	if offset > 0:
		ping_result = string[offset:]
		print ping_result
	else:
		print "Ping Failed."
		result_flood_ping.append(0)
		return

	ping_tmp = ping_result.split('/')
	ping_avg = float(ping_tmp[4])

	result_flood_ping.append(ping_avg)


def iperf_sta_udp():
	print "[STA -> AP UDP]"
	subcommand = "iperf3 -c {0} -i15 -u -bM -T NRC".format(AP_IP)
	cmd = ["sshpass", "-p{0}".format(STA_HOST_PW), "ssh", "-o", "StrictHostKeyChecking=no", "{0}@{1}".format(STA_HOST_NAME, STA_ETH_IP), subcommand] 
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	result_tmp = string.split("NRC")
	print result_tmp[4]
	result = result_tmp[4].split(" ")
	tput = result[14]
	unit = result[15]
	print "{0} {1}".format(tput, unit)
	res = "{0} {1}".format(tput, unit)
	result_iperf_udp_sta.append(res)

def iperf_sta_tcp():
	print "[STA -> AP TCP]"
	subcommand = "iperf3 -c {0} -i15 -T NRC".format(AP_IP)
	cmd = ["sshpass", "-p{0}".format(STA_HOST_PW), "ssh", "-o", "StrictHostKeyChecking=no", "{0}@{1}".format(STA_HOST_NAME, STA_ETH_IP), subcommand] 
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	result_tmp = string.split("NRC")
	print result_tmp[4]
	result = result_tmp[4].split(" ")
	tput = result[14]
	unit = result[15]
	print "{0} {1}".format(tput, unit)
	res = "{0} {1}".format(tput, unit)
	result_iperf_tcp_sta.append(res)

def iperf_ap_udp():
	print "[AP -> STA UDP]"
	cmd = ["iperf3", "-c" "{0}".format(STA_IP), "-i15", "-u", "-bM", "-T", "NRC"]
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	result_tmp = string.split("NRC")
	print result_tmp[4]
	result = result_tmp[4].split(" ")
	tput = result[14]
	unit = result[15]
	print "{0} {1}".format(tput, unit)
	res = "{0} {1}".format(tput, unit)
	result_iperf_udp_ap.append(res)

def iperf_ap_tcp():
	print "[AP -> STA TCP]"
	cmd = ["iperf3", "-c" "{0}".format(STA_IP), "-i15", "-T", "NRC"]
	p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	data = p.communicate()
	string = data[0]
	result_tmp = string.split("NRC")
	print result_tmp[4]
	result = result_tmp[4].split(" ")
	tput = result[14]
	unit = result[15]
	print "{0} {1}".format(tput, unit)
	res = "{0} {1}".format(tput, unit)
	result_iperf_tcp_ap.append(res)

def run_iperf():
	print "[run_iperf]"
	#------------------
	#    STA TO AP
	#------------------
	os.system("sudo killall -9 iperf3")
	time.sleep(1)
	os.system("iperf3 -s -i15 &")
	time.sleep(1)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'iperf3 -c {3} -i15 -u -bM -T NRC'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP, AP_IP))
	if EN_IPERF_UDP_TEST:
		iperf_sta_udp()
	if EN_IPERF_TCP_TEST:
		iperf_sta_tcp()
	os.system("sudo killall -9 iperf3")
	time.sleep(1)

	#------------------
	#    AP TO STA
	#------------------
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'sudo killall -9 iperf3'".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(1)
	os.system("sshpass -p{0} ssh -o StrictHostKeyChecking=no {1}@{2} 'iperf3 -s -i15 &' &".format(STA_HOST_PW, STA_HOST_NAME, STA_ETH_IP))
	time.sleep(1)
	if EN_IPERF_UDP_TEST:
		iperf_ap_udp()
	if EN_IPERF_TCP_TEST:
		iperf_ap_tcp()

def change_channel():
	global curr_channel_idx
	global start_time
	chan = channel_list[curr_channel_idx]
	ret = commands.getoutput("sudo hostapd_cli -iwlan0 all_sta | grep \"\\:\"")
	#print ret
	if ret != "":
		os.system("sudo hostapd_cli -iwlan0 disassociate {0}".format(ret))
	os.system("sudo hostapd_cli -iwlan0 chan 1 {0}".format(chan))
	start_time = int(round(time.time()*1000))

	curr_channel_idx += 1
	return True

def check_connection():
	ret = commands.getoutput("sudo hostapd_cli -iwlan0 all_sta | grep ASSOC")
	if ret != '':
		#print "Connected"
		return True
	else:
		#print "Not Connected"
		return False

print "NRC AP Channel Sweep Test ..."

while True:
	if state == CHANGE_CHANNEL:
		if curr_channel_idx >= len(channel_list):
			break
		print "Change Channel {0}".format(channel_list[curr_channel_idx])
		change_channel()
		#time.sleep(5)
		state = CHECK_CONNECT
	elif state == CHECK_CONNECT:
		res = check_connection()
		elap_time = float(int(round(time.time()*1000)) - start_time)
		if res == True:
			result_success_list.append(1)
			result_success_time.append(elap_time/1000)
			if EN_CONNECTION_ONLY_TEST:
				state = CHANGE_CHANNEL
			else:
				state = RUN_IPERF
			time.sleep(1)
			check_count = 0
		else:
			if elap_time > 30000:
				result_success_list.append(0)
				result_success_time.append(elap_time/1000)
				result_ping.append(0)
				result_flood_ping.append(0)
				result_iperf_udp_ap.append(0)
				result_iperf_tcp_ap.append(0)
				result_iperf_udp_sta.append(0)
				result_iperf_tcp_sta.append(0)
				state = CHANGE_CHANNEL
	elif state == RUN_IPERF:
		if EN_IPERF_TEST:
			run_iperf()
		else:
			result_iperf_udp_ap.append(0)
			result_iperf_tcp_ap.append(0)
			result_iperf_udp_sta.append(0)
			result_iperf_tcp_sta.append(0)
		state = RUN_PING
	elif state == RUN_PING:
		if EN_PING_TEST:
			run_ping()
		else:
			result_ping.append(0)
			result_flood_ping.append(0)
		state = RUN_FLOOD_PING
	elif state == RUN_FLOOD_PING:
		if EN_FLOOD_PING_TEST:
			run_flood_ping()
		state = CHANGE_CHANNEL
	else:
		time.sleep(1)

print "[Test Result]"
idx = 0
if EN_CONNECTION_ONLY_TEST:
	print "11n Freq / S1G Freq / Pass or Fail (Elapsed time) "
	for i in result_success_list:
		if i == 1:
			pass_elap = "PASS ({0} secs)".format(result_success_time[idx])
			print "{0:^9} {1:^10} {2:^29}".format(channel_list[idx], s1g_channel_list[idx], pass_elap) 
		else:
			fail_elap = "FAIL ({0} secs)".format(result_sucess_time[idx])
			print "{0:^9} {1:^10} {2:^29}".format(channel_list[idx], s1g_channel_list[idx], fail_elap)
		idx += 1
else:
	print "11n Freq / S1G Freq / Pass or Fail (Elapsed time) /   Ping   / Flood Ping /   STA UDP TX   /   STA TCP TX   /    AP UDP TX   /    AP TCP TX   /"
	for i in result_success_list:
		if i == 1:
			pass_elap = "PASS ({0} secs)".format(result_success_time[idx])
			ping_res = "{0} ms".format(result_ping[idx])
			f_ping_res = "{0} ms".format(result_flood_ping[idx])
			print "{0:^9} {1:^10} {2:^29} {3:^10} {4:^12} {5:^16} {6:^16} {7:^16} {8:^16}".format(channel_list[idx], s1g_channel_list[idx], 
					pass_elap, ping_res, f_ping_res, result_iperf_udp_sta[idx], result_iperf_tcp_sta[idx], 
					result_iperf_udp_ap[idx], result_iperf_tcp_ap[idx])
		else:
			fail_elap = "FAIL ({0} secs)".format(result_sucess_time[idx])
			print "{0:^9} {1:^10} {2:^29}".format(channel_list[idx], s1g_channel_list[idx], fail_elap)
		idx += 1



