#!/bin/bash

# Default Channel
CHANNEL=153
# Default IP Addresses for VIF 0,1
IP_ADDR0=192.168.222.1
IP_ADDR1=192.168.30.1
# Default SSID for VIF 0,1
# follow hostname without 'nrc-'
# e.g.) halow-b0, halow-b1
SSID0=$(cat /etc/hostname | sed 's/nrc-//g')
SSID1=$(cat /etc/hostname | sed 's/nrc-//g')
# Default passphrasees for VIF 0, 1
PASSWORD0=12345678
PASSWORD1=12345678
# TODO: Use DHCP Server
USE_DHCP_SERVER=0
# Use NAT
USE_NAT=0
# USE Concurrent Mode
USE_CONC=0
# User Mesh Point address to connect
USE_MP_ADDR=0
TREE=2
# Default Mesh Point address to connect
MP_ADDR=

PROGNAME=$(basename $0)
GETOPT_ARGS=$(getopt -o hc:s:d:p:o:m:a:b:p:k:t: -l "help","ssid0","ssid1" -n "$PROGNAME" -- "$@")
eval set -- "$GETOPT_ARGS"


usage() {
	echo $(basename $0) "<mode> <passphrase>"
	echo "  -c <channel>	Channel number (default: 36)"
	echo "  -s <ssid>		ssid for VIF 0"
	echo "  -d <ssid>		ssid for VIF 1"
	echo "  -p <passphrase>		passphrase for VIF 0"
	echo "  -o <passphrase>		passphrase for VIF 1"
	echo "  -a <IP address>		IP Address for VIF 0"
	echo "  -b <IP address>		IP Address for VIF 1"
}


#trap "Ctrl + C" and call ctl_c()
trap ctrl_c INT


cleanup() {
	echo "Clean up"
	#break bridge
	if is_interface br0; then
		sudo ifconfig br0 down > /dev/null 2>&1
		sudo brctl delif br0 wlan0  > /dev/null 2>&1
		sudo brctl delif br0 wlan1 > /dev/null 2>&1
		sudo brctl delbr br0 > /dev/null 2>&1
	fi
	sudo killall -q wpa_supplicant
	sudo killall -q hostapd
	sudo rmmod nrc > /dev/null 2>&1
}


#cleanup and exit
ctrl_c() {
	cleanup
	exit 1;
}


is_interface() {
	[[ -z "$1" ]] && return 1
	[[ -d "/sys/class/net/${1}" ]]
}


get_ip_addr() {
	local INTF_NAME=$1
	echo $(ip addr show ${INTF_NAME} | grep "inet\b" | awk '{print $2}' | cut -d/ -f1)
}


get_network_addr() {
	# Here, it assume subnet address is always /24
	echo $(ip addr show ${1} | grep "inet\b" | awk '{print $2}' | cut -d. -f1,2,3)
}

get_gw_addr() {
	local NW_ADDR=$(get_network_addr $1)
	echo  ${NW_ADDR}".1"
}

# taken from iw/util.c
ieee80211_channel_to_frequency() {
# consider only 5GHZ
	local CH=$1
	if [[ $CH -ge 182 && $CH -le 196 ]]; then
		echo $(( 4000 + $CH * 5 ))
	else
		echo $(( 5000 + $CH * 5 ))
	fi
}


ieee80211_frequency_to_channel() {
	local FREQ=$1
	if [[ $FREQ -eq 2484 ]]; then
		echo 14
	elif [[ $FREQ -lt 2484 ]]; then
		echo $(( ($FREQ - 2407) / 5 ))
	elif [[ $FREQ -ge 4910 && $FREQ -le 4980 ]]; then
		echo $(( ($FREQ - 4000) / 5 ))
	elif [[ $FREQ -le 45000 ]]; then
		echo $(( ($FREQ - 5000) / 5 ))
	elif [[ $FREQ -ge 58320 && $FREQ -le 64800 ]]; then
		echo $(( ($FREQ - 56160) / 2160 ))
	else
		echo 0
	fi
}

wait_for_intf() {
	local TIMEOUT_MS=10000
	while [[ 0 -ne $TIMEOUT_MS ]]; do
		if is_interface $1; then
			echo FOUND!!!
			return
		fi
		sleep 0.1
		TIMEOUT_MS=$[$TIMEOUT_MS-100]
	done
}

enable_drv() {
	local ifname=wlan0
	sudo cp /home/pi/nrc_pkg/sw/firmware/nrc7292_cspi.bin /lib/firmware/uni_s1g.bin
	sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko fw_name=uni_s1g.bin hifspeed=10000000
	wait_for_intf $ifname
        sleep 5
        sudo ifconfig $ifname up
        echo set txpwr 17
        /home/pi/nrc_pkg/script/cli_app set txpwr 17
}

# Enable conc interface
enable_conc_intf() {
	sudo iw dev wlan0 interface add wlan1 type managed
}

# enable monitor interface
enable_monitor() {
	sudo iw phy nrc80211 interface add mon0 type monitor
	sudo ifconfig mon0 up
}

set_nat() {
	sudo sysctl net.ipv4.ip_forward=1 >> /dev/null 2>&1
	sudo sysctl net.ipv6.ip_forward=1 >> /dev/null 2>&1
	sudo iptables -F
	sudo iptables -t nat -A POSTROUTING -o wlan1 -j MASQUERADE >> /dev/null 2>&1
	sudo iptables -A FORWARD -i wlan0 -o wlan1 -m state --state RELATED,ESTABLISHED -j ACCEPT >> /dev/null 2>&1
	sudo iptables -A FORWARD -i wlan1 -o wlan1 -j ACCEPT >> /dev/null 2>&1
	sudo iptables -A INPUT -j ACCEPT >> /dev/null 2>&1
	sudo iptables -A OUTPUT -j ACCEPT >> /dev/null 2>&1
}

set_forwarding() {
	local AP_NW=$(get_network_addr $1)
	local STA_NW=$(get_network_addr $2)
	local GW=$(get_gw_addr $STA)
	sudo route add -net ${AP_NW}.0 netmask 255.255.255.0 gw ${STA_NW}.1
	#sudo route add -net ${STA_NW}.0 netmask 255.255.255.0 gw ${STA_NW}.1
}

set_bridge() {
	sudo iw dev wlan0 set 4addr on > /dev/null 2>&1
	sudo iw dev wlan1 set 4addr on > /dev/null 2>&1
	sudo ifconfig wlan0 0.0.0.0
	sudo ifconfig wlan1 0.0.0.0
	sudo brctl addbr br0
	sudo brctl addif br0 wlan0
	sudo brctl addif br0 wlan1
	sudo ifconfig br0 $IP_ADDR0
}

write_config_mp() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/wpa_supplicant
country=US
network={
	ssid="${SSID0}"
	mode=5
	frequency=${FREQ}
	#key_mgmt=NONE
	key_mgmt=SAE
	psk="${PASSWORD0}"
	#no_auto_peer=1
}
p2p_disabled=1
EOF
}

write_config_ap() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/hostapd
country_code=US
ssid=$SSID0
hw_mode=a
beacon_int=100
channel=${CHANNEL}
ieee80211n=1
interface=wlan0
ap_max_inactivity=16779
wmm_enabled=1
wpa=2
wpa_passphrase=${PASSWORD0}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=CCMP
EOF
}

write_config_sta() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/wpa_supplicant
country=US

network={
	ssid="${SSID1}"
	psk="${PASSWORD1}"
}
p2p_disabled=1
EOF
}

run_mp() {
	cleanup
	enable_drv
	#enable_conc_intf
	write_config_mp run0.conf
	sudo chmod 755 run0.conf
	sudo wpa_supplicant -iwlan0 -c run0.conf -dddd &
	sleep 10
	if [[ $USE_MP_ADDR -eq 1 ]]; then
		echo wpa_cli -i wlan0 mesh_peer_add ${MP_ADDR}
		sudo wpa_cli -i wlan0 mesh_peer_add ${MP_ADDR}
	fi
	sudo ifconfig wlan0 $IP_ADDR0
}

run_map() {
	cleanup
	enable_drv
	enable_conc_intf
	write_config_mp run0.conf
	sudo wpa_supplicant -iwlan0 -c run0.conf -ddd &
	write_config_ap run1.conf
	sudo hostapd run1.conf -ddd &
	#sleep 4
	#set_bridge
}


do_work() {
if [ "$MODE" == "wds" ]; then
	echo "wds"
elif [ "$MODE" == "tree" ]; then
	sudo hostapd_cli -iwlan0 status
	sudo wpa_cli -iwlan1 status

elif [ "$MODE" == "mp" ]; then
	if [[ $USE_MP_ADDR -eq 1 ]]; then
		sudo wpa_cli -i wlan0 mesh_peer_add ${MP_ADDR} >> /dev/null 2>&1
	fi
else
	echo "..."
fi
}


while :; do
	case "$1" in
		-h|--help)
			usage
			exit 0
			;;
		-c)
		shift
		CHANNEL="$1"
		FREQ=$(ieee80211_channel_to_frequency ${CHANNEL})
		shift
		;;
		-s|--ssid0)
		shift
		SSID0="$1"
		shift
		;;
		-p|--password0)
		shift
		PASSWORD0="$1"
		shift
		;;
		-d|--ssid1)
		shift
		USE_CONC=1
		SSID1="$1"
		shift
		;;
		-o|--password1)
		shift
		PASSWORD1="$1"
		shift
		;;
		-m|--mode)
		shift
		MODE="$1"
		shift
		;;
		-a|--ipaddr0)
		shift
		IP_ADDR0="$1"
		shift
		;;
		-b|--ipaddr1)
		shift
		IP_ADDR1="$1"
		shift
		;;
		-k|--mpaddr)
		shift
		USE_MP_ADDR=1
		MP_ADDR="$1"
		shift
		;;
		-t|--tree)
		shift
		TREE="$1"
		shift
		;;
		--)
		shift
		break
		;;
	esac
done

if [ "$MODE" == "map" ]; then
	run_map
elif [ "$MODE" == "mp" ]; then
	run_mp
else
	exit 1
fi


while :; do do_work; sleep 2; done
