# Run this script on AP, then rmmod nrc. Refer to CS#2673 and PT#283
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF0 `awk 'BEGIN{for(i=0;i<251;i++){printf("0xFF ")}}'`
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF1 `awk 'BEGIN{for(i=0;i<251;i++){printf("0x00 ")}}'`
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF2 `awk 'BEGIN{for(i=0;i<251;i++){printf("0x55 ")}}'`
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF3 `awk 'BEGIN{for(i=0;i<251;i++){printf("0xAA ")}}'`
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF4 `awk 'BEGIN{for(i=0;i<251;i++){printf("0x%02X ",i%256)}}'`
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF4 0xEE
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF3 0xDD
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF2 0xCC
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF1 0xBB
sudo iw dev wlan0 vendor recv 0xFCFFAA 0xF0 0xAA
