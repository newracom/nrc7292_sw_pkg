#bin/sh
CNT=0
while true; do
echo "TEST COUNT : ${CNT}"
./sta_sdk2_open.py
sleep 3
iperf3 -c 192.161.200.1 -i 1 -b20M -t100 &
sleep 10
sudo rmmod nrc
sudo killall wpa_supplicant
sudo killall iperf3
sleep 3
(( CNT = "${CNT}" + 1))
done
