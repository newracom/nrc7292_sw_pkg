#!/usr/bin/python

import sys, os, time, subprocess, re
from mesh import *
script_path = "/home/pi/nrc_pkg/script/"

# Default Configuration (you can change value you want here)
##################################################################################
# Raspbery Pi Conf.
max_cpuclock      = 1         # Set Max CPU Clock : 0(off) or 1(on)
##################################################################################
# Firmware Conf.
model             = 7292      # 7292
fw_download       = 1         # 0(FW Download off) or 1(FW Download on)
fw_name           = 'uni_s1g.bin'
##################################################################################
# DEBUG Conf.
# NRC Driver log
driver_debug      = 0         # NRC Driver debug option : 0(off) or 1(on)
#--------------------------------------------------------------------------------#
# WPA Supplicant Log (STA Only)
supplicant_debug  = 0         # WPA Supplicant debug option : 0(off) or 1(on)
#--------------------------------------------------------------------------------#
# HOSTAPD Log (AP Only)
hostapd_debug     = 0         # Hostapd debug option    : 0(off) or 1(on)
#################################################################################
# CSPI Conf. (Default)
spi_clock    = 20000000       # SPI Master Clock Frequency
spi_bus_num  = 0              # SPI Master Bus Number
spi_cs_num   = 0              # SPI Master Chipselect Number
spi_gpio_irq = 5              # NRC-CSPI EIRQ GPIO Number
                              # BBB is 60 recommanded.
spi_polling_interval = 0      # NRC-CSPI Polling Interval (msec)

#
# NOTE:
#  - NRC-CSPI EIRQ Input Interrupt: spi_gpio_irq >= 0 and spi_polling_interval <= 0
#  - NRC-CSPI EIRQ Input Polling  : spi_gpio_irq >= 0 and spi_polling_interval > 0
#  - NRC-CSPI Registers Polling   : spi_gpio_irq < 0 and spi_polling_interval > 0
#
#--------------------------------------------------------------------------------#
# FT232H USB-SPI Conf. (FT232H CSPI Conf)
ft232h_usb_spi = 0            # FTDI FT232H USB-SPI bridge
                              # 0 : Unused
                              # 1 : NRC-CSPI_EIRQ Input Polling
                              # 2 : NRC-CSPI Registers Polling
#################################################################################
# RF Conf.
# Maximum TX Power
txpwr_max_default = 24       # Board Data Max TX Power
#--------------------------------------------------------------------------------#

##################################################################################
# PHY Conf.
guard_int         = 'long'   # Guard Interval ('long'(LGI) or 'short'(SGI))
##################################################################################
# MAC Conf.
# S1G Short Beacon (AP & MESH Only)
#  If disabled, AP sends only S1G long beacon every BI
#  Recommend using S1G short beacon for network efficiency (Default: enabled)
short_bcn_enable  = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# AMPDU (Aggregated MPDU)
#  Enable AMPDU for full channel utilization and throughput enhancement
ampdu_enable      = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Legacy ACK enable (AP & STA)
#  If disabled, AP/STA sends only NDP ack frame
#  Recommend using NDP ack mode  (Default: disable)
legacy_ack_enable  = 0        # 0 (NDP ack mode) or 1 (legacy ack mode)
#--------------------------------------------------------------------------------#
# 1M NDP (Block) ACK (AP Only)
#  Enable 1M NDP ACK on 2/4MHz BW for robustness (default: 2M NDP ACK on 2/4MH BW)
#  STA should follow, if enabled on AP
#  Note: if enabled, max # of mpdu in ampdu is restricted to 8 (default: max 16)
ndp_ack_1m        = 0        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# NDP Probe Request
#  For STA, "scan_ssid=1" in wpa_supplicant's conf should be set to use
ndp_preq          = 0        # 0 (Legacy Probe Req) 1 (NDP Probe Req)
#--------------------------------------------------------------------------------#
# CQM (Channel Quality Manager) (STA Only)
#  STA can disconnect according to Channel Quality (Beacon Loss or Poor Signal)
#  Note: if disabled, STA keeps connection regardless of Channel Quality
cqm_enable        = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# RELAY (Do NOT use! it will be deprecated)
relay_type        = 1        # 0 (wlan0: STA, wlan1: AP) 1 (wlan0: AP, wlan1: STA)
#--------------------------------------------------------------------------------#
# Power Save (STA Only)
#  4-types PS: (0)Always on (1)Modem_Sleep (2)Deep_Sleep(TIM) (3)Deep_Sleep(nonTIM
#  Modem Sleep : turn off only RF while PS (Fast wake-up but less power save)
#   Deep Sleep : turn off almost all power (Slow wake-up but much more power save)
#     TIM Mode : check beacons during PS to receive BU from AP
#  nonTIM Mode : Not check beacons during PS (just wake up by TX or EXT INT)
power_save        = 0        # STA (power save type 0~3)
ps_timeout        = '3s'     # STA (timeout before going to sleep) (min:1000ms)
sleep_duration    = '3s'     # STA (sleep duration only for nonTIM deepsleep) (min:1000ms)
listen_interval   = 1000     # STA (listen interval in BI unit) (max:65535)
#--------------------------------------------------------------------------------#
# BSS MAX IDLE PERIOD (aka. keep alive) (AP Only)
#  STA should follow (i.e STA should send any frame before period),if enabled on AP
#  Period is in unit of 1000TU(1024ms, 1TU=1024us)
#  Note: if disabled, AP removes STAs' info only with explicit disconnection like deauth
bss_max_idle_enable = 1      # 0 (disable) or 1 (enable)
bss_max_idle        = 180    # time interval (e.g. 60: 61440ms) (1 ~ 65535)
#--------------------------------------------------------------------------------#
# Mesh Options (Mesh Only)
#  SW encryption by MAC80211 for Mesh Point
sw_enc              = 0     # 0 (disable), 1 (enable)
#  Manual Peering & Static IP
peer                = 0     # 0 (disable) or Peer MAC Address
static_ip           = 0     # 0 (disable) or Static IP Address
#--------------------------------------------------------------------------------#
# Self configuration (AP Only)
#  AP scans the clearest CH and then starts with it
self_config       = 0        # 0 (disable)  or 1 (enable)
prefer_bw         = 0        # 0: no preferred bandwidth, 1: 1M, 2: 2M, 4: 4M
dwell_time        = 100      # max dwell is 1000 (ms), min: 10ms, default: 100ms
#--------------------------------------------------------------------------------#
# Credit num of AC_BE for flow control between host and target (Test only)
credit_ac_be      = 40        # number of buffer (min: 40, max: 120)
#--------------------------------------------------------------------------------#
# Use bitmap encoding for block ack operation (NRC7292 only)
bitmap_encoding   = 1         # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# User scrambler reversely (NRC7292 only)
reverse_scrambler = 1         # 0 (disable) or 1 (enable)
##################################################################################

def check(interface):
    ifconfig_cmd = "ifconfig " + interface
    ifconfig_process = subprocess.Popen(ifconfig_cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    ifconfig_lines = ifconfig_process.communicate()[0]

    try:
        ifconfig_lines = ifconfig_lines.decode()
    except:
        pass

    ifconfig_lines = ifconfig_lines.split("\n")
    for line in ifconfig_lines:
        if "inet 192.168" in line:
            return line
    return ''

def usage_print():
    print("Usage: \n\tstart.py [sta_type] [security_mode] [country] [channel] [sniffer_mode] \
            \n\tstart.py [sta_type] [security_mode] [country] [mesh_mode] [mesh_peering] [mesh_ip]")
    print("Argument:    \n\tsta_type      [0:STA   |  1:AP  |  2:SNIFFER  | 3:RELAY |  4:MESH] \
            \n\tsecurity_mode [0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE | 4:WPS-PBC] \
                         \n\tcountry       [KR:Korea] \
                         \n\t----------------------------------------------------------- \
                         \n\tchannel       [S1G Channel Number]   * Only for Sniffer & AP \
                         \n\tsniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer \
                         \n\tmesh_mode     [0:MPP | 1:MP | 2:MAP] * Only for Mesh \
                         \n\tmesh_peering  [Peer MAC address]     * Only for Mesh \
                         \n\tmesh_ip       [Static IP address]    * Only for Mesh")
    print ("Example:  \n\tOPEN mode STA for Korea                : ./start.py 0 0 KR \
                      \n\tSecurity mode AP for US                : ./start.py 1 1 US \
                      \n\tLocal Sniffer mode on CH 40 for Japan  : ./start.py 2 0 JP 40 0 \
                      \n\tSAE mode Mesh AP for US                : ./start.py 4 3 US 2 \
                      \n\tMesh Point with static ip              : ./start.py 4 3 US 1 192.168.222.1 \
                      \n\tMesh Point with manual peering         : ./start.py 4 3 US 1 8c:0f:fa:00:29:46 \
                      \n\tMesh Point with manual peering & ip    : ./start.py 4 3 US 1 8c:0f:fa:00:29:46 192.168.222.1")
    print("Note: \n\tsniffer_mode should be set as '1' when running sniffer on remote terminal \
                  \n\tMPP, MP mode support only Open, WPA3-SAE security mode")
    exit()

def strSTA():
    if int(sys.argv[1]) == 0:
        return 'STA'
    elif int(sys.argv[1]) == 1:
        return 'AP'
    elif int(sys.argv[1]) == 2:
        return 'SNIFFER'
    elif int(sys.argv[1]) == 3:
        return 'RELAY'
    elif int(sys.argv[1]) == 4:
        return 'MESH'
    else:
        usage_print()

def checkCountry():
    country_list = ["KR"]
    if str(sys.argv[3]) in country_list:
        return
    else:
        print ("Only KR MIC channel frequencies are supported. Please select the country code KR.")
        exit()

def checkMeshUsage():
    global sw_enc,relay_type, peer, static_ip
    if len(sys.argv) < 5:
        usage_print()
    sw_enc = 1
    relay_type = int(sys.argv[4])
    if len(sys.argv) == 6:
        if isMacAddress(sys.argv[5]):
            peer = sys.argv[5]
        elif isIP(sys.argv[5]):
            static_ip = sys.argv[5]
        else:
            usage_print()
    elif len(sys.argv) == 7:
        if isMacAddress(sys.argv[5]):
            peer = sys.argv[5]
        else:
            usage_print()
        if isIP(sys.argv[6]):
            static_ip = sys.argv[6]
        else:
            usage_print()
    argv_print()

def strSecurity():
    if int(sys.argv[2]) == 0:
        return 'OPEN'
    elif int(sys.argv[2]) == 1:
        return 'WPA2-PSK'
    elif int(sys.argv[2]) == 2:
        return 'WPA3-OWE'
    elif int(sys.argv[2]) == 3:
        return 'WPA3-SAE'
    elif int(sys.argv[2]) == 4:
        return 'WPA-PBC'
    else:
        usage_print()

def strPSType():
    if int(power_save) == 0:
        return 'Always On'
    elif int(power_save) == 1:
        return 'Modem Sleep (TIM)'
    elif int(power_save) == 2:
        return 'Deep Sleep (TIM)'
    elif int(power_save) == 3:
        return 'Deep Sleep (nonTIM)'
    else:
        return 'Invalid Type'

def strSnifferMode():
    if int(sys.argv[5]) == 0:
        return 'LOCAL'
    elif int(sys.argv[5]) == 1:
        return 'REMOTE'
    else:
        usage_print()

def strOnOff(param):
    if int(param) == 1:
        return 'ON'
    else:
        return 'OFF'

def strMeshMode():
    if int(sys.argv[4]) == 0:
        return 'Mesh Portal'
    elif int(sys.argv[4]) == 1:
        return 'Mesh Point'
    elif int(sys.argv[4]) == 2:
        return 'Mesh AP'
    else:
        usage_print()

def strOriCountry():
    if str(sys.argv[3]) == 'EU':
        return 'DE'
    else:
        return str(sys.argv[3])

def isNumber(s):
    try:
        float(s)
        return True
    except ValueError:
        return False

def isMacAddress(s):
    if len(s) == 17 and ':' in s:
        return True
    else:
        return False

def isIP(s):
    if len(s) > 6 and len(s) < 16 and '.' in s:
        return True
    else:
        return False

def argv_print():
    global fw_name
    print("------------------------------")
    print("Model            : " + str(model))
    print("STA Type         : " + strSTA())
    print("Country          : " + str(sys.argv[3]))
    print("Security Mode    : " + strSecurity())
    print("AMPDU            : " + strOnOff(ampdu_enable))
    if strSTA() == 'STA':
        print("CQM              : " + strOnOff(cqm_enable))
    if strSTA() == 'SNIFFER':
        print("Channel Selected : " + str(sys.argv[4]))
        print("Sniffer Mode     : " + strSnifferMode())
    if int(fw_download) == 1:
        print("Download FW      : " + fw_name)
    print ("Max TX Power     : " + str(txpwr_max_default))
    if int(bss_max_idle_enable) == 1 and strSTA() == 'AP':
        print("BSS MAX IDLE     : " + str(bss_max_idle))
    if strSTA() == 'STA':
        print("Power Save Type  : " + strPSType())
        if int(power_save) > 0:
            print("PS Timeout       : " + ps_timeout)
        if int(power_save) == 3:
            print("Sleep Duration   : " + sleep_duration)
        if int(listen_interval) > 0:
            print("Listen Interval  : " + str(listen_interval))
    if strSTA() == 'MESH':
        print("Mesh Mode        : " + strMeshMode())
    print("------------------------------")

def copyConf():
    os.system("sudo /home/pi/nrc_pkg/sw/firmware/copy " + str(model))
    os.system("/home/pi/nrc_pkg/script/conf/etc/ip_config.sh " + strSTA() + " " +  str(relay_type) + " " + str(static_ip))

def startNAT():
    os.system('sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"')
    os.system("sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE")
    os.system("sudo iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT")
    os.system("sudo iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT")

def stopNAT():
    os.system('sudo sh -c "echo 0 > /proc/sys/net/ipv4/ip_forward"')
    os.system("sudo iptables -t nat --flush")
    os.system("sudo iptables --flush")

def startDHCPCD():
    os.system("sudo systemctl start dhcpcd")

def stopDHCPCD():
    os.system("sudo systemctl stop dhcpcd")

def startDNSMASQ():
    os.system("sudo systemctl start dnsmasq")

def stopDNSMASQ():
    os.system("sudo systemctl stop dnsmasq")

def addWLANInterface(interface):
    if strSTA() == 'RELAY' and interface == "wlan1":
        print("[*] Create wlan1 for concurrent mode")
        os.system("sudo iw dev wlan0 interface add wlan1 type managed")
        os.system("sudo ifconfig wlan1 up")
        time.sleep(3)

def self_config_check():
    country = str(sys.argv[3])
    conf_path = script_path + "conf/" + country
    conf_file = ""
    orig_channel = ""
    global dwell_time

    if strSecurity() == "OPEN" :
        conf_file+="/ap_halow_open.conf"
    elif strSecurity() == 'WPA2-PSK' :
        conf_file+="/ap_halow_wpa2.conf"
    elif strSecurity() == 'WPA3-OWE' :
        conf_file+="/ap_halow_owe.conf"
    elif strSecurity() == 'WPA3-SAE' :
        conf_file+="/ap_halow_sae.conf"
    elif strSecurity() == 'WPA-PBC' :
        conf_file+="/ap_halow_pbc.conf"

    print("country: " + country +", prefer_bw: " + str(prefer_bw)+ ", dwell_time: "+ str(dwell_time))

    self_conf_cmd = script_path+'cli_app set self_config ' + country +' '+str(prefer_bw) +' ' + str(dwell_time) + ' '
    if int(dwell_time) > 1000:
        dwell_time = 1000
    elif int(dwell_time) < 10:
        dwell_time = 10
    # Max num of 1M channel is 26
    checkout_timeout = int(dwell_time)*26
    try:
        print("Start CCA scan.... It will take up to "+str(checkout_timeout/1000) +" sec to complete")
        result = subprocess.check_output('timeout ' +str(checkout_timeout) +' ' +self_conf_cmd, shell=True)
        result = result.decode()
    except:
        sys.exit("[self_configuration] No return best channel within " +str(checkout_timeout/1000) +" seconds")

    if 'no_self_conf' in result:
        print("Target FW does NOT support self configuration. Please check FW")
        return 'Fail'
    else:
        print(result)
        best_channel = re.split('[:,\s,\t,\n]+', result)[-3]
        os.system("sudo cp " + conf_path+conf_file + " temp_self_config.conf")
        os.system("sudo mv temp_self_config.conf " +script_path +"conf/")
        os.system("sed -i '/channel=/d' " + script_path + "conf/temp_self_config.conf")
        os.system("sed -i '/hw_mode=/d' " + script_path + "conf/temp_self_config.conf")
        os.system("sed -i '/#ssid=/d' " + script_path + "conf/temp_self_config.conf")
        if int(best_channel) < 36:
            os.system('sed -i "/ssid=.*/ahw_mode=' + 'g' +'" ' + script_path + 'conf/temp_self_config.conf')
        else:
            os.system('sed -i "/ssid=.*/ahw_mode=' + 'a' +'" ' + script_path + 'conf/temp_self_config.conf')
        os.system('sed -i "/hw_mode=.*/achannel=' + best_channel +'" ' + script_path + 'conf/temp_self_config.conf')
        print("Start with channel: " + best_channel)
        return 'Done'

def ft232h_usb():
    # Re-define SPI parameters for ft232h_usb_spi
    # ft232h_usb_spi
    global spi_clock, spi_bus_num, spi_gpio_irq, spi_cs_num, spi_polling_interval
    print("[*] use ft232h_usb_spi")
    spi_bus_num = 3
    spi_gpio_irq = 500
    if int(spi_clock) > 15000000:
        spi_clock = 15000000
    if int(spi_cs_num) != 0:
        spi_cs_num = 0
    if int(spi_polling_interval) <= 0:
        spi_polling_interval = 50
    if int(ft232h_usb_spi) != 1:
        spi_gpio_irq = -1

def setAPParam():
    # Re-define parameters for AP mode
    global ndp_preq
    ndp_preq=1

def setRelayParam():
    # Re-define parameters for RELAY mode
    global sw_enc, power_save, ndp_ack_1m, ndp_preq
    power_save=0; ndp_ack_1m=0; ndp_preq=0;
    # Use sw_enc=2 with Hybrid Security
    # sw_enc=2

def setSnifferParam():
    # Re-define parameters for Sniffer mode
    global sw_enc, ampdu_enable, bss_max_idle_enable, power_save, ndp_ack_1m, listen_interval
    sw_enc=0; ampdu_enable=0; bss_max_idle_enable=0; power_save=0; ndp_ack_1m=0; listen_interval=0;

def setModuleParam():
    # Set module parameters based on configuration

    # Initialize arguments for module params
    spi_arg = fw_arg = power_save_arg = sleep_duration_arg = \
    bss_max_idle_arg = ndp_preq_arg = ndp_ack_1m_arg = auto_ba_arg =\
    sw_enc_arg =  cqm_arg = listen_int_arg = drv_dbg_arg = credit_acbe_arg = \
    sbi_arg = discard_deauth_arg = dbg_fc_arg = legacy_ack_arg = \
    be_arg = rs_arg = ""

    # Check ft232h_usb_spi
    if int(ft232h_usb_spi) > 0:
        ft232h_usb()

    # Set parameters for AP (support NDP probing)
    if strSTA() == 'AP':
        setAPParam()

    # Set parameters for RELAY
    if strSTA() == 'RELAY':
        setRelayParam()

    # Set parameters for SNIFFER
    if strSTA() == 'SNIFFER':
        setSnifferParam()

    # module param for spi setting
    # default:
    #  hifspeed(20000000) spi_bus_num(0) spi_cs_num(0) spi_gpio_irq(5) spi_polling_interval(0)
    spi_arg = " hifspeed=" + str(spi_clock) + \
              " spi_bus_num=" + str(spi_bus_num) + \
              " spi_cs_num=" + str(spi_cs_num) + \
              " spi_gpio_irq=" + str(spi_gpio_irq) + \
              " spi_polling_interval=" + str(spi_polling_interval)

    # module param for FW download from host
    # default: fw_name (NULL: no download)
    if int(fw_download) == 1:
        fw_arg= " fw_name=" + fw_name

    # module param for power_save
    # default: power_save(0: active mode) sleep_duration(0,0)
    if strSTA() == 'STA' and int(power_save) > 0:
        power_save_arg = " power_save=" + str(power_save)
        if int(power_save) == 3:
            sleep_duration_arg = " sleep_duration=" + filter(str.isdigit,sleep_duration)
            unit = sleep_duration[-1]
            if unit == 'm':
                sleep_duration_arg += ",0"
            else:
                sleep_duration_arg += ",1"
        else:
            sleep_duration_arg = " sleep_duration=0,0"

    # module param for bss_max_idle (keep alive)
    # default: bss_max_idle(0: disabled)
    if int(bss_max_idle_enable) == 1:
        if strSTA() == 'AP' or strSTA() == 'RELAY':
            bss_max_idle_arg = " bss_max_idle=" + str(bss_max_idle)

    # module param for NDP Prboe Request (NDP scan)
    # default: ndp_preq(0: disabled)
    if int(ndp_preq) == 1:
        ndp_preq_arg = " ndp_preq=1"

    # module param for legacy ack mode
    # default: 0(Legacy ACK disabled)
    if int(legacy_ack_enable) == 1:
        legacy_ack_arg = " enable_legacy_ack=1"

    # module param for 1MBW NDP ACK
    # default: ndp_ack_1m(0: disabled)
    if int(ndp_ack_1m) == 1:
        ndp_ack_1m_arg = " ndp_ack_1m=1"

    # module param for AMPDU
    # default: auto_ba(0: disabled)
    if int(ampdu_enable) == 1:
        auto_ba_arg = " auto_ba=1"

    # module param for SW-based ENC/DEC
    # default: sw_enc(0: HW-based ENC/DEC)
    if int(sw_enc) == 1:
        sw_enc_arg = " sw_enc=1"

    # module param for CQM
    # default: disable_cqm(0: CQM enabled)
    if int(cqm_enable) == 0:
        cqm_arg = " disable_cqm=1"

    # module param for short beacon
    # default: enable_short_bi(1: Short Beacon enabled)
    if int(short_bcn_enable) == 0:
        sbi_arg = " enable_short_bi=0"

    # module param for listen interval
    # default: listen_interval(100)
    if int(listen_interval) > 0:
        listen_int_arg = " listen_interval=" + str(listen_interval)

    # module param for flow control between host and target (test only)
    # default: credit_ac_be(40)
    if int(credit_ac_be) > 40 and (credit_ac_be) <= 120:
        credit_acbe_arg = " credit_ac_be=" + str(credit_ac_be)

    # module param for driver debug (debug only)
    # default: debug_level_all(0: disabled)
    if int(driver_debug) == 1:
        drv_dbg_arg = " debug_level_all=1"

    # module param for bitmap encoding
    # default: use bitmap encoding (1: enabled)
    if int(bitmap_encoding) == 0:
        be_arg = " bitmap_encoding=0"

    # module param for reverse scrambler
    # default: use reverse scrambler (1: enabled)
    if int(reverse_scrambler) == 0:
        rs_arg = " reverse_scrambler=0"

    # module parameter setting while loading NRC driver
    # Default value is used if arg is not defined
    module_param = spi_arg + fw_arg + \
                 power_save_arg + sleep_duration_arg + bss_max_idle_arg + \
                 ndp_preq_arg + ndp_ack_1m_arg + auto_ba_arg + sw_enc_arg + \
                 cqm_arg + listen_int_arg + drv_dbg_arg + credit_acbe_arg + legacy_ack_arg + \
                 be_arg + rs_arg + sbi_arg

    return module_param

def run_common():
    if int(max_cpuclock) == 1:
        print("[*] Set Max CPU Clock on RPi")
        os.system("sudo /home/pi/nrc_pkg/script/conf/etc/clock_config.sh")

    print("[0] Clear")
    os.system("sudo hostapd_cli disable 2>/dev/null")
    os.system("sudo wpa_cli disable wlan0 2>/dev/null ")
    os.system("sudo wpa_cli disable wlan1 2>/dev/null")
    os.system("sudo killall -9 wpa_supplicant 2>/dev/null")
    os.system("sudo killall -9 hostapd 2>/dev/null")
    os.system("sudo killall -9 wireshark-gtk 2>/dev/null")
    os.system("sudo rmmod nrc 2>/dev/null")
    os.system("sudo rm "+script_path+"conf/temp_self_config.conf 2>/dev/null")
    os.system("sudo rm "+script_path+"conf/temp_hostapd_config.conf 2>/dev/null")
    stopNAT()
    stopDHCPCD()
    stopDNSMASQ()
    time.sleep(1)

    print("[1] Copy and Set Module Parameters")
    copyConf()
    insmod_arg = setModuleParam()

    print("[2] Set Initial Country")
    os.system("sudo iw reg set " + strOriCountry())

    print("[3] Loading module")
    print("sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko " + insmod_arg)
    os.system("sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko " + insmod_arg + "")

    if int(spi_polling_interval) <= 0:
        time.sleep(5)
    else:
        time.sleep(10)

    if strSTA() == 'RELAY' and int(relay_type) == 0:
        addWLANInterface('wlan1')
    elif strSTA() == 'MESH' and int(relay_type) == 2:
        addMeshInterface('mesh0')

    ret = subprocess.call(["sudo", "ifconfig", "wlan0", "up"])
    if ret == 255:
        os.system('sudo rmmod nrc.ko')
        sys.exit()

    print("[4] Transmission Power Control(TPC) is activated")
    os.system('/home/pi/nrc_pkg/script/cli_app set txpwr ' + str(txpwr_max_default))

    print("[5] Set guard interval")
    os.system('/home/pi/nrc_pkg/script/cli_app set gi ' + guard_int)

    print("[*] Start DHCPCD and DNSMASQ")
    startDHCPCD()
    startDNSMASQ()

def run_sta(interface):
    country = str(sys.argv[3])
    os.system("sudo killall -9 wpa_supplicant")

    if int(supplicant_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    if int(power_save) > 0:
        print("[*] Set default power save timeout for " + interface)
        os.system("sudo iwconfig " + interface + " power timeout " + ps_timeout)

    print("[6] Start wpa_supplicant on " + interface)
    if strSecurity() == 'OPEN':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_open.conf " + debug + " &")
    elif strSecurity() == 'WPA2-PSK':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_wpa2.conf " + debug + " &")
    elif strSecurity() == 'WPA3-OWE':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_owe.conf " + debug + " &")
    elif strSecurity() == 'WPA3-SAE':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_sae.conf " + debug + " &")
    elif strSecurity() == 'WPA-PBC':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_pbc.conf " + debug + " &")
        time.sleep(1)
        os.system("sudo wpa_cli wps_pbc")
    time.sleep(3)

    print("[7] Connect and DHCP")
    ret = check(interface)
    while ret == '':
        print("Waiting for IP")
        time.sleep(5)
        ret = check(interface)

    print(ret)
    print("IP assigned. HaLow STA ready")
    print("--------------------------------------------------------------------")

def launch_hostapd(interface, orig_hostapd_conf_file, country, debug, channel):
    print("[*] configure file copied from: %s" % (orig_hostapd_conf_file) )
    TEMP_HOSTAPD_CONF = script_path +  "conf/temp_hostapd_config.conf"
    os.system("sudo cp %s %s" % ( orig_hostapd_conf_file,  TEMP_HOSTAPD_CONF ) )
    os.system("sed -i \"4s/.*/interface=%s/g\" %s" % ( interface, TEMP_HOSTAPD_CONF ) )

    if channel:
        os.system("sed -i \"s/^channel=.*/channel=%s/g\" %s" % ( channel, TEMP_HOSTAPD_CONF ) )

        # According to "UG-7292-001-EVK User Guide (Host Mode).pdf" page 40, the ``hw_mode`` needs to be changed to
        #  ``hw_mode=g`` instead of ``hw_mode=a`` in ``US`` country code.
        if country == "US" and 1 <= int(channel) and int(channel) <= 13:
            os.system("sed -i \"s/^hw_mode=.*/hw_mode=g/g\" %s" % ( TEMP_HOSTAPD_CONF ) )

    os.system("sudo hostapd %s %s &" % ( TEMP_HOSTAPD_CONF, debug ) )


def run_ap(interface):
    country = str(sys.argv[3])
    global self_config
    channel = None

    if len(sys.argv) > 4 :
        channel = str(sys.argv[4])

    if int(hostapd_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    if strSTA() == 'RELAY':
        self_config = 0
        print("[*] Selfconfig is not used in RELAY mode.")
    if (int(self_config)==1):
        print("[*] Self configuration start!")
        self_conf_result = self_config_check()
    elif(int(self_config)==0):
        print("[*] Self configuration off")
    else:
        print("[*] self_conf value should be 0 or 1..  Start with default mode(no self configuration)")

    print("[6] Start hostapd on " + interface)

    if(int(self_config)==1 and self_conf_result=='Done'):
        os.system("sed -i " + '"4s/.*/interface=' + interface + '/g" '  + script_path +  "conf/temp_self_config.conf " )
        os.system("sudo hostapd " + script_path + "conf/temp_self_config.conf " + debug +" &")
        if strSecurity() == 'WPA-PBC':
            time.sleep(1)
            os.system("sudo hostapd_cli wps_pbc")
    else:
        if strSecurity() == 'OPEN':
            launch_hostapd( interface, '/home/pi/nrc_pkg/script/conf/'+country+'/ap_halow_open.conf', country, debug ,channel )
        elif strSecurity() == 'WPA2-PSK':
            launch_hostapd( interface, '/home/pi/nrc_pkg/script/conf/'+country+'/ap_halow_wpa2.conf', country, debug ,channel )
        elif strSecurity() == 'WPA3-OWE':
            launch_hostapd( interface, '/home/pi/nrc_pkg/script/conf/'+country+'/ap_halow_owe.conf', country, debug, channel )
        elif strSecurity() == 'WPA3-SAE':
            launch_hostapd( interface, '/home/pi/nrc_pkg/script/conf/'+country+'/ap_halow_sae.conf', country, debug, channel )
        elif strSecurity() == 'WPA-PBC':
            launch_hostapd( interface, '/home/pi/nrc_pkg/script/conf/'+country+'/ap_halow_pbc.conf', country, debug, channel )
            time.sleep(1)
            os.system("sudo hostapd_cli wps_pbc")
    time.sleep(3)

    print("[7] Start NAT")
    startNAT()

    #print("[8] Start DNSMASQ")
    #startDNSMASQ()

    time.sleep(3)
    print("[8] ifconfig")
    os.system('sudo ifconfig')
    print("HaLow AP ready")
    print("--------------------------------------------------------------------")

def run_sniffer():
    print("[6] Setting Monitor Mode")
    time.sleep(3)
    os.system('sudo ifconfig wlan0 down; sudo iw dev wlan0 set type monitor; sudo ifconfig wlan0 up')
    print("[7] Setting Country: " + strOriCountry())
    os.system("sudo iw reg set " + strOriCountry())
    time.sleep(3)
    print("[8] Setting Channel: " + str(sys.argv[4]))
    os.system("sudo iw dev wlan0 set channel " + str(sys.argv[4]))
    time.sleep(3)
    print("[9] Start Sniffer")
    if strSnifferMode() == 'LOCAL':
        os.system('sudo wireshark-gtk -i wlan0 -k -S -l &')
    else:
        os.system('gksudo wireshark-gtk -i wlan0 -k -S -l &')
    print("HaLow SNIFFER ready")
    print("--------------------------------------------------------------------")

if __name__ == '__main__':
    if len(sys.argv) < 4 or not isNumber(sys.argv[1]) or not isNumber(sys.argv[2]):
        usage_print()
    elif strSTA() == 'SNIFFER' and len(sys.argv) < 6:
        usage_print()
    elif strSTA() == 'MESH':
        checkMeshUsage()
    else:
        argv_print()

    checkCountry()

    print("NRC " + strSTA() + " setting for HaLow...")

    run_common()

    if strSTA() == 'STA':
        run_sta('wlan0')
    elif strSTA() == 'AP':
        run_ap('wlan0')
    elif strSTA() == 'SNIFFER':
        run_sniffer()
    elif strSTA() == 'RELAY':
        #start STA and then AP for Relay
        if int(relay_type) == 0:
            run_sta('wlan0')
            run_ap('wlan1')
        else:
            addWLANInterface('wlan1')
            run_sta('wlan1')
            run_ap('wlan0')
    elif strSTA() == 'MESH':
        if strMeshMode() == 'Mesh Portal':
            run_mpp('wlan0', str(sys.argv[3]), strSecurity(), supplicant_debug, peer, static_ip)
        elif strMeshMode() == 'Mesh Point':
            run_mp('wlan0', str(sys.argv[3]), strSecurity(), supplicant_debug, peer, static_ip)
        elif strMeshMode() == 'Mesh AP':
            run_map('wlan0', 'mesh0', str(sys.argv[3]), strSecurity(), supplicant_debug, peer, static_ip)
        else:
            usage_print()
    else:
        usage_print()

print("Done.")
