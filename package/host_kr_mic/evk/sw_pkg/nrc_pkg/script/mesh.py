import os
import time
import subprocess
from start import check
from mesh_add_peer import addPeer

def isInterface(interface):
    if os.path.isdir("/sys/class/net/" + interface):
        return True
    else:
        return False

def getIpAddr(interface):
    cmd = "ip addr show " + interface + " | grep \"inet\\b\" | awk \'{print $2}\' | cut -d/ -f1"
    addr = subprocess.check_output(cmd, shell=True)
    return addr.strip()

def getNetAddr(interface):
    cmd = "ip addr show " + interface + " | grep \"inet\\b\" | awk \'{print $2}\' | cut -d. -f1,2,3"
    addr = subprocess.check_output(cmd, shell=True)
    return addr.strip()

def addBridgeMeshAP(wlan, mesh):
    print("bridge " + wlan + " with " + mesh)
    os.system("sudo brctl addbr br0")
    os.system("sudo brctl addif br0 " + wlan)
    os.system("sudo brctl addif br0 " + mesh)
    os.system("sudo ifconfig br0 " + getIpAddr(mesh))
    os.system("sudo ifconfig " + mesh + " 0.0.0.0")
    os.system("sudo ip route add default via " + getNetAddr("br0") + ".1 dev br0")

def removeBridgeMeshAP(wlan, mesh):
    if isInterface("br0"):
        os.system("sudo ifconfig br0 down > /dev/null 2>&1")
        os.system("sudo brctl delif br0 " + wlan + " > /dev/null 2>&1")
        os.system("sudo brctl delif br0 " + mesh + " > /dev/null 2>&1")
        os.system("sudo brctl delbr br0 > /dev/null 2>&1")

def startMeshNAT(intf0, intf1):
    os.system("sudo sysctl -w net.ipv4.ip_forward=1 >> /dev/null 2>&1")
    os.system("sudo sysctl -w net.ipv6.ip_forward=1 >> /dev/null 2>&1")
    os.system("sudo iptables -F")
    os.system("sudo iptables -t nat -F")
    os.system("sudo iptables -A FORWARD --in-interface " + intf0 + " -j ACCEPT >> /dev/null 2>&1")
    os.system("sudo iptables --table nat -A POSTROUTING --out-interface " + intf1 + " -j MASQUERADE >> /dev/null 2>&1")
    os.system("sudo iptables -A FORWARD --in-interface " + intf1 + " -j ACCEPT >> /dev/null 2>&1")
    os.system("sudo iptables --table nat -A POSTROUTING --out-interface " + intf0 + " -j MASQUERADE >> /dev/null 2>&1")

def stopMeshNAT():
    os.system("sudo sysctl -w net.ipv4.ip_forward=0 >> /dev/null 2>&1")
    os.system("sudo sysctl -w net.ipv6.ip_forward=0 >> /dev/null 2>&1")
    os.system("sudo iptables -F")
    os.system("sudo iptables -t nat -F")

def addMeshInterface(interface):
    print("[*] Create " + interface + " for concurrent mode")
    os.system("sudo iw dev wlan0 interface add " + interface + " type managed")
    os.system("sudo ifconfig " + interface + " up")

def run_mp(interface, country, security, debug, peermac, ip):
    removeBridgeMeshAP(interface, interface)
    os.system("sudo killall -9 wpa_supplicant")

    if int(debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print("[7] Start wpa_supplicant on " + interface)
    if security == 'OPEN':
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_open.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_open.conf ')
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_open.conf " + debug + " &")
    elif security == 'WPA3-SAE':
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_sae.conf " + debug + " &")
    time.sleep(1)

    if ip != 0:
        os.system("sudo ifconfig " + interface + " " + ip)

    if peermac != 0 and peermac != '00:00:00:00:00:00':
        print("[*] Manual Peering with " + peermac)
        addPeer(interface, peermac)

    print("[8] Connect and DHCP")
    ret = check(interface)
    while ret == '':
        print("Waiting for IP")
        time.sleep(1)
        ret = check(interface)

    os.system("sudo iw dev " + interface + " set mesh_param mesh_plink_timeout 30")

    print(ret)
    print("IP assigned. HaLow Mesh Point ready")
    print("--------------------------------------------------------------------")

def run_mpp(interface, country, security, debug, peermac, ip):
    removeBridgeMeshAP(interface, interface)
    os.system("sudo killall -9 wpa_supplicant")

    if int(debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print("[7] Start wpa_supplicant on " + interface)
    if security == 'OPEN':
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_open.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_open.conf ')
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_open.conf " + debug + " &")
    elif security == 'WPA3-SAE':
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        os.system("sudo wpa_supplicant -i" + interface + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_sae.conf " + debug + " &")
    time.sleep(1)

    print("[8] Start NAT")
    startMeshNAT("eth0", interface)

    ret = check(interface)
    while ret == '':
        print("Waiting for IP")
        time.sleep(1)
        ret = check(interface)

    print("[9] Add Route")
    os.system("sudo ip route add " + getNetAddr(interface) + ".0/24 via " + getIpAddr(interface) + " dev " + interface)
    os.system("route")

    if ip != 0:
        os.system("sudo ifconfig " + interface + " " + ip)
        os.system("sudo ip route add " + getNetAddr(interface) + ".0/24 via " + getIpAddr(interface) + " dev " + interface)

    if peermac != 0 and peermac != '00:00:00:00:00:00':
        print("[*] Manual Peering with " + peermac)
        addPeer(interface, peermac)

    os.system("sudo iw dev " + interface + " set mesh_param mesh_hwmp_rootmode 4")
    os.system("sudo iw dev " + interface + " set mesh_param mesh_gate_announcements 1")
    os.system("sudo iw dev " + interface + " set mesh_param mesh_plink_timeout 30")

    print("[10] ifconfig")
    os.system('sudo ifconfig')
    print("HaLow Mesh Portal ready")
    print("--------------------------------------------------------------------")

def run_map(wlan, mesh, country, security, debug, peermac, ip):
    removeBridgeMeshAP(wlan, mesh)
    os.system("sudo killall -9 wpa_supplicant")

    if int(debug) == 1:
        debug = '-dddd'
    else:
        debug = ''

    print("[7] Start wpa_supplicant on " + mesh)
    if security == 'OPEN':
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_open.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_open.conf ')
        os.system("sudo wpa_supplicant -i" + mesh + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_open.conf " + debug + " &")
    elif security == 'WPA3-SAE':
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        os.system("sudo wpa_supplicant -i" + mesh + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_sae.conf " + debug + " &")
    time.sleep(1)

    if ip != 0:
        os.system("sudo ifconfig " + mesh + " " + ip)

    if peermac != 0 and peermac != '00:00:00:00:00:00':
        print("[*] Manual Peering with " + peermac)
        addPeer(mesh, peermac)

    print("[8] Connect and DHCP for Mesh Network")
    ret = check(mesh)
    while ret == '':
        print("Waiting for IP")
        time.sleep(1)
        ret = check(mesh)

    os.system("sudo iw dev " + mesh + " set mesh_param mesh_hwmp_rootmode 4")
    os.system("sudo iw dev " + mesh + " set mesh_param mesh_gate_announcements 1")
    os.system("sudo iw dev " + mesh + " set mesh_param mesh_plink_timeout 30")

    print("[9] Start hostapd on " + wlan)
    if security == 'OPEN':
        os.system("sed -i " + '"4s/.*/interface=' + wlan + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/map_halow_open.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/map_halow_open.conf " + debug +" &")
    elif security == 'WPA2-PSK':
        os.system("sed -i " + '"4s/.*/interface=' + wlan + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/map_halow_wpa2.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/map_halow_wpa2.conf " + debug + "  &")
    elif security == 'WPA3-OWE':
        os.system("sed -i " + '"4s/.*/interface=' + wlan + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/map_halow_owe.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/map_halow_owe.conf " + debug + "  &")
    elif security == 'WPA3-SAE':
        os.system("sed -i " + '"4s/.*/interface=' + wlan + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/map_halow_sae.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/map_halow_sae.conf " + debug + "  &")
    elif security == 'WPA-PBC':
        os.system("sed -i " + '"4s/.*/interface=' + wlan + '/g"  /home/pi/nrc_pkg/script/conf/' + country + '/map_halow_pbc.conf ')
        os.system("sudo hostapd /home/pi/nrc_pkg/script/conf/" + country + "/map_halow_pbc.conf " + debug + "  &")
        time.sleep(1)
        os.system("sudo hostapd_cli wps_pbc")
    time.sleep(1)

    print("[10] Start NAT")
    startMeshNAT(wlan, mesh)

    print("[11] Add Bridge")
    addBridgeMeshAP(wlan, mesh)

    print("[12] ifconfig")
    os.system('sudo ifconfig')
    print("HaLow Mesh AP ready")
    print("--------------------------------------------------------------------")
