#!/usr/bin/env python
import fire
import nrcnetlink
import sys


EID_RSN				= 48
EID_EXT_ELEMENT		= 255
EXT_EID_OWE_DH		= 32
FRAME_AUTH			= 65535
AUTH_GROUP_ID_LEN	= 2
# AUTH_SCALAR_LEN		= 32
# AUTH_FF_ELEMENT_LEN	= 160
MAC_ADDR_LEN		= 6
# AUTH_SCALAR_LEN_DICT indicates input length for scalar field by group ID
AUTH_SCALAR_LEN_DICT = {
	"1": 96,
	"2": 128,
	"3": 20,
	"4": 36,
	"5": 192,
	"6": 21,
	"7": 21,
	"8": 21,
	"9": 21,
	"10": 21,
	"11": 21,
	"12": 21,
	"13": 21,
	"14": 256,
	"19": 32,
	"22": 128,
	"23": 28,
	"24": 32,
	"25": 24,
	"26": 28,
	"27": 42,
	"28": 32,
	"29": 48,
	"30": 64,
}
# AUTH_FF_ELEMENT_LEN_DICT indicates input length for FF element field by group ID
AUTH_FF_ELEMENT_LEN_DICT = {
	"1": 96,
	"2": 128,
	"3": 40,
	"4": 36,
	"5": 192,
	"6": 42,
	"7": 42,
	"8": 42,
	"9": 42,
	"10": 42,
	"11": 42,
	"12": 42,
	"13": 42,
	"14": 256,
	"19": 64,
	"22": 128,
	"23": 256,
	"24": 256,
	"25": 48,
	"26": 56,
	"27": 42,
	"28": 59,
	"29": 48,
	"30": 51,
}
def usage():
	print("Usage:")
	print("./nrc_set_ie.py [set|set_field|stop] <eid|\"auth\"> <data>")
	print("	- Use hexagonal digits for group ID (e.g. '0e00....')")
	sys.exit(1)

def readData(str_data):
	data = ""
	for i in range(0, len(str_data), 2):
		data += "{}".format(chr(int(str_data[i:i+2], 16)))
	return data

def get_field(eid, length, scalar_len=0, ffe_len=0):
	result = []
	if eid == FRAME_AUTH:
		if length == AUTH_GROUP_ID_LEN + scalar_len + ffe_len:
			result.append(["SAE AUTH", AUTH_GROUP_ID_LEN + scalar_len + ffe_len])
			# result.append(["Scalar", AUTH_SCALAR_LEN])
			# result.append(["FF_Element", AUTH_FF_ELEMENT_LEN])
		if length == MAC_ADDR_LEN:
			result.append(["STA AUTH", MAC_ADDR_LEN])

	elif eid == EID_EXT_ELEMENT + EXT_EID_OWE_DH:
		result.append(["Group Transform ID", EXT_EID_OWE_DH])

	return result

def get_eid(eid_str):
	if (eid_str == "auth"):
		eid = FRAME_AUTH
		print_eid = "Frame: AUTH"
	else:
		eid = int(eid_str.replace("ext", ''))
		print_eid = "EID: {}".format(eid)
		if "ext" in eid_str:
			eid += EID_EXT_ELEMENT
			print_eid = "Ext_" + print_eid

	return eid, print_eid

if __name__ == "__main__":
	if not 3 <= len(sys.argv) <= 4:
		usage()

	nl = nrcnetlink.NrcNetlink()

	op = sys.argv[1]
	eid, print_eid = get_eid(sys.argv[2])
	str_data = sys.argv[3].replace(':', '') if len(sys.argv) == 4 else ""

	if (op == "stop"):
		nl.nrc_set_ie(eid, 0, str_data)
		print("[Stop] {}".format(print_eid))
	elif (op == "set" and str_data):
		data = readData(str_data[4:])
		nl.nrc_set_ie(eid, len(data), data)
		print("[Set] {}, Info_Length: {}, Info: {}".format(print_eid, len(data), str_data))
	elif (op == "set_field" and str_data):
		group_id = 0
		AUTH_SCALAR_LEN = 0
		AUTH_FF_ELEMENT_LEN = 0
		data = readData(str_data)
		if eid == FRAME_AUTH:
			if len(data) != MAC_ADDR_LEN:
				group_id = str(int(str_data[:2], 16))
				# group_id = readData(group_id)
				print(group_id)
				if group_id not in AUTH_SCALAR_LEN_DICT or group_id not in AUTH_FF_ELEMENT_LEN_DICT:
					print("Invalid group ID (" + group_id + ").")
					print("Select from below known group IDs (but use them in 2 hex digits + 00) :")
					print("1(01), 2(02), 3(03), 4(04), 5(05), 6(06), 7(07), 8(08), 9(09),")
					print("10(0a), 11(0b), 12(0c), 13(0d), 14(0e), 19(13), 22(16), 23(17),")
					print("24(18), 25(19), 26(1a), 27(1b), 28(1c), 29(1d), 30(1e)")
					sys.exit(1)
				AUTH_SCALAR_LEN = AUTH_SCALAR_LEN_DICT[group_id]
				AUTH_FF_ELEMENT_LEN = AUTH_FF_ELEMENT_LEN_DICT[group_id]
		queue = get_field(eid, len(data), AUTH_SCALAR_LEN, AUTH_FF_ELEMENT_LEN)
		if not queue:
			sys.exit("[Set_field] Invalid {} Field! Length: {} (should be: {})".format(print_eid, len(data), AUTH_GROUP_ID_LEN + AUTH_SCALAR_LEN + AUTH_FF_ELEMENT_LEN))
		prev = 0
		for f in queue:
			field_data = data[prev:prev+f[1]]
			if eid == FRAME_AUTH and len(field_data) != MAC_ADDR_LEN:
				nl.nrc_set_sae(eid, len(field_data), field_data)
			else:
				nl.nrc_set_ie(eid, len(field_data), field_data)
			print("[Set_field] {}, Field: {}, Field_Length: {},\nInfo: {}, Group ID (SAE) : {}".format(print_eid, f[0], f[1], str_data[prev*2:(prev+f[1])*2], group_id))
			prev = prev+f[1]
	else:
		usage()
