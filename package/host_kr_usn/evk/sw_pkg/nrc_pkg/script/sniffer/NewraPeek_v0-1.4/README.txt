Date: 12/20/2018
Author: Aaron J. Lee (jehun.lee@newracom.com)

Base code: wireshark-2.2.5.tar.bz2 (https://2.na.dl.wireshark.org/src/wireshark-2.2.5.tar.bz2)

Platform: Raspberry Pi 3 Model B
OS: Raspbian Jessie with Pixel (Linux Kernel Version: 4.14.70)

Installation

1. Install NewraPeek: sudo dpkg -i newrapeek_0-1.4_armhf.deb
2. Install PCAP libray: sudo apt-get install libpcap0.8 (if necessary)
3. Modify /etc/environment and add following line.
   LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib
4. Reboot
5. Run NewraPeek: country_code = KR, US, JP, TW , AU, or NZ
   6.1 local running:  ./start.py 2 0 KR channel_number 0 &
   6.2 remote running: ./start.py 2 0 KR channel_number 1 &
   6.3 change channel: ./change_channel.py channel_number

Version History
(05/04/2018) NewraPeek v0-1.3 - renew version
(10/16/2018) NewraPeek v0-1.4 - NRC7292/NRC7291 version
(12/20/2018) NewraPeek v0-1.4 - repackaging
