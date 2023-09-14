#!/usr/bin/python
import os
import sys
import binascii
import argparse
import struct
import traceback
import subprocess

filename = sys.argv[1]

def check_align(f, curr, type_unit):
	padding = type_unit - (curr % type_unit)
	if (padding != type_unit):
		curr += padding
		f.read(padding);
	curr += type_unit
	return (curr)

def decode_ver_dump(f, curr):
	try:
		m_curr = check_align(f, curr, 1)
		major = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		minor = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		rev = struct.unpack('B', f.read(1))[0]
		print "Newracom Firmware Version : {0}.{1}.{2}".format(major, minor, rev)
		m_curr = check_align(f, m_curr, 1)
		length = struct.unpack('B', f.read(1))[0]
		m_curr = m_curr+length
		string = f.read(length)
		print "{0}".format(string)
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)

def decode_corereg_dump(f, curr):
	try:
		print "------------------"
		print "CORE REGISTER DUMP"
		print "------------------"
		m_curr = check_align(f, curr, 4)
		R0 = struct.unpack('I', f.read(4))[0]
		print "R0  : 0x%X" %R0
		m_curr = check_align(f, m_curr, 4)
		R1 = struct.unpack('I', f.read(4))[0]
		print "R1  : 0x%X" %R1
		m_curr = check_align(f, m_curr, 4)
		R2 = struct.unpack('I', f.read(4))[0]
		print "R2  : 0x%X" %R2
		m_curr = check_align(f, m_curr, 4)
		R3 = struct.unpack('I', f.read(4))[0]
		print "R3  : 0x%X" %R3
		m_curr = check_align(f, m_curr, 4)
		R13 = struct.unpack('I', f.read(4))[0]
		print "R13 : 0x%X" %R13
		m_curr = check_align(f, m_curr, 4)
		LR = struct.unpack('I', f.read(4))[0]
		print "LR  : 0x%X" %LR
		m_curr = check_align(f, m_curr, 4)
		PC = struct.unpack('I', f.read(4))[0]
		print "PC  : 0x%X" %PC
		m_curr = check_align(f, m_curr, 4)
		PSR = struct.unpack('I', f.read(4))[0]
		print "PSR : 0x%X" %PSR
		m_curr = check_align(f, m_curr, 4)
		CFSR = struct.unpack('I', f.read(4))[0]
		print "CFSR: 0x%X" %CFSR
		m_curr = check_align(f, m_curr, 4)
		HFSR = struct.unpack('I', f.read(4))[0]
		print "HFSR: 0x%X" %HFSR
		m_curr = check_align(f, m_curr, 4)
		DFSR = struct.unpack('I', f.read(4))[0]
		print "DFSR: 0x%X" %DFSR
		m_curr = check_align(f, m_curr, 4)
		AFSR = struct.unpack('I', f.read(4))[0]
		print "AFSR: 0x%X" %AFSR
		m_curr = check_align(f, m_curr, 4)
		BFAR = struct.unpack('I', f.read(4))[0]
		print "BFAR: 0x%X" %BFAR
		m_curr = check_align(f, m_curr, 4)
		MMAR = struct.unpack('I', f.read(4))[0]
		print "MMAR: 0x%X" %MMAR
		m_curr = check_align(f, m_curr, 4)
		ICSR = struct.unpack('I', f.read(4))[0]
		print "ICSR: 0x%X" %ICSR
		print "------------------"

		if (ICSR & 0xFF == 0):
			print "Exception: DEFAULT"
		if (ICSR & 0xFF == 1):
			print "Exception: RESET"
		if (ICSR & 0xFF == 2):
			print "Exception: NMI"
		if (ICSR & 0xFF == 3):
			print "Exception: HARDFAULT"
		if (ICSR & 0xFF == 4):
			print "Exception: MEMFAULT"
		if (ICSR & 0xFF == 5):
			print "Exception: BUSFAULT"
		if (ICSR & 0xFF == 6):
			print "Exception: USAGEFAULT"
		if (ICSR & 0xFF == 11):
			print "Exception: SVC"
		if (ICSR & 0xFF == 12):
			print "Exception: DEBUG"
		if (ICSR & 0xFF == 14):
			print "Exception: PENDSV"
		if (ICSR & 0xFF == 15):
			print "Exception: SYSTICK"
		if (ICSR & 0xFF == 16):
			print "Exception: PMU"
		if (ICSR & 0xFF == 17):
			print "Exception: TIMER4"
		if (ICSR & 0xFF == 18):
			print "Exception: RTC"
		if (ICSR & 0xFF == 19):
			print "Exception: WDT"
		if (ICSR & 0xFF == 20):
			print "Exception: TIMER0"
		if (ICSR & 0xFF == 21):
			print "Exception: HMAC0"
		if (ICSR & 0xFF == 22):
			print "Exception: DMACINTTC"
		if (ICSR & 0xFF == 23):
			print "Exception: TIMER5"
		if (ICSR & 0xFF == 24):
			print "Exception: TIMER1"
		if (ICSR & 0xFF == 25):
			print "Exception: HSUARTR0"
		if (ICSR & 0xFF == 26):
			print "Exception: SSP0"
		if (ICSR & 0xFF == 27):
			print "Exception: PWR_FAIL"
		if (ICSR & 0xFF == 28):
			print "Exception: LPO"
		if (ICSR & 0xFF == 29):
			print "Exception: DMACINTERR"
		if (ICSR & 0xFF == 30):
			print "Exception: HIF"
		if (ICSR & 0xFF == 31):
			print "Exception: HSUART3"
		if (ICSR & 0xFF == 32):
			print "Exception: TIMER2"
		if (ICSR & 0xFF == 33):
			print "Exception: HSUART1"
		if (ICSR & 0xFF == 34):
			print "Exception: SSP1"
		if (ICSR & 0xFF == 35):
			print "Exception: I2C"
		if (ICSR & 0xFF == 36):
			print "Exception: HMAC1"
		if (ICSR & 0xFF == 37):
			print "Exception: HMAC2"
		if (ICSR & 0xFF == 38):
			print "Exception: HIF_TXDONE"
		if (ICSR & 0xFF == 39):
			print "Exception: HIF_RXDONE"
		if (ICSR & 0xFF == 40):
			print "Exception: TIMER3"
		if (ICSR & 0xFF == 41):
			print "Exception: HSURAT2"
		if (ICSR & 0xFF == 42):
			print "Exception: MBXTX"
		if (ICSR & 0xFF == 43):
			print "Exception: MBXRX"
		if (ICSR & 0xFF == 44):
			print "Exception: EXT0"
		if (ICSR & 0xFF == 45):
			print "Exception: EXT1"
		if (ICSR & 0xFF == 46):
			print "Exception: TXQUE"
		if (ICSR & 0xFF == 47):
			print "Exception: RXQUE"

		if (CFSR & (1 << 25)):
			print "Divide by zero usage fault!"
			print "Divide by zero taken place and DIV_O_TRP is set.\n" \
				+ "The code causing the fault can be located using stacked PC."
		if (CFSR & (1 << 24)):
			print "Unaligned access usage fault!"
			print "Unaligned access attempted with multiple load/store instruction,\n" \
				+ "exclusive access instruction or when UNALIGN_TRP is set. The\n" \
				+ "code causing the fault can be located using stacked PC."
		if (CFSR & (1 << 19)):
			print "No coprocessor usage fault!"
			print "Attempt to execute a floating point instruction when the CortexM4\n" \
				+ "floating point unit is not available or when the floating point\n" \
				+ "unit has not been enabled.\n" \
				+ "Attempt to execute a coprocessor instruction. The code causing\n" \
				+ "the fault can be located using stacked PC."
		if (CFSR & (1 << 18)):
			print "Invalid PC load usage fault!"
			print "1) Invalid value in EXC_RETURN number during exception return.\n" \
				+ "For example,\n" \
				+ "- Return to thread with EXC_RETURN = 0xFFFFFFF1\n" \
				+ "- Return to handler with EXC_RETURN = 0xFFFFFFF9\n" \
				+ "To investigate the problem, the current LR value provides\n" \
				+ "the value of LR at the failing exception return.\n" \
				+ "2) Invalid exception active status. For example:\n" \
				+ "- Exception return with exception active bit for the current\n" \
				+ "exception already cleared. Possibly caused by use of\n" \
				+ "VECTCLRACTIVE, or clearing of exception active status in\n" \
				+ "SCB->SHCSR.\n" \
				+ "- Exception return to thread with one (or more) exception\n" \
				+ "active bit still active.\n" \
				+ "3) Stack corruption causing the stacked IPSR to be incorrect.\n" \
				+ "For INVPC fault, the Stacked PC shows the point where the\n" \
				+ "faulting exception interrupted the main/pre-empted program.\n" \
				+ "To investigate the cause of the problem, it is best to use\n" \
				+ "exception trace feature in ITM.\n" \
				+ "4) ICI/IT bit invalid for current instruction. This can happen when\n" \
				+ "a multiple-load/store instruction gets interrupted and, during\n" \
				+ "the interrupt handler, the stacked PC is modified. When the\n" \
				+ "interrupt return takes place, the non-zero ICI bit is applied to\n" \
				+ "an instruction that do not use ICI bits. The same problem can\n" \
				+ "also happen due to corruption of stacked PSR."
		if (CFSR & (1 << 17)):
			print "Invalid state usage fault!"
			print "1) Loading branch target address to PC with LSB equals zero.\n" \
				+ "Stacked PC should show the branch target.\n" \
				+ "2) LSB of vector address in vector table is zero. Stacked PC\n" \
				+ "should show the starting of exception handler.\n" \
				+ "3) Stacked PSR corrupted during exception handling, so after\n" \
				+ "the exception the core tries to return to the interrupted code in\n" \
				+ "ARM state."
		if (CFSR & (1 << 16)):
			print "Undefined instruction usage fault!"
			print "1) Use of instructions not supported in Cortex-M3/M4\n" \
				+ "2) Bad/corrupted memory contents\n" \
				+ "3) Loading of ARM object code during link stage. Check compilation steps.\n" \
				+ "4) Instruction align problem. For example, if GNU Toolchain is\n" \
				+ "used, omitting of .align after .ascii might cause next instruction to be unaligned\n" \
				+ "(start in odd memory address instead of half-word addresses)."
		if (CFSR & (1 << 13)):
			print "Bus fault on stacking for exception entry!"
			print "Error occurred during stacking (starting of exception)\n" \
				+ "- Stack pointer is corrupted.\n" \
				+ "- Stack size go too large, reaching an undefined memory region.\n" \
				+ "- PSP is used but not initialized."
		if (CFSR & (1 << 12)):
			print "Bus fault on unstacking for a return from exception!"
			print "Error occurred during unstacking (ending of exception).\n" \
				+ "If there was no error stacking but error occurred during\n" \
				+ "unstacking, it might be that the stack pointer was corrupted during exception."
		if (CFSR & (1 << 11)):
			print "Imprecise data bus error!"
			print "Bus error during data access. Bus error could be caused by\n" \
				+ "- Coding error that leads to access of invalid memory space.\n" \
				+ "- Device accessed return error response. For example,\n" \
				+ "device has not been initialized, access of privileged-only\n" \
				+ "device in user mode, or the transfer size is incorrect for the\n" \
				+ "specific device."
		if (CFSR & (1 << 10)):
			print "Precise data bus error!"
			print "Bus error during data access. The fault address may be\n" \
				+ "indicated by BFAR. A bus error could be caused by:\n" \
				+ "- Coding error which leads to access of invalid memory space.\n" \
				+ "- Device accessed return error response. For example, the\n" \
				+ "device has not been initialized, or the transfer size is\n" \
				+ "incorrect for the specific device, or access of privilegedonly device\n" \
				+ "in unprivileged state (including System Control Space registers and\n" \
				+ "debug components on System region)."
		if (CFSR & (1 << 9)):
			print "Instruction bus error!"
			print "Branch to invalid memory location, which could be caused by\n" \
				+ "- simple coding error. For example, caused by incorrect function pointers in program code.\n" \
				+ "- use of incorrect instruction for exception return\n" \
				+ "- corruption of stack, which can affect stacked LR which is\n" \
				+ "used for normal function returns, or corruption of stack frame which contains return address.\n" \
				+ "- Invalid entry in exception vector table. For example,\n" \
				+ "loading of an executable image for traditional ARM processor core into the memory,\n" \
				+ "or exception happen before vector table in SRAM is set up."
		if (CFSR & (1 << 8)):
			print "Memory manager fault on stacking for exception entry!"
			print "Error occurred during stacking (starting of exception)\n" \
				+ "1) Stack pointer is corrupted.\n" \
				+ "2) Stack size go too large, reaching a region not defined by\n" \
				+ "the MPU or disallowed in the MPU configuration."
		if (CFSR & (1 << 7)):
			print "Memory manager fault on unstacking for a return from exception!"
			print " Error occurred during unstacking (ending of exception).\n" \
				+ "If there was no error stacking but error occurred during unstacking, it might be:\n" \
				+ "- Stack pointer was corrupted during exception.\n" \
				+ "- MPU configuration changed by exception handler."
		if (CFSR & (1 << 5)):
			print "Data access violation flag!"
			print "Violation to memory access protection, which is defined by MPU setup.\n" \
				+ "For example, user application (unprivileged) trying to access privileged-only region."
		if (CFSR & (1 << 4)):
			print "Instruction access violation flag!"
			print "1) Violation to memory access protection, which is defined by MPU setup.\n" \
				+ "For example, user application (unprivileged) trying to access privileged-only region.\n" \
				+ "Stacked PC might able to locate the code that has caused the problem.\n" \
				+ "2) Branch to non-executable regions, which could be caused by\n" \
				+ "- simple coding error.\n" \
				+ "- use of incorrect instruction for exception return\n" \
				+ "- corruption of stack, which can affect stacked LR which is used for normal function returns,\n" \
				+ "or corruption of stack frame which contains return address.\n" \
				+ "- Invalid entry in exception vector table. For example,\n" \
				+ "loading of an executable image for traditional ARM processor core into the memory,\n" \
				+ "or exception happen before vector table in SRAM is set up."
		if (CFSR & (1 << 3)):
			print "Indicates a forced Hard Fault!"
			print "1) Trying to run SVC/BKPT within SVC/monitor or another handler with same or higher priority.\n" \
				+ "2) A fault occurred, but it corresponding handler is disabled or\n" \
				+ "cannot be started because another exception with same or\n" \
				+ "higher priority is running, or because exception mask is set."
		if (CFSR & (1 << 1)):
			print "Indicates a Bus Fault on a vector table read during exception processing!"
			print "Vector fetch failed. Could be caused by:\n" \
				+ "1) Bus fault at vector fetch.\n" \
				+ "2) Incorrect vector table offset setup."
		print "------------------"
		if (CFSR & ((1 << 25) | (1 << 24) | (1 << 19) | (1 << 17) | (1 << 16) | 1)):
			print "Check PC: 0x%X" %PC
			print "- Where:"
			cmd = 'arm-none-eabi-addr2line -C -f -e nrc7292_cspi.elf %X' %PC
			print subprocess.check_output(cmd, shell=True)
		if (CFSR & (1 << 18)):
			print "Check LR: 0x%X" %LR
			print "- Where:"
			cmd = 'arm-none-eabi-addr2line -C -f -e nrc7292_cspi.elf %X' %LR
			print subprocess.check_output(cmd, shell=True)
		if not(~CFSR & ((1 << 9) | (1 << 13))):
			print "Check BFAR: 0x%X" %BFAR
			print "- Where:"
			cmd = 'arm-none-eabi-addr2line -C -f -e nrc7292_cspi.elf %X' %BFAR
			print subprocess.check_output(cmd, shell=True)
		if not(~CFSR & ((1 << 7) | (1 << 1))):
			print "Check MMAR: 0x%X" %MMAR
			print "- Where:"
			cmd = 'arm-none-eabi-addr2line -C -f -e nrc7292_cspi.elf %X' %MMAR
			print subprocess.check_output(cmd, shell=True)
		print ""
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)

def decode_lmac_dump(f, curr):
	try:
		print "---------------"
		print "LMAC DUMP"
		print "---------------"
		print "Registers in TX"
		print "---------------"
		m_curr = check_align(f, curr, 4)
		print "TX PSDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "TX MPDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "MPDU Length Mismatch Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "MPDU Length Mismatch Capture: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "Data Process State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "Sec Wrapper State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Protected MPDU: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "KEY Search Error MPDU: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "TXControl State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "  State after sifs ready: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "  State after control frame to tsw: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "  State after rxppdu start: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "TX Vector late Count: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "PHY Interface state: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Buffered data Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "MAX Buffered data Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "RTS gen Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  CTS before CTS Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  Other before CTS Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  NOT OK before CTS Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  CTS Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "ACK DATA PSDU gen Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  ACK before ACK Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  Other before ACK Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  NOT OK before ACK Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  ACK Timeout Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "RTS receive Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  CTS gen Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "NORMAL RID Data receive count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "NDP RID Data receive count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "  ACK gen count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "NO RID Data receive count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "TX request Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "PHY TX PPDU Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Success Data PSDU Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Requested Data Psdu Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "BO Invoke Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "SIFS Invoke Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "BD2 Data Length: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "DMA FIFO Read Length: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Last DMA Request Length: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Last DMA Request Address: 0x%X" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "PHY TXFIFO Write Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "PHY TXFIFO Read  Count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "TX Vector: 0x%X" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+--Format: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+--Bandwidth: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 2)
		print "TX SIG0: 0x%X" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+--1Mhz  MCS: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  1Mhz  Aggregation: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 2)
		print "+  1Mhz  Length: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  1Mhz  RID: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  1Mhz  NDP Indication: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 2)
		print "TX SIG1: 0x%X" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+--Short MCS: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  Short BW: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  Short Aggregation: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 2)
		print "+  Short Length: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  Short RID: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "+  Short NDP Indication: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "PHY CCA: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RX End TD: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 2)
		print "PHY Error Count&TX Timeout: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Buffered Data Error count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "PHY recovery count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "SIFS Late Invoke count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "SIFS Late Invoke LAST elapsed Time: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "AC0 Late Invoke count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "    Late Invoke MAX elapsed slot count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "    Late Invoke Cumulative elapsed slot count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "AC1 Late Invoke count ac1: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "    Late Invoke MAX elapsed slot count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "    Late Invoke Cumulative elapsed slot count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "AC2 Late Invoke count ac2: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "    Late Invoke MAX elapsed slot count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "    Late Invoke Cumulative elapsed slot count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "AC3 Late Invoke count ac3: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "    Late Invoke MAX elapsed slot count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "    Late Invoke Cumulative elapsed slot count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "BCN Late Invoke count bcn: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "    Late Invoke MAX elapsed slot count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "    Late Invoke Cumulative elapsed slot count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print " GP Late Invoke count gp: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "    Late Invoke MAX elapsed slot count: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "    Late Invoke Cumulative elapsed slot count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32 monitor sample: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32 monitor cca: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32 monitor cca 1m: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32 monitor cca 2m: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32 monitor cca 2m sec: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "control last usgae info: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "BW Match Fail Count 0: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "BW Match Fail Count 1: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "BW Match Fail Count 2: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "BW Match Fail Count 3: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "BW Match Fail Count 4: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "BW Match Fail Count 5: %d" %struct.unpack('B', f.read(1))[0]
		print ""

		print "---------------"
		print "Registers in RX"
		print "---------------"
		m_curr = check_align(f, m_curr, 4)
		print "Rx DMA CMD Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "RDI Normal MPDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Counter PSDU Total Received: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Counter MPDU Total Received: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Counter MPDU Total Success: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Counter MPDU CRC Error: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Counter MPDU Length Error: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Counter PSDU SEQ Error: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "RDI Abort MPDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RDI Abort Reason Count: Abort MPDU %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RDI Abort Reason Count: FIFO full %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RDI Abort Reason Count: descriptor underrun %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RDI Rx Info number error: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_filter_flag_set: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "RX Filter FSM: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_filter_counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_filter_fifo_dma_control: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_filter_match_result: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_obss_counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "dead_beef: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "w8_rx_dma_if_state: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "w4_rx_sec_wrapper_state: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Number of protected mpdu: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Number of key search error mpdu: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Number of mic check error mpdu: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "w6_rx_dea_crc_current_state: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "MAX RSW FIFO Depth: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Command Queue Full MPDU Count: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RSW FIFO Sig: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Current RSW FIFO Depth: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "0: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "0: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_fifo_counter_rx_cdc: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_fifo_max_counter_rx_cdc: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Addr Match Success MPDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Addr Match Fail MPDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Addr Match Success NDP Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Addr Match Fail NDP Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "w4_cmzero_fsm_state: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_cmzero_interrupt_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_cmzero_psdu_crc_error_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rdi_dma_queue_cnt: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RX current filter fifo depth: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RX max used filter fifo depth: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Exception Occurred MPDU Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_ndp_cmac_1m_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_ndp_cmac_2m_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_not_ndp_cmac_1m_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_not_ndp_cmac_2m_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "RX NOT Supported Format: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_rx_long_preamble_2m_count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Rx Start Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Rx Counter No data: %d" %struct.unpack('I', f.read(4))[0]
		print ""

		print "-----------------"
		print "Registers in DMA"
		print "-----------------"
		m_curr = check_align(f, m_curr, 1)
		print "Ahm State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "AHB CTRL State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "ARB State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "TX DMA CTRL State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "RX DMA Ctrl State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "DMAC READ FIFO Counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "DMAC READ FIFO MAX Counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "DMAC WRITE FIFO Counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "DMAC WRITE FIFO MAX Counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 1)
		print "NEW_MAC_TX_CNT_MAC_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "NEW_MAC_TX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "NEW_BUS_TX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "MAX_MAC_TX_CNT_MAC_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "MAX_MAC_TX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "MAX_BUS_TX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "NEW_MAC_RX_CNT_MAC_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "NEW_MAC_RX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "NEW_BUS_RX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "MAX_MAC_RX_CNT_MAC_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "MAX_MAC_RX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "MAX_BUS_RX_CNT_BUS_CLK: %d" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "DMA Error Counter: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Read Index Mismatch Error Counter: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "Write Index Mismatch Error Counter: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "RxInfo Mismatch Error: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "w32_reg_fill_done_status: 0x%X" %struct.unpack('I', f.read(4))[0]
		print ""

		print "-----------------"
		print "Registers in SEC"
		print "-----------------"
		m_curr = check_align(f, m_curr, 1)
		print "SEC controller State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "CCMP controller State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "Key controller State: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "CCMP input fifo signal: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		print "CCMP output fifo signal: 0x%X" %struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Number of encrypted MPDU: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Number of decrypted MPDU: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Number of bus transaction: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "ASM State: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Calculated mic value lower 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Calculated mic value upper 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Received mic value lower 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Received mic value upper 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Encrypted mic value lower 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Encrypted mic value upper 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "mic error information: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "mic error key lower 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "mic error key upper 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "mic error peer address lower 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "mic error peer address upper 32bit: 0x%X" %struct.unpack('I', f.read(4))[0]
		print ""

		print "-----------------"
		print "Registers in IRQ"
		print "-----------------"
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count0: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count1: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count2: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count3: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count4: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count5: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count6: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count7: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count8: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count9: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count10: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count11: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count12: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count13: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count14: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count15: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count16: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count17: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count18: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count19: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count20: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count21: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count22: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count23: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Interrupt Read Count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "Total Interrupt Count: %d" %struct.unpack('I', f.read(4))[0]
		print ""
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)

def decode_qm_dump(f, curr):
	try:
		n_qm = [0, 1, 2, 3, 4, 5]
		n_stats = ["Q_RE_TX       ", "Q_NEW_TX      ", "Q_PENDING     ", "Q_RE_TX_CHUNK ", "Q_NEW_TX_CHUNK",
					"Q_PEND_CHUNK  ", "LIST_SCH_CHUNK", "LIST_FRE_CHUNK", "Total Queued  ", "Total Byte    ",
					"Remain Byte   ", "Issued Tx     ", "Retx Queued   ", "Sent MPDU     ", "Sent AMPDU    ",
					"Sent PSDU     ", "Tx Success    ", "Tx Failure    ", "# BO issued   ", "# SIFS issued ",
					"# MPDU Free   ", "# Discard     ", "# UL Request  ", "# Ack Receive "]
		n_credits = ["Buf MagicCode ", "Buf TLV       ", "Buf s1ghook   ", "Buf diff      ", "Update Credit ",
					"Allocate Buf  ", "Credit Loss   ", "Txop wa no wrk"]
		n_state = ["WAIT_FRAME    ", "WAIT_CHANNEL  ", "WAIT_ACK      ", "CHANNEL_READY "]
		print "--------------------------------------------------------------------------------------------------"
		print "Item                 QM[0]      QM[1]      QM[2]      QM[3]      QM[4]      QM[5]"
		print "--------------------------------------------------------------------------------------------------"
		m_curr = curr

		for i in n_stats:
			string = ["{0}".format(i)]
			for j in n_qm:
				m_curr = check_align(f, m_curr, 2)
				val = struct.unpack('H', f.read(2))[0]
				str_val = "{0:>10}".format(val)
				string.append(str_val)
			idx = 0
			for k in string:
				print "{0}".format(string[idx]),
				idx += 1
			print ""


		print "--------------------------------------------------------------------------------------------------"
		print "Buffer & Credit"
		print "--------------------------------------------------------------------------------------------------"

		for i in n_credits:
			string = ["{0}".format(i)]
			for j in n_qm:
				m_curr = check_align(f, m_curr, 4)
				val = struct.unpack('I', f.read(4))[0]
				str_val = "{0:>10}".format(val)
				string.append(str_val)
			idx = 0
			for k in string:
				print "{0}".format(string[idx]),
				idx += 1
			print ""


		print "--------------------------------------------------------------------------------------------------"
		print "QM State Hit"
		print "--------------------------------------------------------------------------------------------------"

		c_string = []
		c_string.append("Current state ")
		for i in n_qm:
			m_curr = check_align(f, m_curr, 1)
			val = struct.unpack('B', f.read(1))[0]
			str_val = "{0:>10}".format(val)
			c_string.append(str_val)
		idx = 0
		for i in c_string:
			print "{0}".format(c_string[idx]),
			idx += 1
		print ""

		for i in n_state:
			state_string = ["{0}".format(i)]
			for j in n_qm:
				m_curr = check_align(f, m_curr, 2)
				val = struct.unpack('H', f.read(2))[0]
				str_val = "{0:>10}".format(val)
				state_string.append(str_val)
			idx = 0
			for k in state_string:
				print "{0}".format(state_string[idx]),
				idx += 1
			print ""

		print "--------------------------------------------------------------------------------------------------"
		m_curr = check_align(f, m_curr, 2)
		reset = struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		success = struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		fail = struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		case4 = struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		isr_winac = struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		isr_dl = struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		isr_etc = struct.unpack('H', f.read(2))[0]

		print "Number of workaround for async Txdone> reset:{0}, s:{1}, f:{2} case 4:{3} isr_winac:{4} isr_dl:{5} isr_etc:{6}".format(reset, success, fail, case4, isr_winac, isr_dl, isr_etc)
		print "--------------------------------------------------------------------------------------------------"
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)

def decode_dl_dump(f, curr):
	try:
		print "<Downlink Statistics>"
		m_curr = check_align(f, curr, 4)
		total_rx_byte = struct.unpack('I', f.read(4))[0]
		print " -- Total MPDU Received Byte:{0}".format(total_rx_byte)
		m_curr = check_align(f, m_curr, 4)
		discard_cnt = struct.unpack('I', f.read(4))[0]
		print " -- Discarded count:{0}".format(discard_cnt)

		m_curr = check_align(f, m_curr, 1)
		good = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		crc_err = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		match_err = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		len_err = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		key_err = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		mic_err = struct.unpack('B', f.read(1))[0]
		m_curr = check_align(f, m_curr, 1)
		etc = struct.unpack('B', f.read(1))[0]
		print " -- DescRing: good:{0}, crc_err:{1}, match_err:{2}, len_err:{3}".format(good, crc_err, match_err, len_err)
		print "                      key_err:{0}, mic_err:{1}, etc:{2}".format(key_err, mic_err, etc)
		print ""

		print " -- SW valid:"
		desc_cnt = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
			10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
			20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31]
		idx = 0
		for i in desc_cnt:
			m_curr = check_align(f, m_curr, 1)
			val = struct.unpack('B', f.read(1))[0]
			print "[{0:>2}] {1}".format(i,val),
			if ((idx % 8)==7):
				print ""
			idx += 1

		print ""
		print " -- Ownership:"
		idx = 0
		for i in desc_cnt:
			m_curr = check_align(f, m_curr, 1)
			val = struct.unpack('B', f.read(1))[0]
			if val == 1:
				print "[{0:>2}] H".format(i),
			else:
				print "[{0:>2}] S".format(i),
			if ((idx % 8)==7):
				print ""
			idx += 1
		print ""

		# vif_cnt = [0, 1]
		# for i in vif_cnt:
		# 	print " -- seq num control:"
		# 	m_curr = check_align(f, m_curr, 1)
		# 	mac0 = struct.unpack('B', f.read(1))[0]
		# 	m_curr = check_align(f, m_curr, 1)
		# 	mac1 = struct.unpack('B', f.read(1))[0]
		# 	m_curr = check_align(f, m_curr, 1)
		# 	mac2 = struct.unpack('B', f.read(1))[0]
		# 	m_curr = check_align(f, m_curr, 1)
		# 	mac3 = struct.unpack('B', f.read(1))[0]
		# 	m_curr = check_align(f, m_curr, 1)
		# 	mac4 = struct.unpack('B', f.read(1))[0]
		# 	m_curr = check_align(f, m_curr, 1)
		# 	mac5 = struct.unpack('B', f.read(1))[0]
		# 	print " --- addr: {0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}".format(mac0, mac1, mac2, mac3, mac4, mac5) 

		# 	m_curr = check_align(f, m_curr, 1)
		# 	tid = struct.unpack('B', f.read(1))[0]
		# 	print " --- tid: {0}".format(tid)
		# 	m_curr = check_align(f, m_curr, 2)
		# 	start = struct.unpack('H', f.read(2))[0]
		# 	m_curr = check_align(f, m_curr, 2)
		# 	end = struct.unpack('H', f.read(2))[0]
		# 	print " --- window: start: {0}, end: {1}".format(start, end)
		# 	m_curr = check_align(f, m_curr, 1)
		# 	bitmap = struct.unpack('B', f.read(1))[0]
		# 	print "             bitmap: {0:x}(hex)".format(bitmap)

	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)
 
def decode_phy_dump(f, curr):
	try:
		print "---------------"
		print "PHY DUMP"
		print "---------------"
		m_curr = check_align(f, curr, 4)
		print "CS mon 1 - total CS: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "CS mon 2 - 1M corr CS: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "CS mon 3 - 2M corr CS: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "CS mon 4 - sat corr CS: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "CS mon 5 - pwr corr CS: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 2)
		print "AGC mon 1 - pwr report: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 2)
		print "AGC mon 2 - Total Gain: %d" %struct.unpack('H', f.read(2))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SYNC mon 1 - sync done: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SYNC mon 2 - sync missing: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SYNC mon 3 - sync timeout: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "RSSI mon 1 - rssi timeout: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "S1G1M mon 1 -  S1G_1M detection: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "S1G1M mon 2 - S1G detection: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SHTLNG mon 1 - short det cnt: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SHTLNG mon 2 - long det cnt: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SIG mon 1 - SIG OK: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SIG mon 2 - SIG error: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SIG mon 3 - SIG My frame: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SIG mon 4 - SIG My AP frame: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SIG mon 5 - NDP count: %d" %struct.unpack('I', f.read(4))[0]
		m_curr = check_align(f, m_curr, 4)
		print "SIG mon 6 - NDPCMAC count: %d" %struct.unpack('I', f.read(4))[0]
		print ""
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)
 
def decode_rf_dump(f, curr):
	try:
		print "---------------"
		print "RF DUMP"
		print "---------------"
		m_curr = check_align(f, curr, 4)
		print "Sample value: 0x%X" %struct.unpack('I', f.read(4))[0]
		print ""
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)

def decode_hostif_dump(f, curr):
	try:
		print "---------------"
		print "Host Interface DUMP"
		print "---------------"
		m_curr = check_align(f, curr, 4)
		print "Sample value: 0x%X" %struct.unpack('I', f.read(4))[0]
		print ""
	except Exception as e:
		traceback.print_exc()
	finally:
		return (m_curr)

with open(filename, "rb") as f:
	curr = decode_ver_dump(f, 0)
	curr = decode_corereg_dump(f, curr)
	curr = decode_lmac_dump(f, curr)
	curr = decode_qm_dump(f, curr)
	curr = decode_dl_dump(f, curr)
	curr = decode_phy_dump(f, curr)
	curr = decode_rf_dump(f, curr)
	if 'host' in filename:
		curr = decode_hostif_dump(f, curr)
	print "read cnt: %d" %curr
	print "Decode Core Dump Done."


