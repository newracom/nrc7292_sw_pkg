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
concurrent  = 0         # 0(Concurrent Mode off) or 1(Coucurrent Mode on)
interface_11ah   = 'wlan1'  # 11ah driver interface : 'wlan0' or 'wlan1'
supplicant_debug = 0    # WPA Supplicant debug option : 0(off) or 1(on)
hostapd_debug    = 0    # Hostapd debug option    : 0(off) or 1(on)
max_cpuclock     = 1    # RPi Max CPU Clock : 0(off) or 1(on)
# Optional Configuration for dual-band (11N + 11AH)
enable_11n        = 1        # Enable Dual-Band (11ah + 11n)
role_11n          = 'AP'     # Role of 11N ('AP' or 'STA')
interface_11n     = 'wlan0'  # 11n driver interface : 'wlan0'
disable_NAT       = 0   # 1(NAT disable) 0(NAT enable)
power_save        = 0   # power save : 0(off) or 1(on)
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
    print ("Interface        : " + interface_11ah)
    print ("TX Power         : " + str(txpwr_val))
    if int(bss_max_idle_enable) == 1 and strSTA() == 'AP':
        print ("bss_max_idle     : " + str(bss_max_idle))
    print ("------------------------------")
    if int(enable_11n):
        print ("11n Interface    : " + interface_11n)
        print ("11n Role         : " + role_11n)
        print ("------------------------------")

def copyConf():
    os.system("sudo /home/pi/nrc_pkg/sw/firmware/copy " + str(model) + " " + str(bd_name))
    if strSTA() == 'AP':
        os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dhcpcd_ap.conf /etc/dhcpcd.conf")
        os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dnsmasq/dnsmasq_ap.conf /etc/dnsmasq.conf")
        if int(enable_11n) and role_11n == 'STA':
            print ("copy dhcpcd_ap_sta.conf &  dnsmasq_ap_sta.conf")
            os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dhcpcd_ap_sta.conf /etc/dhcpcd.conf")
            os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dnsmasq/dnsmasq_ap_sta.conf /etc/dnsmasq.conf")
    else:
        os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dhcpcd_sta.conf /etc/dhcpcd.conf")
        if int(enable_11n) and role_11n == 'AP':
            print ("copy dhcpcd_sta_ap.conf & dnsmasq_sta_ap.conf")
            os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dhcpcd_sta_ap.conf /etc/dhcpcd.conf")
            os.system("sudo cp /home/pi/nrc_pkg/etc/dhcpcd/dnsmasq/dnsmasq_sta_ap.conf /etc/dnsmasq.conf")


def startNAT(in_interface, out_interface):
    os.system('sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"')
    if int(enable_11n) and int(disable_NAT):
        print ("Skip setting NAT table ")
        return
    print ("INPUT:" + in_interface + " OUTPUT:" + out_interface +"" )
    os.system("sudo iptables -t nat -A POSTROUTING -o " + out_interface + " -j MASQUERADE")
    os.system("sudo iptables -A FORWARD -i " + out_interface + " -o " + in_interface + " -m state --state RELATED,ESTABLISHED -j ACCEPT")
    os.system("sudo iptables -A FORWARD -i " + in_interface + " -o " + out_interface + " -j ACCEPT")
    time.sleep(1)

def startDNSMASQ():
    print ("[*] Start dnsmasq service")
    os.system("sudo service dnsmasq start")
    time.sleep(1)

def run_common():
    if int(max_cpuclock) == 1:
        print "[*] Set Max CPU Clock on RPi"
        os.system("sudo /home/pi/nrc_pkg/script/fix_clock.sh")

    print "[0] Clear"
    os.system("sudo killall -9 wpa_supplicant")
    os.system("sudo killall -9 hostapd")
    os.system("sudo killall -9 wireshark-gtk")
    os.system("sudo rmmod nrc")
    os.system("sudo service dnsmasq stop")
    time.sleep(1)

    #if int(enable_11n):
    print "11n inteface (" + interface_11n + ") down"
    os.system("sudo ifconfig " + interface_11n + " down");

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

    if int(concurrent) == 1 and strSTA() == 'AP':
        print "[2-1] Create wlan2 for concurrent mode"
        os.system("sudo iw dev wlan1 interface add wlan2 type managed");
        os.system("sudo ifconfig wlan1 up");

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
        time.sleep(1)


    print "[5] Set guard interval"
    os.system('python /home/pi/nrc_pkg/etc/python/shell.py run --cmd="set gi ' + guard_int + '"')
    time.sleep(1)

def run_sta(interface):
    country = str(sys.argv[3])

    # debugging option of wpa_supplicant
    if int(supplicant_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print "[6] Start wpa_supplicant"
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

    print "[8] ifconfig"
    os.system('sudo ifconfig')

def run_ap(interface):
    country = str(sys.argv[3])

    # debugging option of hostapd
    if int(hostapd_debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print "[6] Start hostapd"
    if strSecurity() == 'OPEN':
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_open.conf " + debug +" &")
    elif strSecurity() == 'WPA2-PSK':
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_wpa2.conf " + debug + "  &")
    elif strSecurity() == 'WPA3-OWE':
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_owe.conf " + debug + "  &")
    elif strSecurity() == 'WPA3-SAE':
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/ap_halow_sae.conf " + debug + "  &")
    time.sleep(3)

    if not (int(enable_11n) and role_11n == 'STA'):
        print "[7] Setting NAT"
        startNAT(interface, "eth0")

    print "[8] ifconfig"
    os.system('sudo ifconfig')
    print "HaLow AP ready"

def run_sniffer(interface):
    print "[6] Setting Monitor Mode"
    os.system('sudo ifconfig ' + interface + ' down; sudo iw dev ' + interface + ' set type monitor; sudo ifconfig ' + interface + ' up')
    print "[7] Setting Country: " + str(sys.argv[3])
    os.system("sudo iw reg set " + str(sys.argv[3]))
    print "[8] Setting Channel: " + str(sys.argv[4])
    os.system("sudo iw dev " + interface + " set channel " + str(sys.argv[4]))
    time.sleep(3)
    print "[9] Start Sniffer"
    if strSnifferMode() == 'LOCAL':
        os.system('sudo wireshark-gtk -i' + interface + ' -k -S -l &')
    else:
        os.system('gksudo wireshark-gtk -i' + interface + ' -k -S -l &')

def run_11n(interface):
    print "[*] Enable 11n interface(" + interface + ")"
    os.system("sudo ifconfig " +  interface + " up")
    if role_11n == 'AP':
        print "[*] Start 11n AP"
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/11N/hostapd_" + strSTA() + ".conf &")
        time.sleep(3)
        if strSTA() == 'STA':
            #set default gw to wlan1(11ah)
            os.system("sudo route add default gw 192.168.200.1 dev wlan1")
            # 11n_STA <-> [11n_AP(wlan0) + 11ah_STA(wlan1)*] <-> 11ah AP <-> Internet
            print "[*] Setting NAT"
            startNAT(interface, interface_11ah)
    else:
        print "[*] Start 11n STA"
        os.system("sudo wpa_supplicant -i " + interface + " -c /home/pi/nrc_pkg/script/conf/11N/wpa_supplicant.conf &")
        time.sleep(3)
        if strSTA() == 'AP':
            os.system('sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"')
            # Internet <-> 11n_AP <-> [*11n_STA(wlan0) + 11ah_AP(wlan1)] <-> 11ah STA
            print "[*] Setting NAT"
            startNAT(interface_11ah, interface)

#main funtion
if __name__ == '__main__':
    if len(sys.argv) < 4 or not isNumber(sys.argv[1]) or not isNumber(sys.argv[2]):
        usage_print()
    elif strSTA() == 'SNIFFER' and len(sys.argv) < 6:
        usage_print()
    else:
        argv_print()

    checkCountry()

    print "NRC " + strSTA() + " setting for HaLow..."

    if int(enable_11n):
        if strSTA() == 'STA' and role_11n == 'STA':
            print "Dual-band for (11AH STA and 11N STA) is not supported yet"
            exit()
        interface_11ah = 'wlan1'

    run_common()

    if strSTA() == 'STA':
        run_sta(interface_11ah)
    elif strSTA() == 'AP':
        run_ap(interface_11ah)
    elif strSTA() == 'SNIFFER':
        run_sniffer(interface_11ah)
    else:
        usage_print()

    if int(enable_11n):
        run_11n(interface_11n)

    if int(concurrent) == 1 and strSTA() == 'AP':
        run_sta('wlan2')

    if strSTA() == 'AP' or (int(enable_11n) and role_11n == 'AP'):
       startDNSMASQ()

print "Done."
