1. Alias
 There are 4 aliases for fast access to folder or files.
 You can just type aliases to access them fast.
 For example, type "script" to access folder(/home/pi/nrc_pkg/script/) where there are scripts to manipulate 11ah AP, STA and SNIFFER.

  1) nrc='cd /home/pi/nrc_pkg'
  2) script='cd /home/pi/nrc_pkg/script/'
  3) ipset='sudo vi /etc/dhcpcd.conf'
  4) dhcpset='sudo vi /etc/dnsmasq.conf'

2. Before running EVK, you should change python files included in nrc_pkg as executable.
   Just do like below.

  cd /home/pi/nrc_pkg
  chmod -R 755 *


3. Run EVK as AP, STA or Sniffer
 We provide the python script to run and stop STA, AP and Sniffer, so it's not difficult to run and stop.
 You should give 4 or 6 arguments when running them by script.
 Usage is like below. (This script is at /home/pi/nrc_pkg/script)

Usage:
        start.py [sta_type] [security_mode] [country] [channel] [sniffer_mode]
Argument:
        sta_type      [0:STA   |  1:AP  		|  2:SNIFFER  | 3:RELAY]
        security_mode [0:Open  |  1:WPA2-PSK  	|  2:WPA3-OWE | 3:WPA3-SAE]
        country       [US:USA  |  JP:Japan  |  TW:Taiwan  | KR:Korea | EU:EURO | CN:China |
	                   AU:Austrailia  |  NZ:New Zealand]
        -----------------------------------------------------------
        channel       [S1G Channel Number]   * Only for Sniffer
        sniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer
Example:
        OPEN mode STA for Korea                : ./start.py 0 0 KR
        Security mode AP for US                : ./start.py 1 1 US
        Local Sniffer mode on CH 40 for Japan  : ./start.py 2 0 JP 40 0
Note:
        sniffer_mode should be set as '1' when running sniffer on remote terminal

4. Stop AP, STA or Sniffer
 To stop STA/AP/Sniffer, just run stop.py script like below.
 It clears all the processes related to EVK and remove the module.

  ./stop.py

5. Changing Channel for Sniffer
 Running-change of channel is possible only during Sniffer Mode.
 It can be done by the script below which are on /home/pi/nrc_pkg/script/sniffer/

  ./change_channel [channel]
