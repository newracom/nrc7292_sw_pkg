#!/usr/bin/python

import sys
import os
import time
import commands
import subprocess

##################################################################################
# Default Configuration (you can change value you want here)
model       = 7292      # 7292 or 7192
hif_speed   = 16000000  # HSPI Clock
gain_type   = 'phy'     # 'phy' or 'nrf(legacy)'
txpwr_val   = 17        # TX Power
maxagg_num  = 8         # 0(AMPDU off) or  >2(AMPDU on)
cqm_off     = 0         # 0(CQM on) or 1(CQM off)
fw_download = 1         # 0(FW Download off) or 1(FW Download on)
fw_name     = 'uni_s1g.bin'
bd_download = 0         # 0(Board Data Download off) or 1(Board Data Download on)
bd_name     = 'nrc7292_bd.dat'
guard_int   = 'long'    # 'long'(LGI) or 'short'(SGI)
supplicant_debug = 0    # WPA Supplicant debug option : 0(off) or 1(on)
hostapd_debug    = 0    # Hostapd debug option    : 0(off) or 1(on)
max_cpuclock     = 1    # RPi Max CPU Clock : 0(off) or 1(on)
relay_type       = 0    # 0 (wlan0: STA, wlan1: AP) 1 (wlan0: AP, wlan1: STA)
power_save       = 0    # power save : 0(off) or 1(on)
bss_max_idle_enable = 0 # 0(bss_max_idle off) or 1(bss_max_idle on)
bss_max_idle = 10       # number of keepalives (0 ~ 65535)
##################################################################################

def check(interface):
    ret = commands.getoutput("sudo wpa_cli -i" + interface + " status | grep ip_address=192.168.")
    return ret

def usage_print():
    print ("Usage: \n\tstart.py [sta_type] [security_mode] [country] [channel] [sniffer_mode]")
    print ("Argument:    \n\tsta_type      [0:STA   |  1:AP  |  2:SNIFFER  | 3:RELAY] \
            \n\tsecurity_mode [0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE] \
                         \n\tcountry       [KR:Korea] \
                         \n\t----------------------------------------------------------- \
                         \n\tchannel       [S1G Channel Number]   * Only for Sniffer \
                         \n\tsniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer")
    print ("Example:  \n\tOPEN mode STA for Korea                : ./start.py 0 0 KR \
                      \n\tSecurity mode AP for KR                : ./start.py 1 1 KR \
                      \n\tLocal Sniffer mode on CH 36 for Korea  : ./start.py 2 0 KR 36 0")
    print ("Note: \n\tsniffer_mode should be set as '1' when running sniffer on remote terminal")
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
    else:
        usage_print()

def checkCountry():
    country_list = ["KR"]
    if str(sys.argv[3]) in country_list:
        return
    else:
        print ("Only KR MIC channel frequencies are supported. Please select the country code KR.")
        exit()

def strSecurity():
    if int(sys.argv[2]) == 0:
        return 'OPEN'
    elif int(sys.argv[2]) == 1:
        return 'WPA2-PSK'
    elif int(sys.argv[2]) == 2:
        return 'WPA3-OWE'
    elif int(sys.argv[2]) == 3:
        return 'WPA3-SAE'
    else:
        usage_print()

def strSnifferMode():
    if int(sys.argv[5]) == 0:
        return 'LOCAL'
    elif int(sys.argv[5]) == 1:
        return 'REMOTE'
    else:
        usage_print()

def isNumber(s):
    try:
        float(s)
        return True
    except ValueError:
        return False

def argv_print():
    print ("------------------------------")
    print ("Model            : " + str(model))
    print ("STA Type         : " + strSTA())
    print ("Security Mode    : " + strSecurity())
    print ("Country Selected : " + str(sys.argv[3]))
    if strSTA() == 'SNIFFER':
        print ("Channel Selected : " + str(sys.argv[4]))
        print ("Sniffer Mode     : " + strSnifferMode())
    if int(fw_download) == 1:
        print ("Download FW      : " + fw_name)
    if int(bd_download) == 1:
        print ("Download Board Data      : " + bd_name)
    print ("TX Power         : " + str(txpwr_val))
    if int(bss_max_idle_enable) == 1 and strSTA() == 'AP':
        print ("bss_max_idle     : " + str(bss_max_idle))
    print ("------------------------------")

def copyConf():
    os.system("sudo /home/pi/nrc_pkg/sw/firmware/copy " + str(model) + " " + str(bd_name))
    os.system("/home/pi/nrc_pkg/script/conf/etc/ip_config.sh " + strSTA() + " " +  str(relay_type))

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
    os.system("sudo service dhcpcd start")

def stopDHCPCD():
    os.system("sudo service dhcpcd stop")

def startDNSMASQ():
    os.system("sudo service dnsmasq start")

def stopDNSMASQ():
    os.system("sudo service dnsmasq stop")

def addWLANInterface(interface):
    if strSTA() == 'RELAY' and interface == "wlan1":
        print "[*] Create wlan1 for concurrent mode"
        os.system("sudo iw dev wlan0 interface add wlan1 type managed");
        os.system("sudo ifconfig wlan1 up");
        time.sleep(3)

def run_common():
    if int(max_cpuclock) == 1:
        print "[*] Set Max CPU Clock on RPi"
        os.system("sudo /home/pi/nrc_pkg/script/conf/etc/clock_config.sh")

    print "[0] Clear"
    os.system("sudo killall -9 wpa_supplicant")
    os.system("sudo killall -9 hostapd")
    os.system("sudo killall -9 wireshark-gtk")
    os.system("sudo rmmod nrc")
    stopNAT()
    stopDHCPCD()
    stopDNSMASQ()
    time.sleep(1)

    print "[1] Copy"
    copyConf()

    if int(fw_download) == 1:
        fw_arg= "fw_name=" + fw_name
    else:
        fw_arg= ""

    if int(bd_download) == 1:
        bd_arg= " bd_name=" + bd_name
    else:
        bd_arg= ""

    if int(model) == 7291:
        alt_mode_arg = " alternate_mode=1"
    else:
        alt_mode_arg = ""

    if int(power_save) == 1:
        power_save_arg = " power_save=1"
    else:
        power_save_arg = " power_save=0"

    if int(bss_max_idle_enable) == 1 and strSTA() == 'AP':
        bss_max_idle_arg = " bss_max_idle=" + str(bss_max_idle)
    else:
        bss_max_idle_arg = ""

    insmod_arg = fw_arg + bd_arg + alt_mode_arg + power_save_arg + bss_max_idle_arg + " disable_cqm=" + str(cqm_off) + " hifspeed=" + str(hif_speed)
    print "[2] Loading module"
    print "sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko " + insmod_arg
    os.system("sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko " + insmod_arg + "")
    time.sleep(5)

    if strSTA() == 'RELAY' and int(relay_type) == 0:
        addWLANInterface('wlan1')

    ret = subprocess.call(["sudo", "ifconfig", "wlan0", "up"])
    if ret == 255:
        os.system('sudo rmmod nrc.ko')
        sys.exit()

    print "[3] Set tx power"
    os.system('python /home/pi/nrc_pkg/etc/python/shell.py run --cmd="nrf txpwr ' + str(txpwr_val) + '"')

    print "[4] Set aggregation number"
    if maxagg_num:
        os.system('python /home/pi/nrc_pkg/etc/python/test_send_addba.py 0')
        os.system('python /home/pi/nrc_pkg/etc/python/shell.py run --cmd="set maxagg 1 ' + str(maxagg_num) + '"')

    print "[5] Set guard interval"
    os.system('python /home/pi/nrc_pkg/etc/python/shell.py run --cmd="set gi ' + guard_int + '"')


    print "[*] Start DHCPCD and DNSMASQ"
    startDHCPCD()
    startDNSMASQ()

def run_sta(interface):
    country = str(sys.argv[3])

    if int(supplicant_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print "[6] Start wpa_supplicant on " + interface
    if strSecurity() == 'OPEN':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_open.conf " + debug + " &")
    elif strSecurity() == 'WPA2-PSK':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_wpa2.conf " + debug + " &")
    elif strSecurity() == 'WPA3-OWE':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_owe.conf " + debug + " &")
    elif strSecurity() == 'WPA3-SAE':
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/sta_halow_sae.conf " + debug + " &")
    time.sleep(3)

    print "[7] Connect and DHCP"
    ret = check(interface)
    while ret == '':
        print "Waiting for IP"
        time.sleep(5)
        ret = check(interface)

    print ret
    print "IP assigned. HaLow STA ready"
    print "--------------------------------------------------------------------"

def run_ap(interface):
    country = str(sys.argv[3])

    if int(hostapd_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print "[6] Start hostapd on " + interface
    if strSecurity() == 'OPEN':
        os.system("sed -i " + '"4s/.*/interface=' + interface + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/ap_halow_open.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_open.conf " + debug +" &")
    elif strSecurity() == 'WPA2-PSK':
        os.system("sed -i " + '"4s/.*/interface=' + interface + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/ap_halow_wpa2.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_wpa2.conf " + debug + "  &")
    elif strSecurity() == 'WPA3-OWE':
        os.system("sed -i " + '"4s/.*/interface=' + interface + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/ap_halow_owe.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_owe.conf " + debug + "  &")
    elif strSecurity() == 'WPA3-SAE':
        os.system("sed -i " + '"4s/.*/interface=' + interface + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/ap_halow_sae.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_sae.conf " + debug + "  &")
    time.sleep(3)

    print "[7] Start NAT"
    startNAT()

    #print "[8] Start DNSMASQ"
    #startDNSMASQ()

    time.sleep(3)
    print "[8] ifconfig"
    os.system('sudo ifconfig')
    print "HaLow AP ready"
    print "--------------------------------------------------------------------"

def run_sniffer():
    country = str(sys.argv[3])
    if country == 'EU':
        country = 'DE'

    print "[6] Setting Monitor Mode"
    time.sleep(3)
    os.system('sudo ifconfig wlan0 down; sudo iw dev wlan0 set type monitor; sudo ifconfig wlan0 up')
    print "[7] Setting Country: " + country
    os.system("sudo iw reg set " + country)
    print "[8] Setting Channel: " + str(sys.argv[4])
    os.system("sudo iw dev wlan0 set channel " + str(sys.argv[4]))
    time.sleep(3)
    print "[9] Start Sniffer"
    if strSnifferMode() == 'LOCAL':
        os.system('sudo wireshark-gtk -i wlan0 -k -S -l &')
    else:
        os.system('gksudo wireshark-gtk -i wlan0 -k -S -l &')
    print "HaLow SNIFFER ready"
    print "--------------------------------------------------------------------"

if __name__ == '__main__':
    if len(sys.argv) < 4 or not isNumber(sys.argv[1]) or not isNumber(sys.argv[2]):
        usage_print()
    elif strSTA() == 'SNIFFER' and len(sys.argv) < 6:
        usage_print()
    else:
        argv_print()

    checkCountry()

    print "NRC " + strSTA() + " setting for HaLow..."

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
    else:
        usage_print()

print "Done."
