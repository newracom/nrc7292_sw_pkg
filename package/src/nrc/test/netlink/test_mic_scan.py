import nrcnetlink
import sys
import argparse
from channel import *

PRINT_ALL = True 	#print all
PRINT = PRINT_ALL 	#print only for the selected bandwidth
COLOR_ENABLED = True #colored print

def check_argument():
	# ssid = raw_input("Enter SSID: ")
	# passphrase = raw_input("Enter passphrase (blank for open mode): ")
	region = raw_input("Enter Region Code: ").upper()
	bw = int(raw_input("Enter bandwidth (number only): "))
	print("")

	if not CHANNEL_LIST.get(region):
		sys.exit("Wrong Region Code {}.".format(region))
	if not (bw == BW_1M or bw == BW_2M or bw == BW_4M):
		sys.exit("Wrong Bandwidth BW_{}M.".format(bw))
	# if (len(passphrase) < 8):
	# 	sys.exit("Passphrase cannot be shorter than 8 characters.")

	return region, bw

def create_sequence(bw_1m_freq):
	start = [bw_1m_freq[0]]
	end = []

	for idx in range(1, len(bw_1m_freq)):
		if bw_1m_freq[idx] != (bw_1m_freq[idx - 1] + 10):
			end.append(bw_1m_freq[idx - 1])
			start.append(bw_1m_freq[idx])
		if idx == len(bw_1m_freq) - 1:
			end.append(bw_1m_freq[idx])

	if not len(start) == len(end):
		sys.exit("Error")

	return start, end


def scan_cca(start, end, bw_1m_freq, region, result_cca):
	nl = nrcnetlink.NrcNetlink()
	cca_array = [False for i in range(len(bw_1m_freq))]
	cca_array_idx = 0
	while (start and end):
		start_freq = start.pop()
		end_freq = end.pop()
		cca_bitmap = 0
		cca_bitmap = nl.nrc_mic_scan(start_freq, end_freq)
		count = (end_freq - start_freq) / 10 + 1
		for i in range(count):
			if (cca_bitmap & 1):
				cca_array[cca_array_idx] = True

			cca_bitmap = cca_bitmap >> 1
			cca_array_idx += 1

	for i in range(len(cca_array)):
		if cca_array[i]:
			cca_freq = bw_1m_freq[i]
			cca_bw = BW_1M
			while (cca_freq and cca_bw):
				result_cca[cca_bw][cca_freq] = True
				if (cca_bw <= BW_2M and cca_bw != 0):
					current = CHANNEL_LIST[region][cca_bw][cca_freq]
					cca_freq = current["upper_freq"]
					cca_bw = current["upper_bw"]
				else:
					break

	return cca_array


def select_channel(bw, bw_1m_freq, cca_array):
	chan_dist = [i for i in bw_1m_freq]
	candidate = []

	if bw != BW_1M:
		for idx in range(len(bw_1m_freq)):
			upper_bw = CHANNEL_LIST[region][BW_1M][bw_1m_freq[idx]]["upper_bw"]
			upper_freq = CHANNEL_LIST[region][BW_1M][bw_1m_freq[idx]]["upper_freq"]
			if bw == upper_bw:
				chan_dist[idx] = upper_freq
			elif upper_freq != 0:
				upper_bw = CHANNEL_LIST[region][BW_2M][upper_freq]["upper_bw"]
				upper_freq = CHANNEL_LIST[region][BW_2M][upper_freq]["upper_freq"]
				if bw == upper_bw:
					chan_dist[idx] = upper_freq
			else:
				chan_dist[idx] = 0

	#preprocessing
	start_idx = -1
	curr_chan = -1
	last_idx = 0
	cca_flag = False
	for idx in range(len(chan_dist)):
		if curr_chan != chan_dist[idx]:
			if idx > 0:
				last_idx = idx - 1
				if cca_flag:
					for i in range(start_idx, last_idx +1):
						chan_dist[i] = -1 if chan_dist[i] == -1 else 0
				elif (chan_dist[start_idx] != 0 and chan_dist[last_idx] != 0):
					candidate.append((start_idx, last_idx))

			curr_chan = chan_dist[idx]
			start_idx = idx
			cca_flag = False

		if curr_chan == chan_dist[idx]:
			if cca_array[idx]:
				cca_flag = True
				chan_dist[idx] = -1


		if idx == len(chan_dist) - 1:
			last_idx = idx
			if cca_flag:
				for i in range(start_idx, last_idx + 1):
					chan_dist[i] = -1 if chan_dist[i] == -1 else 0
			else:
				candidate.append((start_idx, last_idx))

	max_score = 0
	max_score_option = 0
	for option in candidate:
		st = option[0]
		ed = option[1]
		lead = 0
		trail = 0
		edge_R = False
		edge_L = False
		head = st
		tail = ed
		for i in range(max(len(chan_dist) - ed, st)):
			if head < 0:
				edge_L = True
			elif chan_dist[head] == -1:
				pass
			else:
				head -= 1
				lead += 1

			if tail > len(chan_dist) - 1:
				edge_R = True
			elif chan_dist[tail] == -1:
				pass
			else:
				tail += 1
				trail += 1

		if edge_L:
			score = trail
		elif edge_R:
			score = lead
		else:
			score = min(lead, trail)

		if max_score < score:
			max_score = score
			max_score_option = st
		elif max_score == score:
			if edge_L or edge_R:
				max_score = score
				max_score_option = st

	return chan_dist[max_score_option]


def print_cca_array(bw, channel, cca_array):
	#cca
	print_line_cca = "{}".format("|   |")
	#bw_1m
	print_line_1m_0 ="{}".format("|   |")
	print_line_1m_1 ="{}".format("| B |")
	print_line_1m_2 ="{}".format("| W |")
	print_line_1m_3 ="{}".format("| 1 |")
	print_line_1m_4 ="{}".format("| M |")
	print_line_1m_5 ="{}".format("|   |")
	#bw_2m
	print_line_2m_0 ="{}".format("|   |")
	print_line_2m_1 ="{}".format("| B |")
	print_line_2m_2 ="{}".format("| W |")
	print_line_2m_3 ="{}".format("| 2 |")
	print_line_2m_4 ="{}".format("| M |")
	print_line_2m_5 ="{}".format("|   |")
	#bw_4m
	print_line_4m_0 ="{}".format("|   |")
	print_line_4m_1 ="{}".format("| B |")
	print_line_4m_2 ="{}".format("| W |")
	print_line_4m_3 ="{}".format("| 4 |")
	print_line_4m_4 ="{}".format("| M |")
	print_line_4m_5 ="{}".format("|   |")
	if COLOR_ENABLED:
		NORMAL = "\033[m"
		ENDC = "\033[5;37;40m"
		WHITE_AND_BOLD = "\033[1;37;40m"
		GREEN_AND_BOLD = "\033[1;32;40m"
		RED_AND_NORMAL = "\033[0;31;40m"
	else:
		NORMAL = ""
		ENDC = ""
		WHITE_AND_BOLD = ""
		GREEN_AND_BOLD = ""
		RED_AND_NORMAL = ""

	for res in cca_array:
		if res:
			print_line_cca += "{} X {}|".format(RED_AND_NORMAL, ENDC)
		else:
			print_line_cca += "{} O {}|".format(GREEN_AND_BOLD, ENDC)

	prev_2m = -1
	prev_4m = -1
	curr_2m = 0
	curr_4m = 0
	cca_2m = False
	cca_4m = False
	for i in sorted(result_cca[BW_1M].keys()):
		freq_str = str(i)
		freq_res = "*" if channel == i else " "
		cca_res = result_cca[BW_1M][i]
		text_format = RED_AND_NORMAL if cca_res else GREEN_AND_BOLD
		print_line_1m_0 += "{} {} {}|".format(text_format, freq_res, ENDC)
		print_line_1m_1 += "{} {} {}|".format(text_format, freq_str[0], ENDC)
		print_line_1m_2 += "{} {} {}|".format(text_format, freq_str[1], ENDC)
		print_line_1m_3 += "{} {} {}|".format(text_format, freq_str[2], ENDC)
		print_line_1m_4 += "{} {} {}|".format(text_format, freq_str[3], ENDC)
		print_line_1m_5 += "{} {} {}|".format(text_format, freq_res, ENDC)

		freq_2m = CHANNEL_LIST[region][BW_1M][i]["upper_freq"]
		freq_4m = CHANNEL_LIST[region][BW_2M][freq_2m]["upper_freq"] if freq_2m != 0 else 0

		if curr_2m == 0 and freq_2m > 0 and freq_2m != curr_2m:
			curr_2m = freq_2m
			cca_2m = False
		if freq_2m != curr_2m or i == sorted(result_cca[BW_1M].keys())[-1]:
			freq_str = str(curr_2m)
			freq_res = "*" if channel == curr_2m else " "
			delimiter = "| " if prev_2m == -2 else " "
			text_format = RED_AND_NORMAL if cca_2m else GREEN_AND_BOLD
			print_line_2m_0 += "{}{}  {}   {}|".format(delimiter, text_format, freq_res, ENDC)
			print_line_2m_1 += "{}{}  {}   {}|".format(delimiter, text_format, freq_str[0], ENDC)
			print_line_2m_2 += "{}{}  {}   {}|".format(delimiter, text_format, freq_str[1], ENDC)
			print_line_2m_3 += "{}{}  {}   {}|".format(delimiter, text_format, freq_str[2], ENDC)
			print_line_2m_4 += "{}{}  {}   {}|".format(delimiter, text_format, freq_str[3], ENDC)
			print_line_2m_5 += "{}{}  {}   {}|".format(delimiter, text_format, freq_res, ENDC)
			prev_2m = freq_2m
			curr_2m = freq_2m
			cca_2m = False
		if cca_res: cca_2m = True


		if curr_4m == 0 and freq_4m > 0 and freq_4m != curr_4m:
			curr_4m = freq_4m
			cca_4m = False
		if freq_4m != curr_4m or i == sorted(result_cca[BW_1M].keys())[-1]:
			freq_str = str(curr_4m)
			freq_res = "*" if channel == curr_4m else " "
			delimiter = "| " if prev_4m == -2 else " "
			text_format = RED_AND_NORMAL if cca_4m else GREEN_AND_BOLD
			print_line_4m_0 += "{}{}      {}       {}|".format(delimiter, text_format, freq_res, ENDC)
			print_line_4m_1 += "{}{}      {}       {}|".format(delimiter, text_format, freq_str[0], ENDC)
			print_line_4m_2 += "{}{}      {}       {}|".format(delimiter, text_format, freq_str[1], ENDC)
			print_line_4m_3 += "{}{}      {}       {}|".format(delimiter, text_format, freq_str[2], ENDC)
			print_line_4m_4 += "{}{}      {}       {}|".format(delimiter, text_format, freq_str[3], ENDC)
			print_line_4m_5 += "{}{}      {}       {}|".format(delimiter, text_format, freq_res, ENDC)
			prev_4m = freq_4m
			curr_4m = freq_4m
			cca_4m = False
		if cca_res: cca_4m = True

		if (curr_2m != freq_2m or freq_2m == 0):
			delimiter = " " if prev_2m == -2 else ""
			print_line_2m_0 += "{}   ".format(delimiter)
			print_line_2m_1 += "{}   ".format(delimiter)
			print_line_2m_2 += "{}   ".format(delimiter)
			print_line_2m_3 += "{}   ".format(delimiter)
			print_line_2m_4 += "{}   ".format(delimiter)
			print_line_2m_5 += "{}   ".format(delimiter)
			prev_2m = -2

		if (curr_4m != freq_4m or freq_4m == 0):
			delimiter = " " if prev_4m == -2 else ""
			print_line_4m_0 += "{}   ".format(delimiter)
			print_line_4m_1 += "{}   ".format(delimiter)
			print_line_4m_2 += "{}   ".format(delimiter)
			print_line_4m_3 += "{}   ".format(delimiter)
			print_line_4m_4 += "{}   ".format(delimiter)
			print_line_4m_5 += "{}   ".format(delimiter)
			prev_4m = -2

	divider = ENDC+"-"*((len(cca_array))*4 + 5)

	#cca
	print(WHITE_AND_BOLD + "\n[SCAN RESULT]")
	print(NORMAL + divider)
	print(NORMAL + print_line_cca)
	print(NORMAL + divider)

	#bw_1m
	if (PRINT_ALL or bw == BW_1M):
		print(NORMAL + print_line_1m_0)
		print(NORMAL + print_line_1m_1)
		print(NORMAL + print_line_1m_2)
		print(NORMAL + print_line_1m_3)
		print(NORMAL + print_line_1m_4)
		print(NORMAL + print_line_1m_5)
		print(NORMAL + divider)

	#bw_2m
	if (PRINT_ALL or bw == BW_2M):
		print(NORMAL + print_line_2m_0)
		print(NORMAL + print_line_2m_1)
		print(NORMAL + print_line_2m_2)
		print(NORMAL + print_line_2m_3)
		print(NORMAL + print_line_2m_4)
		print(NORMAL + print_line_2m_5)
		print(NORMAL + divider)

	#bw_4m
	if (PRINT_ALL or bw == BW_4M):
		print(NORMAL + print_line_4m_0)
		print(NORMAL + print_line_4m_1)
		print(NORMAL + print_line_4m_2)
		print(NORMAL + print_line_4m_3)
		print(NORMAL + print_line_4m_4)
		print(NORMAL + print_line_4m_5)
		print(NORMAL + divider)

	print("[SUMMARY]")
	print("Selected Frequency : BW_{}M {:.1f}MHz".format(bw, channel/10.))

if __name__ == "__main__":
	region, bw = check_argument()

	bw_1m_freq = sorted(CHANNEL_LIST[region][BW_1M].keys())

	bw_1m_cca = {freq: False for freq in CHANNEL_LIST[region][BW_1M].keys()}
	bw_2m_cca = {freq: False for freq in CHANNEL_LIST[region][BW_2M].keys()}
	bw_4m_cca = {freq: False for freq in CHANNEL_LIST[region][BW_4M].keys()}
	result_cca = {BW_1M: bw_1m_cca, BW_2M: bw_2m_cca, BW_4M: bw_4m_cca}

	start, end = create_sequence(bw_1m_freq)
	cca_array = scan_cca(start, end, bw_1m_freq, region, result_cca)
	channel = select_channel(bw, bw_1m_freq, cca_array)

	if PRINT:
		print_cca_array(bw, channel, cca_array)
