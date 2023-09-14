#!/usr/bin/env python
import fire
import nrcnetlink
import sys
import re

def hex2num(c):
	if (c >= '0' and c <= '9'):
		return ord(c) - ord('0')
	if (c >= 'a' and c <= 'f'):
		return ord(c) - ord('a') + 10
	if (c >= 'A' and c <= 'F'):
		return ord(c) - ord('A') + 10
	return -1

def hex2byte(hex_value):
	a = hex2num(hex_value[0])
	if (a < 0):
		return -1
	b = hex2num(hex_value[1])
	if (b < 0):
		return -1
	return (a << 4) | b

def readPacket(packet):
	buffer = ""
	lines = []
	if packet.endswith('.txt'):
		with open(packet, 'r') as pkt:
			lines = pkt.readlines()
	else:
		lines.append(packet)

	for line in lines:
		line = line.strip()
		line = re.sub(r'[^\w]', '', line)
		for i in range(0, len(line), 2):
			buffer += "{}".format(chr(int(line[i:i+2], 16)))

	return buffer

if __name__ == "__main__":
	if len(sys.argv) < 2:
		sys.exit("Usage: ./nrc_frame_injector.py <test_packet [string|.txt]>")

	packet = sys.argv[1]
	buffer = readPacket(packet)
	print("Frame Length: {}".format(len(buffer)))

	nl = nrcnetlink.NrcNetlink()
	nl.nrc_frame_injection(buffer)