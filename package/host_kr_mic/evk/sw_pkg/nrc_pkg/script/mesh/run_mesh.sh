#!/bin/bash
#
# MPP: ./run_mesh.sh -m mpp -c 161 -s halow_mesh -p 12345678
# MP : ./run_mesh.sh -m mp -c 161 -s halow_mesh -p 12345678
# MAP: ./run_mesh.sh -m map -c 161 -s halow_mesh -p 12345678
#
# Default Channel
CHANNEL=161
# Default IP Addresses for VIF 0,1
IP_ADDR0=192.168.222.1
IP_ADDR1=192.168.200.1
# Default SSID for VIF 0,1
# follow hostname without 'nrc-'
# e.g.) halow-b0, halow-b1
SSID0=$(cat /etc/hostname | sed 's/nrc-//g')
SSID1=$(cat /etc/hostname | sed 's/nrc-//g')
# Default passphrasees for VIF 0, 1
PASSWORD0=12345678
PASSWORD1=12345678
# TODO: Use DHCP Server
USE_DHCP_SERVER=1
# Use NAT
USE_NAT=0
# USE Concurrent Mode
USE_CONC=0
# User Mesh Point address to connect
USE_MP_ADDR=0
TREE=2
# Default Mesh Point address to connect
MP_ADDR=
# Default Mesh Key Mgmt
KEY_MGMT="SAE"
# Country
COUNTRY="US"

PROGNAME=$(basename $0)
GETOPT_ARGS=$(getopt -o hc:s:d:p:o:m:a:b:p:k:t:q:u:n: -l "help","ssid0","ssid1" -n "$PROGNAME" -- "$@")
eval set -- "$GETOPT_ARGS"


usage() {
	echo $(basename $0) "<mode> <passphrase>"
	echo "  -c <channel>	Channel number (default: 161)"
	echo "  -s <ssid>		ssid for VIF 0"
	echo "  -d <ssid>		ssid for VIF 1"
	echo "  -p <passphrase>		passphrase for VIF 0"
	echo "  -o <passphrase>		passphrase for VIF 1"
	echo "  -a <IP address>		IP Address for VIF 0"
	echo "  -b <IP address>		IP Address for VIF 1"
	echo "  -u <NONE|SAE|WPA2|OWE>	key mgmt"
	echo "  -q <1|0>		use dhcp server"
	echo "  -n <US|KR...>	country"
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
	if [[ $USE_DHCP_SERVER == 1 ]]; then
		sudo killall -q dhcpcd
		sudo killall -q dnsmasq
		sudo sysctl -w net.ipv4.ip_forward=0 >> /dev/null 2>&1
		sudo sysctl -w net.ipv6.ip_forward=0 >> /dev/null 2>&1
		sudo iptables -F
		sudo iptables -t nat -F
	fi
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

get_bc_addr() {
	local NW_ADDR=$(get_network_addr $1)
	echo  ${NW_ADDR}".255"
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
	local TIMEOUT_MS=15000
	while [[ 0 -ne $TIMEOUT_MS ]]; do
		if is_interface $1; then
			echo FOUND!!!
			return
		fi
		sleep 0.1
		TIMEOUT_MS=$[$TIMEOUT_MS-100]
	done
}

wait_for_ip() {
	local TIMEOUT_S=10
	while [[ 0 -ne $TIMEOUT_S ]]; do
		if [[ $(echo $(get_network_addr ${1}) | cut -d. -f1,2) == "192.168" ]]; then
			echo ${1}:$(get_ip_addr ${1})
			return
		fi
		sleep 0.1
		#TIMEOUT_MS=$[$TIMEOUT_S-1]
	done
}

wait_for_peer() {
	local FOUND=0
	while [[ $FOUND -eq 0 ]]; do
		echo "Wait for Peer: ${2}"
		peer=$(sudo wpa_cli -i ${1} list_sta 2> /dev/null)
		for i in ${peer[*]}
		do
			if [[ $i == ${2} ]]; then
				FOUND=1
				echo "$i FOUND!"
				break
			fi
		done
		sleep 1
	done
}

add_peer() {
	local CONN=0
	wait_for_peer ${1} ${2}
	echo "Wait for adding new STA: ${2}"
	while [[ $CONN -eq 0 ]]; do
		sta=$(sudo iw dev ${1} mpath get ${2} 2> /dev/null | cut -d ' ' -f1)
		if [[ $sta == ${2} ]]; then
			CONN=1
			echo "$sta CONNECTED!"
			break
		else
			sudo wpa_cli -i ${1} mesh_peer_add ${2} 2> /dev/null
		fi
		sleep 1
	done
}

enable_drv() {
	local ifname=${1}
	sudo cp /home/pi/nrc_pkg/sw/firmware/nrc7292_cspi.bin /lib/firmware/uni_s1g.bin
	sudo insmod /home/pi/nrc_pkg/sw/driver/nrc.ko fw_name=uni_s1g.bin hifspeed=20000000 power_save=0 sw_enc=1
	wait_for_intf $ifname
}

# Enable conc interface
enable_conc_intf() {
	sudo iw dev ${1} interface add ${2} type managed
	sudo ifconfig ${2} up
	sudo ifconfig ${1} up
}

# enable monitor interface
enable_monitor() {
	sudo iw phy nrc80211 interface add mon0 type monitor
	sudo ifconfig mon0 up
}

set_nat() {
	sudo sysctl -w net.ipv4.ip_forward=1 >> /dev/null 2>&1
	sudo sysctl -w net.ipv6.ip_forward=1 >> /dev/null 2>&1
	sudo iptables -F
	sudo iptables -t nat -F
	sudo iptables -A FORWARD --in-interface ${1} -j ACCEPT >> /dev/null 2>&1
	sudo iptables --table nat -A POSTROUTING --out-interface ${2} -j MASQUERADE >> /dev/null 2>&1
	sudo iptables -A FORWARD --in-interface ${2} -j ACCEPT >> /dev/null 2>&1
	sudo iptables --table nat -A POSTROUTING --out-interface ${1} -j MASQUERADE >> /dev/null 2>&1
}

set_forwarding() {
	local AP_NW=$(get_network_addr $1)
	local STA_NW=$(get_network_addr $2)
	local GW=$(get_gw_addr $STA)
	sudo route add -net ${AP_NW}.0 netmask 255.255.255.0 gw ${STA_NW}.1
	#sudo route add -net ${STA_NW}.0 netmask 255.255.255.0 gw ${STA_NW}.1
}

set_bridge_map() {
	echo bridge ${1} to ${2}
	sudo brctl addbr br0
	sudo brctl addif br0 ${1}
	sudo brctl addif br0 ${2}
	sudo ifconfig br0 up
}

set_bridge_ap() {
	echo bridge ${1} to ${2}
	sudo iw dev ${1} set 4addr on > /dev/null 2>&1
	sudo iw dev ${2} set 4addr on > /dev/null 2>&1
	sudo ifconfig ${1} 0.0.0.0
	sudo ifconfig ${2} 0.0.0.0
	sudo brctl addbr br0
	sudo brctl addif br0 ${1}
	sudo brctl addif br0 ${2}
	sudo ifconfig br0 $IP_ADDR1
}

write_config_mp() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/wpa_supplicant
country=${COUNTRY}
network={
	ssid="${SSID0}"
	mode=5
	frequency=${FREQ}
	scan_freq=${FREQ}
	freq_list=${FREQ}
	key_mgmt=${KEY_MGMT}
	psk="${PASSWORD0}"
	#no_auto_peer=1
	beacon_int=100
	dot11MeshRetryTimeout=1000
	dot11MeshHoldingTimeout=400
	dot11MeshMaxRetries=4
	mesh_rssi_threshold=0
}
p2p_disabled=1
EOF
}

write_config_mp_sec() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/wpa_supplicant
country=${COUNTRY}
network={
	ssid="${SSID0}"
	mode=5
	frequency=${FREQ}
	scan_freq=${FREQ}
	freq_list=${FREQ}
	key_mgmt=${KEY_MGMT}
	psk="${PASSWORD0}"
	proto=RSN
	pairwise=CCMP
	ieee80211w=2
	#no_auto_peer=1
	beacon_int=100
	dot11MeshRetryTimeout=1000
	dot11MeshHoldingTimeout=400
	dot11MeshMaxRetries=4
	mesh_rssi_threshold=0
}
p2p_disabled=1
EOF
}

write_config_ap() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/hostapd
country_code=${COUNTRY}
ssid=$SSID1
hw_mode=a
bridge=br0
beacon_int=100
channel=${CHANNEL}
ieee80211h=1
ieee80211d=1
ieee80211n=1
interface=${2}
macaddr_acl=0
driver=nl80211
ap_max_inactivity=16779
EOF
}

write_config_ap_append_sec() {
	local FILENAME=$1
cat << EOF >> $FILENAME
ieee80211w=2
wpa=2
wpa_passphrase=${PASSWORD0}
wpa_key_mgmt=${KEY_MGMT}
wpa_pairwise=CCMP
rsn_pairwise=CCMP
EOF
}

write_config_ap_append_wpa2() {
	local FILENAME=$1
cat << EOF >> $FILENAME
wpa=2
wpa_passphrase=${PASSWORD0}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=CCMP
rsn_pairwise=CCMP
EOF
}

write_config_sta() {
	local FILENAME=$1
cat << EOF > $FILENAME
ctrl_interface=/var/run/wpa_supplicant
country=${COUNTRY}

network={
	ssid="${SSID1}"
	psk="${PASSWORD1}"
}
p2p_disabled=1
EOF
}

write_config_dnsmasq() {
	local FILENAME=$1
	local SUBNET=$(echo ${2} | cut -d. -f1,2,3)
cat << EOF > $FILENAME
interface=${3}
dhcp-range=${SUBNET}.2,${SUBNET}.254,255.255.255.0,infinite
dhcp-leasefile=dnsmasq.leases
EOF
}

run_mp() {
	cleanup
	enable_drv ${1}
	sudo ifconfig ${1} up
	echo set txpwr 17
	/home/pi/nrc_pkg/script/cli_app set txpwr 17

	if [ "$KEY_MGMT" != "NONE" ]; then
		KEY_MGMT="SAE"
		write_config_mp_sec mesh.conf
	else
		write_config_mp mesh.conf
	fi
	sudo chmod 755 mesh.conf
	sudo wpa_supplicant -i${1} -c mesh.conf -dddd &
	sleep 1

	if [[ $USE_MP_ADDR -eq 1 ]]; then
		add_peer ${1} ${MP_ADDR}
	fi
	if [[ $USE_DHCP_SERVER == 1 ]]; then
		sudo dhcpcd -j dhcpcd.log
	else
		sudo ifconfig ${1} $IP_ADDR0
	fi

	sudo sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
	#sudo iw dev ${1} set mesh_param mesh_hwmp_rootmode 3
	#sudo iw dev ${1} set mesh_param mesh_hwmp_root_interval 5000
	#sudo iw dev ${1} set mesh_param mesh_hwmp_active_path_to_root_timeout 10000
	wait_for_ip ${1}
}

run_mpp() {
	cleanup
	enable_drv ${1}
	sudo ifconfig ${1} up
	echo set txpwr 17
	/home/pi/nrc_pkg/script/cli_app set txpwr 17

	if [ "$KEY_MGMT" != "NONE" ]; then
		KEY_MGMT="SAE"
		write_config_mp_sec mesh.conf
	else
		write_config_mp mesh.conf
	fi
	sudo chmod 755 mesh.conf
	sudo wpa_supplicant -i${1} -c mesh.conf -dddd &
	sleep 1

	if [[ $USE_MP_ADDR -eq 1 ]]; then
		add_peer ${1} ${MP_ADDR}
	fi
	if [[ $USE_DHCP_SERVER == 1 ]]; then
		write_config_dnsmasq dnsmasq.conf $IP_ADDR0 ${1}
		sudo dnsmasq -C dnsmasq.conf
	fi
	sudo ifconfig ${1} $IP_ADDR0
	set_nat ${2} ${1}
	sudo ip route add 192.168.0.0/16 via $IP_ADDR0 dev ${1}

	sudo sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
	#sudo iw dev ${1} set mesh_param mesh_hwmp_rootmode 3
	#sudo iw dev ${1} set mesh_param mesh_hwmp_root_interval 5000
	#sudo iw dev ${1} set mesh_param mesh_hwmp_active_path_to_root_timeout 10000
}

run_map() {
	cleanup
	enable_drv ${1}
	enable_conc_intf ${1} ${2}
	echo set txpwr 17
	/home/pi/nrc_pkg/script/cli_app set txpwr 17

	write_config_ap ap.conf ${1}
	if [ "$KEY_MGMT" == "WPA2" ]; then
		write_config_ap_append_wpa2 ap.conf
	elif [ "$KEY_MGMT" != "NONE" ]; then
		write_config_ap_append_sec ap.conf
	fi
	sudo chmod 755 ap.conf

	if [ "$KEY_MGMT" != "NONE" ]; then
		KEY_MGMT="SAE"
		write_config_mp_sec mesh.conf
	else
		write_config_mp mesh.conf
	fi
	sudo chmod 755 mesh.conf
	sudo wpa_supplicant -i${2} -c mesh.conf -dddd &
	sleep 1

	if [[ $USE_MP_ADDR -eq 1 ]]; then
		add_peer ${2} ${MP_ADDR}
	fi
	if [[ $USE_DHCP_SERVER == 1 ]]; then
		sudo dhcpcd ${2} -j dhcpcd.log
	else
		sudo ifconfig ${2} $IP_ADDR0
	fi

	wait_for_ip ${2}
	sudo hostapd ap.conf -ddd &
	sleep 1

	set_nat ${1} ${2}
	set_bridge_map ${1} ${2}
	sudo ifconfig br0 $(get_ip_addr ${2})

	sudo sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
	#sudo iw dev ${2} set mesh_param mesh_hwmp_rootmode 3
	#sudo iw dev ${2} set mesh_param mesh_hwmp_root_interval 5000
	#sudo iw dev ${2} set mesh_param mesh_hwmp_active_path_to_root_timeout 10000
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
		SSID1="$1"
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
		-u|--auth)
		shift
		KEY_MGMT="$1"
		shift
		;;
		-q|--dhcp)
		shift
		USE_DHCP_SERVER="$1"
		shift
		;;
		-n|--country)
		shift
		COUNTRY="$1"
		shift
		;;
		--)
		shift
		break
		;;
	esac
done

if [ "$MODE" == "map" ]; then
	run_map wlan0 wlan1
elif [ "$MODE" == "mp" ]; then
	run_mp wlan0
elif [ "$MODE" == "mpp" ]; then
	run_mpp wlan0 eth0
else
	exit 1
fi


while :; do do_work; sleep 2; done
