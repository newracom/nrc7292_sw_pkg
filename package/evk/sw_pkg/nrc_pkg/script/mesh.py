import os
import time
import subprocess
from start import check
from mesh_add_peer import addPeer, checkPeer

def isInterface(interface):
    if os.path.isdir("/sys/class/net/" + interface):
        return True
    else:
        return False

def getIpAddr(interface):
    cmd = "ip addr show " + interface + " | grep \"inet\\b\" | awk \'{print $2}\' | cut -d/ -f1"
    addr = subprocess.check_output(cmd, shell=True)
    return addr.strip().decode('utf-8')

def getNetAddr(interface):
    cmd = "ip addr show " + interface + " | grep \"inet\\b\" | awk \'{print $2}\' | cut -d. -f1,2,3"
    addr = subprocess.check_output(cmd, shell=True)
    return addr.strip().decode('utf-8')

def addBridgeMeshAP(wlan, mesh):
    print("bridge " + wlan + " with " + mesh)
    os.system("sudo brctl addbr br0")
    os.system("sudo brctl addif br0 " + wlan)
    os.system("sudo brctl addif br0 " + mesh)
    print("[11] Connect and DHCP for Mesh Network")
    os.system("sudo dhcpcd -n br0")
    ret = check("br0")
    rebind = 1
    while ret == '':
        if rebind % 15 == 0:
            rebind = 1
            os.system("sudo dhcpcd -n br0")
        else:
            rebind = rebind + 1
        print("Waiting for IP")
        time.sleep(1)
        ret = check("br0")
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

def run_mp(interface, country, security, debug, peermac, ip, batman):
    removeBridgeMeshAP(interface, interface)
    os.system("sudo killall -9 wpa_supplicant")
    os.system("sudo batctl if del wlan0 > /dev/null 2>&1")
    os.system("sudo batctl if del mesh0 > /dev/null 2>&1")
    os.system("sudo ifconfig bat0 down > /dev/null 2>&1")

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

    if batman != 0:
        os.system("sudo iw dev " + interface + " set mesh_param mesh_fwding 0")
        os.system("sudo batctl -m " + batman + " if add " + interface)
        os.system("sudo ifconfig " + batman + " up")
        os.system("sudo ifconfig " + interface + " mtu 1532")
        intf = batman
    else:
        os.system("sudo iw dev " + interface + " set mesh_param mesh_hwmp_rootmode 2")
        os.system("sudo iw dev " + interface + " set mesh_param mesh_hwmp_root_interval 1000")
        intf = interface

    os.system("sudo iw dev " + interface + " set mesh_param mesh_plink_timeout 0")

    if ip != 0:
        os.system("sudo ifconfig " + intf + " " + ip)

    if peermac != 0 and peermac != '00:00:00:00:00:00':
        print("[*] Manual Peering with " + peermac)
        if batman != 0:
            addPeer(interface, peermac, 'batman')
        else:
            addPeer(interface, peermac, '')

    print("[8] Connect and DHCP")
    os.system("sudo dhcpcd -n " + intf)
    ret = check(intf)
    rebind = 1
    while ret == '':
        if rebind % 15 == 0:
            rebind = 1
            os.system("sudo dhcpcd -n " + intf)
        else:
            rebind = rebind + 1
        print("Waiting for IP")
        time.sleep(1)
        ret = check(intf)

    print(ret)
    print("IP assigned. HaLow Mesh Point ready")
    print("--------------------------------------------------------------------")
    if peermac != 0 and peermac != '00:00:00:00:00:00':
        if batman != 0:
            checkPeer(interface, peermac, 'batman')
        else:
            checkPeer(interface, peermac, '')

def run_mpp(interface, country, security, debug, peermac, ip, batman):
    removeBridgeMeshAP(interface, interface)
    os.system("sudo killall -9 wpa_supplicant")
    os.system("sudo batctl if del wlan0 > /dev/null 2>&1")
    os.system("sudo batctl if del mesh0 > /dev/null 2>&1")
    os.system("sudo ifconfig bat0 down > /dev/null 2>&1")

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

    if ip != 'nodhcp':
        print("[8] Start NAT")
        startMeshNAT("eth0", interface)

    if batman != 0:
        os.system("sudo iw dev " + interface + " set mesh_param mesh_fwding 0")
        os.system("sudo batctl -m " + batman + " if add " + interface)
        os.system("sudo batctl -m " + batman + " if add " + "eth0")
        os.system("sudo ifconfig bat0 up")
        os.system("sudo ifconfig " + interface + " mtu 1532")
        intf = batman
    else:
        os.system("sudo iw dev " + interface + " set mesh_param mesh_hwmp_rootmode 2")
        os.system("sudo iw dev " + interface + " set mesh_param mesh_hwmp_root_interval 1000")
        os.system("sudo iw dev " + interface + " set mesh_param mesh_gate_announcements 1")
        if ip == 'nodhcp':
            os.system("sudo ifconfig " + interface + " 0.0.0.0")
            os.system("sudo brctl addbr br0")
            os.system("sudo brctl addif br0 eth0")
            os.system("sudo brctl addif br0 " + interface)
            os.system("sudo ifconfig br0 up")
            intf = "br0"
        else:
            intf = interface

    os.system("sudo iw dev " + interface + " set mesh_param mesh_plink_timeout 0")

    ret = check(intf)
    while ret == '':
        print("Waiting for IP")
        time.sleep(1)
        ret = check(intf)

    print("[9] Add Route")
    if ip == 'nodhcp':
        os.system("sudo ip route add " + getNetAddr(intf) + ".0/24 via " + getIpAddr(intf) + " dev " + intf)
        os.system("sudo ip route add " + getNetAddr(intf) + ".0/24 via " + getNetAddr(intf) + ".1 dev " + intf)
    else:
        os.system("sudo ip route add " + getNetAddr(intf) + ".0/24 via " + getIpAddr(intf) + " dev " + intf)
    os.system("route")

    if ip != 0 and ip != 'nodhcp':
        os.system("sudo ifconfig " + intf + " " + ip)
        os.system("sudo ip route add " + getNetAddr(intf) + ".0/24 via " + getIpAddr(intf) + " dev " + intf)

    if peermac != 0 and peermac != '00:00:00:00:00:00':
        print("[*] Manual Peering with " + peermac)
        if batman != 0:
            addPeer(interface, peermac, 'batman')
        else:
            addPeer(interface, peermac, '')

    print("[10] ifconfig")
    os.system('sudo ifconfig')
    print("HaLow Mesh Portal ready")
    print("--------------------------------------------------------------------")
    if peermac != 0 and peermac != '00:00:00:00:00:00':
        if batman != 0:
            checkPeer(interface, peermac, 'batman')
        else:
            checkPeer(interface, peermac, '')

def run_map(wlan, mesh, country, security, debug, peermac, ip, batman):
    removeBridgeMeshAP(wlan, mesh)
    os.system("sudo killall -9 wpa_supplicant")
    os.system("sudo batctl if del wlan0 > /dev/null 2>&1")
    os.system("sudo batctl if del mesh0 > /dev/null 2>&1")
    os.system("sudo ifconfig bat0 down > /dev/null 2>&1")

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
    else:
        if peermac != 0:
            os.system("sed -i " + '"s/#no_auto_peer=1/no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        else:
            os.system("sed -i " + '"s/ no_auto_peer=1/ #no_auto_peer=1/g"  /home/pi/nrc_pkg/script/conf/' + country + '/mp_halow_sae.conf ')
        os.system("sudo wpa_supplicant -i" + mesh + " -c /home/pi/nrc_pkg/script/conf/" + country + "/mp_halow_sae.conf " + debug + " &")
    time.sleep(1)

    if batman != 0:
        os.system("sudo iw dev " + mesh + " set mesh_param mesh_fwding 0")
        os.system("sudo batctl -m " + batman + " if add " + mesh)
        os.system("sudo ifconfig " + batman + " up")
        os.system("sudo ifconfig " + mesh + " mtu 1532")
        intf = batman
    else:
        os.system("sudo iw dev " + mesh + " set mesh_param mesh_hwmp_rootmode 2")
        os.system("sudo iw dev " + mesh + " set mesh_param mesh_hwmp_root_interval 1000")
        intf = mesh

    os.system("sudo iw dev " + mesh + " set mesh_param mesh_plink_timeout 0")

    if ip != 0:
        os.system("sudo ifconfig " + intf + " " + ip)

    if peermac != 0 and peermac != '00:00:00:00:00:00':
        print("[*] Manual Peering with " + peermac)
        if batman != 0:
            addPeer(mesh, peermac, 'batman')
        else:
            addPeer(mesh, peermac, '')

    print("[8] Start hostapd on " + wlan)
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

    print("[9] Start NAT")
    startMeshNAT(wlan, intf)

    print("[10] Add Bridge")
    addBridgeMeshAP(wlan, intf)

    print("[12] ifconfig")
    os.system('sudo ifconfig')
    print("HaLow Mesh AP ready")
    print("--------------------------------------------------------------------")
    if peermac != 0 and peermac != '00:00:00:00:00:00':
        if batman != 0:
            checkPeer(mesh, peermac, 'batman')
        else:
            checkPeer(mesh, peermac, '')
