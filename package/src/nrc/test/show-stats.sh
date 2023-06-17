#!/bin/sh

echo " ------------------------------------------------------------------"
echo "  #     # ####### #     # ######     #     #####  ####### #     #  "
echo "  ##    # #       #  #  # #     #   # #   #     # #     # ##   ##  "
echo "  # #   # #       #  #  # #     #  #   #  #       #     # # # # #  "
echo "  #  #  # #####   #  #  # ######  #     # #       #     # #  #  #  "
echo "  #   # # #       #  #  # #   #   ####### #       #     # #     #  "
echo "  #    ## #       #  #  # #    #  #     # #     # #     # #     #  "
echo "  #     # #######  ## ##  #     # #     #  #####  ####### #     #  "
echo " ------------------------------------------------------------------"


echo " - Monitor SNR/RSSI "
while true; do 
	snr="$(cat /sys/kernel/debug/ieee80211/nrc80211/snr)"
	rssi="$(cat /sys/kernel/debug/ieee80211/nrc80211/rssi)"
	printf "\r - SNR:%b, RSSI:%b" $snr $rssi
	sleep 0.1
done
