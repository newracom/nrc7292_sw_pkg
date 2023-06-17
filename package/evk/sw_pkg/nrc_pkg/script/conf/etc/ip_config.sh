#!/bin/bash

#This script is for setting IP and DHCP
source /home/pi/nrc_pkg/script/conf/etc/CONFIG_IP

if [ $# -eq 0 ]; then
	echo "Usage: ./ip_config.sh [STA|AP|RELAY|SNIFFER|MESH|IBSS|IBSS_DHCPS] [relay_type] [static_ip] [batman]"
	exit
fi

echo "===================================="
if [ $USE_ETH_DHCP_SERVER == 'Y' ];then
	echo " INTERFACE        : eth0 "
	echo " ETH STATIC IP    : $ETH_DHCPS_IP "
	echo " DHCP CONFIG      : $ETH_DHCPS_CONFIG "
	echo "----------------------------------"
elif [ $USE_ETH_STATIC_IP == 'Y' ]; then
	echo " INTERFACE        : eth0 "
	echo " ETH STATIC IP    : $ETH_STATIC_IP "
	echo " NET MASK NUM     : $ETH_STATIC_NETMASK "
fi


if [[ "$1" == "STA" ]] || [[ "$1" == "SNIFFER" ]]; then
	if [ $USE_HALOW_STA_STATIC_IP == 'Y' ];then
		echo " STA INTERFACE    : wlan0"
		echo " STA STATIC IP    : $HALOW_STA_IP "
		echo " NET MASK NUM     : $HALOW_STA_NETMASK "
		echo " Default GW       : $HALOW_STA_DEFAULT_GW "
	else
		echo " STA INTERFACE    : wlan0"
		echo " USE DHCP Client "
	fi
elif [ "$1" == "AP" ]; then
	echo " AP INTERFACE     : wlan0"
	echo " AP STATIC IP     : $HALOW_AP_IP "
	echo " NET MASK NUM     : $HALOW_AP_NETMASK "
elif [ "$1" == "RELAY" ]; then
	if [ "$2" == 0 ]; then
		STA_INF=wlan0
		AP_INF=wlan1
	else
		STA_INF=wlan1
		AP_INF=wlan0
	fi
	if [ $USE_HALOW_STA_STATIC_IP == 'Y' ];then
		echo " STA INTERFACE    : $STA_INF "
		echo " STA STATIC IP    : $HALOW_STA_IP "
		echo " NET MASK NUM     : $HALOW_STA_NETMASK "
		echo " Default GW       : $HALOW_STA_DEFAULT_GW "
		echo "----------------------------------"
	else
		echo " STA INTERFACE    : $STA_INF "
		echo " USE DHCP Client "
		echo "----------------------------------"
	fi
	echo " AP INTERFACE     : $AP_INF "
	echo " AP STATIC IP     : $HALOW_RELAY_AP_IP "
	echo " NET MASK NUM     : $HALOW_RELAY_AP_NETMASK "
elif [ "$1" == "MESH" ]; then
	if [ "$2" == 2 ]; then
		echo " MESH INTERFACE   : mesh0 "
		echo " AP INTERFACE     : wlan0 "
		MESH_INF=br0
	elif [ "$2" == 0 ]; then
		if [ "$3" == "nodhcp" ]; then
			echo " MESH INTERFACE   : br0 "
			MESH_INF=br0
		else
			echo " MESH INTERFACE   : wlan0 "
			MESH_INF=wlan0
		fi
	else
		echo " MESH INTERFACE   : wlan0 "
		MESH_INF=wlan0
	fi
	if [ "$3" == 0 ]; then
		echo " USE DHCP Client "
	elif [ "$3" == "nodhcp" ]; then
		echo " NO DHCP Server "
	else
		echo " MESH STATIC IP   : $3 "
	fi
	if [ "$4" == 0 ]; then
		echo " - HWMP "
	else
		echo " - B.A.T.M.A.N    : $4 "
		MESH_INF=$4
	fi
elif [ "$1" == "IBSS" ]; then
	if [ "$3" == 0 ];then
		echo " IBSS STA INTERFACE    : wlan0"
		echo " USE DHCP Client "
    else
        echo " IBSS STA INTERFACE    : wlan0"
		echo " IBSS STA STATIC IP    : $3 "
	fi
elif [ "$1" == "IBSS_DHCPS" ]; then
	echo " IBSS STA with DHCP Server "
	echo " IBSS STA INTERFACE     : wlan0"
	echo " IBSS STA STATIC IP     : $HALOW_AP_IP "
	echo " NET MASK NUM           : $HALOW_AP_NETMASK "
fi
echo "===================================="

if [ -d "/home/pi/nrc_pkg" ]; then
	# HaLow STA and Sniffer
	if [[ "$1" == "STA" ]] || [[ "$1" == "SNIFFER" ]]; then
		#interface wlan0 for HaLow STA
		sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
		sed -i "60s/.*/metric 100/g" /home/pi/nrc_pkg/etc/dhcpcd/dhcpcd.conf
		if [ $USE_HALOW_STA_STATIC_IP == 'Y' ];then
			sed -i "61s/.*/static ip_address=$HALOW_STA_IP\/$HALOW_STA_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "62s/.*/static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
		else
			sed -i "61s/.*/#static ip_address=$HALOW_STA_IP\/$HALOW_STA_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "62s/.*/#static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
		fi
		#inteface wlan1 => Not Used
		sed -i "65s/.*/#interface wlan1/g" $DHCPCD_CONF_FILE
		sed -i "66s/.*/#metric 100/g" $DHCPCD_CONF_FILE
		sed -i "67s/.*/#static ip_address=$HALOW_AP_IP\/$HALOW_AP_NETMASK/g" $DHCPCD_CONF_FILE
		sed -i "68s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
		#dhcp server on wlan0 => Not Used
		sed -i "3s/.*/#interface=wlan0/g" $DNSMASQ_CONF_FILE
		sed -i "4s/.*/#dhcp-range=$HALOW_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
		echo "Config for $1 is done!"
	# HaLow AP
	elif [ "$1" == "AP" ]; then
		#interface wlan0 AP
		sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
		sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
		sed -i "61s/.*/static ip_address=$HALOW_AP_IP\/$HALOW_AP_NETMASK/g" $DHCPCD_CONF_FILE
		sed -i "62s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
		#inteface wlan1 => Not Used
		sed -i "65s/.*/#interface wlan1/g" $DHCPCD_CONF_FILE
		sed -i "66s/.*/#metric 100/g" $DHCPCD_CONF_FILE
		sed -i "67s/.*/#static ip_address=$HALOW_AP_IP\/$HALOW_AP_NETMASK/g" $DHCPCD_CONF_FILE
		sed -i "68s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
		#dhcp server on wlan0 => Used
		sed -i "3s/.*/interface=wlan0/g" $DNSMASQ_CONF_FILE
		sed -i "4s/.*/dhcp-range=$HALOW_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
		echo "Config for AP is done!"
	# HaLow RELAY
	elif [ "$1" == "RELAY" ]; then
		if [ "$2" == 0 ]; then
			#Type 0: interface wlan0 STA and wlan1 AP
			# wlan0 - STA
			sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
			sed -i "60s/.*/metric 100/g" $DHCPCD_CONF_FILE
			if [ $USE_HALOW_STA_STATIC_IP == 'Y' ];then
				sed -i "61s/.*/static ip_address=$HALOW_STA_IP\/$HALOW_STA_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			else
				sed -i "61s/.*/#static ip_address=$HALOW_STA_IP\/$HALOW_STA_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/#static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			fi
			# wlan1 - AP
			sed -i "65s/.*/interface wlan1/g" $DHCPCD_CONF_FILE
			sed -i "66s/.*/#metric 100/g" $DHCPCD_CONF_FILE
			sed -i "67s/.*/static ip_address=$HALOW_RELAY_AP_IP\/$HALOW_RELAY_AP_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "68s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
			# dhcp client on STA (wlan0) => Used
			sed -i "3s/.*/#interface=wlan0/g" $DNSMASQ_CONF_FILE
			sed -i "4s/.*/#dhcp-range=$HALOW_RELAY_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			# dhcp server on AP (wlan1) => Used
			sed -i "5s/.*/interface=wlan1/g" $DNSMASQ_CONF_FILE
			sed -i "6s/.*/dhcp-range=$HALOW_RELAY_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
		else
			#Type 1: interface wlan0 AP and wlan1 STA
			# wlan0 - AP
			sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
			sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
			sed -i "61s/.*/static ip_address=$HALOW_RELAY_AP_IP\/$HALOW_RELAY_AP_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "62s/.*/#static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			# wlan1 - STA
			sed -i "65s/.*/interface wlan1/g" $DHCPCD_CONF_FILE
			sed -i "66s/.*/metric 100/g" $DHCPCD_CONF_FILE
			if [ $USE_HALOW_STA_STATIC_IP == 'Y' ];then
				sed -i "67s/.*/static ip_address=$HALOW_STA_IP\/$HALOW_STA_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "68s/.*/static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			else
				sed -i "67s/.*/#static ip_address=$HALOW_STA_IP\/$HALOW_STA_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "68s/.*/#static routers=$HALOW_STA_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			fi
			#dhcp server on AP (wlan0) => Used
			sed -i "3s/.*/interface=wlan0/g" $DNSMASQ_CONF_FILE
			sed -i "4s/.*/dhcp-range=$HALOW_RELAY_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			#dhcp client on STA (wlan1) => Used
			sed -i "5s/.*/#interface=wlan1/g" $DNSMASQ_CONF_FILE
			sed -i "6s/.*/#dhcp-range=$HALOW_RELAY_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
		fi

		echo "Config for RELAY is done!"
	elif [ "$1" == "MESH" ]; then
		if [ "$2" == 0 ]; then
			#Type 0: Mesh Portal
			#interface br0 for Mesh
			if [ "$3" == 0 ]; then
				sed -i "59s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
				sed -i "61s/.*/static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			elif [ "$3" == "nodhcp" ]; then
				sed -i "59s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
				sed -i "61s/.*/#static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			else
				sed -i "59s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
				sed -i "61s/.*/static ip_address=$3\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			fi
			#inteface wlan1 => Not Used
			sed -i "65s/.*/#interface wlan1/g" $DHCPCD_CONF_FILE
			sed -i "66s/.*/#metric 100/g" $DHCPCD_CONF_FILE
			sed -i "67s/.*/#static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "68s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			#dhcp server on wlan0 => Used
			if [ "$3" == 0 ]; then
				if [ "$4" == 0 ]; then
					sed -i "3s/.*/interface=$MESH_INF/g" $DNSMASQ_CONF_FILE
				else
					sed -i "3s/.*/#interface=$MESH_INF/g" $DNSMASQ_CONF_FILE
				fi
				sed -i "4s/.*/dhcp-range=$HALOW_MESH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			else
				sed -i "3s/.*/#interface=$MESH_INF/g" $DNSMASQ_CONF_FILE
				sed -i "4s/.*/#dhcp-range=$HALOW_MESH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			fi
			echo "Config for Mesh Portal is done!"
		elif [ "$2" == 1 ]; then
			#Type 1: Mesh Point
			#interface wlan0 for Mesh
			if [ "$3" == 0 ]; then
				sed -i "59s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "60s/.*/metric 100/g" $DHCPCD_CONF_FILE
				sed -i "61s/.*/#static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/#static routers=$HALOW_MESH_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			else
				sed -i "59s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "60s/.*/metric 100/g" $DHCPCD_CONF_FILE
				sed -i "61s/.*/static ip_address=$3\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "62s/.*/#static routers=$HALOW_MESH_DEFAULT_GW/g" $DHCPCD_CONF_FILE
			fi
			#inteface wlan1 => Not Used
			sed -i "65s/.*/#interface wlan1/g" $DHCPCD_CONF_FILE
			sed -i "66s/.*/#metric 100/g" $DHCPCD_CONF_FILE
			sed -i "67s/.*/#static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "68s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			#dhcp server on wlan0 => Not Used
			sed -i "3s/.*/#interface=$MESH_INF/g" $DNSMASQ_CONF_FILE
			sed -i "4s/.*/#dhcp-range=$HALOW_MESH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			echo "Config for Mesh Point is done!"
		elif [ "$2" == 2 ]; then
			#Type 2: Mesh AP
			#interface wlan0 for AP
			sed -i "59s/.*/#interface wlan0/g" $DHCPCD_CONF_FILE
			sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
			sed -i "61s/.*/#static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "62s/.*/#static routers==$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			#interface mesh0 for Mesh
			if [ "$3" == 0 ]; then
				sed -i "65s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "66s/.*/metric 100/g" $DHCPCD_CONF_FILE
				sed -i "67s/.*/#static ip_address=$HALOW_MESH_IP\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "68s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			else
				sed -i "65s/.*/interface $MESH_INF/g" $DHCPCD_CONF_FILE
				sed -i "66s/.*/metric 100/g" $DHCPCD_CONF_FILE
				sed -i "67s/.*/static ip_address=$3\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
				sed -i "68s/.*/#static routers=$HALOW_MESH_IP/g" $DHCPCD_CONF_FILE
			fi
			#dhcp server on AP (wlan0) => Not Used
			sed -i "3s/.*/#interface=wlan0/g" $DNSMASQ_CONF_FILE
			sed -i "4s/.*/#dhcp-range=$HALOW_MESH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			#dhcp server on Mesh (wlan1) => Not Used
			sed -i "5s/.*/#interface=wlan1/g" $DNSMASQ_CONF_FILE
			sed -i "6s/.*/#dhcp-range=$HALOW_MESH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
			echo "Config for Mesh AP is done!"
		fi
	# HaLow IBSS STA
	elif [ "$1" == "IBSS" ] ; then
		#interface wlan0 for HaLow IBSS STA
		if [ "$3" == 0 ]; then
		#DHCP Client
			sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
			sed -i "60s/.*/metric 100/g" $DHCPCD_CONF_FILE
			sed -i "61s/.*/#static ip_address=$3\/$HALOW_AP_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "62s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
		else
		#Static IP
			sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
			sed -i "60s/.*/metric 100/g" $DHCPCD_CONF_FILE
			sed -i "61s/.*/static ip_address=$3\/$HALOW_MESH_NETMASK/g" $DHCPCD_CONF_FILE
			sed -i "62s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
		fi
		echo "Config for $1 STA is done!"
	elif [ "$1" == "IBSS_DHCPS" ]; then
		#interface wlan0 IBSS STA with DHCP Server
		sed -i "59s/.*/interface wlan0/g" $DHCPCD_CONF_FILE
		sed -i "60s/.*/#metric 100/g" $DHCPCD_CONF_FILE
		sed -i "61s/.*/static ip_address=$HALOW_AP_IP\/$HALOW_AP_NETMASK/g" $DHCPCD_CONF_FILE
		sed -i "62s/.*/#static routers=$HALOW_AP_IP/g" $DHCPCD_CONF_FILE
		#dhcp server on wlan0 => Used
		sed -i "3s/.*/interface=wlan0/g" $DNSMASQ_CONF_FILE
		sed -i "4s/.*/dhcp-range=$HALOW_AP_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
	else
		echo "Usage: ./set_dhcpcd [STA|AP|RELAY|SNIFFER|MESH|IBSS]"
		exit
	fi
else
	echo "nrc_pkg is not installed yet"
	exit
fi

# DHCP Server on Ethenet
if [ $USE_ETH_DHCP_SERVER == 'Y' ];then
	#interface eth0 uses static IP
	sed -i "71s/.*/interface eth0/g" $DHCPCD_CONF_FILE
	sed -i "72s/.*/static ip_address=$ETH_DHCPS_IP\/24/g" $DHCPCD_CONF_FILE
	# Ethernet uses DHCP Server
	sed -i "1s/.*/interface=eth0/g" $DNSMASQ_CONF_FILE
	sed -i "2s/.*/dhcp-range=$ETH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
else
	if [ $USE_ETH_STATIC_IP == 'Y' ];then
		#interface eth0 uses static IP
		sed -i "71s/.*/interface eth0/g" $DHCPCD_CONF_FILE
		sed -i "72s/.*/static ip_address=$ETH_STATIC_IP\/$ETH_STATIC_NETMASK/g" $DHCPCD_CONF_FILE
	else
		#interface eth0 uses DHCP
		sed -i "71s/.*/#interface eth0/g" $DHCPCD_CONF_FILE
		sed -i "72s/.*/#static ip_address=$ETH_STATIC_IP\/24/g" $DHCPCD_CONF_FILE
	fi
	# Ethernet does NOT use DHCP Server
	sed -i "1s/.*/#interface=eth0/g" $DNSMASQ_CONF_FILE
	sed -i "2s/.*/#dhcp-range=$ETH_DHCPS_CONFIG/g" $DNSMASQ_CONF_FILE
fi

#copy dhcpcd.conf and dnsmasq.conf into /etc/
sudo cp $DHCPCD_CONF_FILE /etc/dhcpcd.conf
sudo cp $DNSMASQ_CONF_FILE /etc/dnsmasq.conf

echo "IP and DHCP config done"
