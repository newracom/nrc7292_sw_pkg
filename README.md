# NRC7292 Software Package for Host mode (Linux OS)

## Notice

> [!CAUTION]
> The software package released here is specifically designed for the NRC7292 EVK, and the accompanying board data files in https://github.com/newracom/nrc7292_sw_pkg/tree/master/package/evk/binary and 
https://github.com/newracom/nrc7292_sw_pkg/tree/master/package/evk/sw_pkg/nrc_pkg/sw/firmware are intended solely for this model.
> If users plan to use this software package with other devices that incorporate the NRC7292 chip, they should utilize the board data file supplied by the device's vendor.

> [!IMPORTANT]
> For support regarding the SAE Hash-to-Element (H2E), please refer to https://github.com/newracom/nrc7292_sw_pkg/blob/master/README-H2E.md

### Release roadmap
- v1.5.1 (2024.05.10): Hotfix
- v1.5 (2023.10.31)
- v1.4.1 (2023.09.08)
- v1.4 (2023.06.16)
- v1.3.4_rev14 (2022.12.01)
- v1.3.4_rev13 (2022.11.30)
- v1.3.4_rev12 (2022.10.28)
- v1.3.4_rev11 (2022.10.14)
- v1.3.4_rev10 (2022.10.05)
- v1.3.4_rev09 (2022.09.29)
- v1.3.4_rev08 (2022.08.30)
- v1.3.4_rev07 (2022.08.10)
- v1.3.4_rev06 (2022.07.22)
- v1.3.4_rev05 (2022.07.08)
- v1.3.4_rev04 (2022.06.30)
- v1.3.4_rev03 (2022.05.24)
- v1.3.4_rev02 (2022.01.19)
- v1.3.4_rev01 (2021.12.20)
- v1.3.4 (2021.10.22)
- v1.3.3 (2021.04.26)
- v1.3.2 (2020.09.16)
- v1.3.1 (2020.08.05)
- v1.3.0 (2020.05.30)

### Latest release
- [NRC7292_SW_PKG_v1.5.1](https://github.com/newracom/nrc7292_sw_pkg/releases/tag/v1.5.1)

### Release package contents
- package: NRC7292 software package for global regulatory domains
```
  package
  ├── doc
  ├── evk
  │   ├── binary
  │   └── sw_pkg
  │       └── nrc_pkg
  └── src
      ├── cli_app
      ├── ft232h-usb-spi
      ├── nrc
      └── nrc_hspi_simple
```
## NRC7292 Software Package User Guide
### Get NRC7292 Software Package
NRC7292 Software package is provided in this repository. Please refer to the following git command to get it.
```
cd ~/
git clone https://github.com/newracom/nrc7292_sw_pkg.git
```
### Prepare host platform (Raspberry Pi 3 Model B+)
#### Install Newracom Device Tree (DT) overlay
You can get Device Tree Source (DTS) for NRC7292 in dts directory. Please install DT overlay by following commands below.
```
cd /boot/overlays
sudo dtc -I dts -O dtb -o newracom.dtbo newracom.dts
sudo vi /boot/config.txt
  dtoverlay=newracom
```
### Install release package 
Please upload the contents in host/evk/sw_pkg to sw_pkg directory on host platform, and follow below commands to install release package.
```
cd sw_pkg
chmod +x ./update.sh
./update.sh
```
And you can also install key binaries or build host driver as below.
#### (Optional) Install firmware/driver/cli_app binaries
Please follow below commands to install firmware, driver and cli_app.
```
cd evk/binary
cp ./cli_app ~/nrc_pkg/script
cp ./nrc.ko ~/nrc_pkg/sw/driver
cp ./nrc7292_cspi.bin ~/nrc_pkg/sw/firmware
```
#### (Optional) Build host driver
You can get the pre-built host driver in nrc_pkg/sw/driver, but you can build it by youself by following below commands.
```
cd package/src/nrc
make clean
make
cp nrc.ko ~/nrc_pkg/sw/driver
```
### Apply a specific package
If you want to apply a specific package to your exiting package directory, you can choose one of following methods.
#### Method #1: replace the whole package
Let's assume that you have v1.3.0 and want to apply v1.3.1 to your package location.
1. Download a specific package you want.
   * If it is official release version 1.3.1
     1. Go to https://github.com/newracom/nrc7292_sw_pkg/releases and choose the release package you want.
     ![sw_pkg_release](/images/sw_pkg_release.png)
     1. Downalod the compressed package: zip version or tar.gz version
     1. Check the filename: nrc7292_sw_pkg-1.3.1.zip or nrc7292_sw_pkg-1.3.1.tar.gz
   * If it is the latest pacakge
     1. Click "Code" and then click "Download ZIP"
     ![sw_pkg_latest](/images/sw_pkg_latest.png)
     1. Check the filename: nrc7292_sw_pkg-master.zip
1. Replace your old package directory with the one you downloaded.
#### Method #2: pull down a branch
This needs your cloned repository and the internet connection.
1. Move to the repository directory
   ```
   cd repo/nrc7292_sw_pkg
   ```
1. Pull down a branch
   * If you want to pull down the latest one from master branch
   ```
   git pull
   ```
   * If you want to change into a specific branch by using tag version
   ```
   git tag -l
   git checkout v1.3.1
   ```
### Run NRC7292 Software Package
#### Configure start script
There are a couple of configurable parameters as below.
```
# Default Configuration (you can change value you want here)
##################################################################################
# Raspbery Pi Conf.
max_cpuclock      = 1         # Set Max CPU Clock : 0(off) or 1(on)
##################################################################################
# Firmware Conf.
model             = 7292      # 7292/7393/7394
fw_download       = 1         # 0(FW Download off) or 1(FW Download on)
fw_name           = 'uni_s1g.bin'
##################################################################################
# DEBUG Conf.
# NRC Driver log
driver_debug      = 0         # NRC Driver debug option : 0(off) or 1(on)
dbg_flow_control  = 0         # Print TRX slot and credit status in real-time
#--------------------------------------------------------------------------------#
# WPA Supplicant Log (STA Only)
supplicant_debug  = 0         # WPA Supplicant debug option : 0(off) or 1(on)
#--------------------------------------------------------------------------------#
# HOSTAPD Log (AP Only)
hostapd_debug     = 0         # Hostapd debug option    : 0(off) or 1(on)
#################################################################################
# CSPI Conf. (Default)
spi_clock    = 20000000       # SPI Master Clock Frequency
spi_bus_num  = 0              # SPI Master Bus Number
spi_cs_num   = 0              # SPI Master Chipselect Number
spi_gpio_irq = 5              # NRC-CSPI EIRQ GPIO Number
                              # BBB is 60 recommanded.
spi_polling_interval = 0      # NRC-CSPI Polling Interval (msec)

#
# NOTE:
#  - NRC-CSPI EIRQ Input Interrupt: spi_gpio_irq >= 0 and spi_polling_interval <= 0
#  - NRC-CSPI EIRQ Input Polling  : spi_gpio_irq >= 0 and spi_polling_interval > 0
#  - NRC-CSPI Registers Polling   : spi_gpio_irq < 0 and spi_polling_interval > 0
#
#--------------------------------------------------------------------------------#
# FT232H USB-SPI Conf. (FT232H CSPI Conf)
ft232h_usb_spi = 0            # FTDI FT232H USB-SPI bridge
                              # 0 : Unused
                              # 1 : NRC-CSPI_EIRQ Input Polling
                              # 2 : NRC-CSPI Registers Polling
#################################################################################
# RF Conf.
max_txpwr         = 24       # Maximum TX Power (in dBm)
epa               = 0        # (7394 only) External PA : 0(none) or 1(used)
bd_name           = ''       # board data name (bd defines max TX Power per CH/MCS/CC)
                             # specify your bd name here. If not, follow naming rules in strBDName()
##################################################################################
# PHY Conf.
guard_int         = 'long'   # Guard Interval ('long'(LGI) or 'short'(SGI))
##################################################################################
# MAC Conf.
# S1G Short Beacon (AP & MESH Only)
#  If disabled, AP sends only S1G long beacon every BI
#  Recommend using S1G short beacon for network efficiency (Default: enabled)
short_bcn_enable  = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Legacy ACK enable (AP & STA)
#  If disabled, AP/STA sends only NDP ack frame
#  Recommend using NDP ack mode  (Default: disable)
legacy_ack_enable  = 0        # 0 (NDP ack mode) or 1 (legacy ack mode)
#--------------------------------------------------------------------------------#
# Beacon Bypass enable (STA only)
#  If enabled, STA receives beacon frame from other APs even connected
#  Recommend that STA only receives beacon frame from AP connected while connecting  (Default: disable)
beacon_bypass_enable  = 0        # 0 (Receive beacon frame from only AP connected while connecting)
                                 # 1 (Receive beacon frame from all APs even while connecting)
#--------------------------------------------------------------------------------#
# AMPDU (Aggregated MPDU)
#  Enable AMPDU for full channel utilization and throughput enhancement (Default: auto)
#  disable(0): AMPDU is deactivated
#   manual(1): AMPDU is activated and BA(Block ACK) session is made by manual
#     auto(2): AMPDU is activated and BA(Block ACK) session is made automatically
ampdu_enable      = 2        # 0 (disable) 1 (manual) 2 (auto)
#--------------------------------------------------------------------------------#
# 1M NDP (Block) ACK
#  Enable 1M NDP ACK on 2/4MHz BW instead of 2M NDP ACK
#  Note: if enabled, throughput can be decreased on high MCS
ndp_ack_1m        = 0        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# NDP Probe Request
#  For STA, "scan_ssid=1" in wpa_supplicant's conf should be set to use
ndp_preq          = 0        # 0 (Legacy Probe Req) 1 (NDP Probe Req)
#--------------------------------------------------------------------------------#
# CQM (Channel Quality Manager) (STA Only)
#  STA can disconnect according to Channel Quality (Beacon Loss or Poor Signal)
#  Note: if disabled, STA keeps connection regardless of Channel Quality
cqm_enable        = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# RELAY (Do NOT use! it will be deprecated)
relay_type        = 1        # 0 (wlan0: STA, wlan1: AP) 1 (wlan0: AP, wlan1: STA)
relay_nat         = 1        # 0 (not use NAT) 1 (use NAT - no need to add routing table)
#--------------------------------------------------------------------------------#
# Power Save (STA Only)
#  3-types PS: (0)Always on (2)Deep_Sleep(TIM) (3)Deep_Sleep(nonTIM)
#     TIM Mode : check beacons during PS to receive BU from AP
#  nonTIM Mode : Not check beacons during PS (just wake up by TX or EXT INT)
power_save        = 0        # STA (power save type 0~3)
ps_timeout        = '3s'     # STA (timeout before going to sleep) (min:1000ms)
sleep_duration    = '3s'     # STA (sleep duration only for nonTIM deepsleep) (min:1000ms)
listen_interval   = 1000     # STA (listen interval in BI unit) (max:65535)
                             # Listen Interval time should be less than bss_max_idle time to avoid association reject
#--------------------------------------------------------------------------------#
# BSS MAX IDLE PERIOD (aka. keep alive) (AP Only)
#  STA should follow (i.e STA should send any frame before period),if enabled on AP
#  Period is in unit of 1000TU(1024ms, 1TU=1024us)
#  Note: if disabled, AP removes STAs' info only with explicit disconnection like deauth
bss_max_idle_enable = 1      # 0 (disable) or 1 (enable)
bss_max_idle        = 1800   # time interval (e.g. 1800: 1843.2 sec) (1 ~ 65535)
#--------------------------------------------------------------------------------#
#  SW encryption/decryption (default HW offload)
sw_enc              = 0     # 0 (HW), 1 (SW), 2 (HYBRID: SW GTK HW PTK)
#--------------------------------------------------------------------------------#
# Mesh Options (Mesh Only)
#  Manual Peering & Static IP
peer                = 0     # 0 (disable) or Peer MAC Address
static_ip           = 0     # 0 (disable) or Static IP Address
batman              = 0     # 0 (disable) or 'bat0' (B.A.T.M.A.N routing protocol)
#--------------------------------------------------------------------------------#
# Self configuration (AP Only)
#  AP scans the clearest CH and then starts with it
self_config       = 0        # 0 (disable)  or 1 (enable)
prefer_bw         = 0        # 0: no preferred bandwidth, 1: 1M, 2: 2M, 4: 4M
dwell_time        = 100      # max dwell is 1000 (ms), min: 10ms, default: 100ms
#--------------------------------------------------------------------------------#
# Credit num of AC_BE for flow control between host and target (Test only)
credit_ac_be      = 40        # number of buffer (min: 40, max: 120)
#--------------------------------------------------------------------------------#
# Filter tx deauth frame for Multi Connection Test (STA Only) (Test only)
discard_deauth    = 1         # 1: discard TX deauth frame on STA
#--------------------------------------------------------------------------------#
# Use bitmap encoding for NDP (block) ack operation (NRC7292 only)
bitmap_encoding   = 1         # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# User scrambler reversely (NRC7292 only)
reverse_scrambler = 1         # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# Use bridge setup in br0 interface
use_bridge_setup = 0         # AP & STA : 0 (not use bridge setup) or n (use bridge setup with eth(n-1))
                             # RELAY : 0 (not use bridge setup) or 1 (use bridge setup with wlan0,wlan1)
##################################################################################
```
You can apply your parameters by updating start.py script file.
```
vi nrc_pkg/script/start.py
```
#### Access Point (AP) running procedure
The following is the parameters for start.py script file.
```
Usage:
        start.py [sta_type] [security_mode] [country] [channel] [sniffer_mode]
        start.py [sta_type] [security_mode] [country] [mesh_mode] [mesh_peering] [mesh_ip]
Argument:
        sta_type      [0:STA   |  1:AP  |  2:SNIFFER  | 3:RELAY |  4:MESH | 5:IBSS]
        security_mode [0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE | 4:WPS-PBC]                
        country       [US:USA  |  JP:Japan  |  TW:Taiwan  | EU:EURO | CN:China |                       
                       AU:Australia  |  NZ:New Zealand  | K1:Korea-USN  | K2:Korea-MIC]                
        -----------------------------------------------------------
        channel       [S1G Channel Number]   * Only for Sniffer & AP
        sniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer
        mesh_mode     [0:MPP | 1:MP | 2:MAP] * Only for Mesh
        mesh_peering  [Peer MAC address]     * Only for Mesh
        mesh_ip       [Static IP address]    * Only for Mesh
        ibss_ip       [0:DHCPC or static IP | 1:DHCPS]    * Only for IBSS
Example:
        OPEN mode STA for US                : ./start.py 0 0 US
        Security mode AP for US                : ./start.py 1 1 US
        Local Sniffer mode on CH 40 for Japan  : ./start.py 2 0 JP 40 0
        SAE mode Mesh AP for US                : ./start.py 4 3 US 2
        Mesh Point with static ip              : ./start.py 4 3 US 1 192.168.222.1                     
        Mesh Point with manual peering         : ./start.py 4 3 US 1 8c:0f:fa:00:29:46                 
        Mesh Point with manual peering & ip    : ./start.py 4 3 US 1 8c:0f:fa:00:29:46 192.168.222.1   
        OPEN mode IBSS for US with DHCP server      : ./start.py 5 0 US 1
        Security mode IBSS for US with DHCPC client : ./start.py 5 1 US 0
        Security mode IBSS for US with static IP    : ./start.py 5 1 US 0 192.168.200.17
Note:
        sniffer_mode should be set as '1' when running sniffer on remote terminal
        MPP, MP mode support only Open, WPA3-SAE security mode
```
The following shows an example of AP running for US channel, and its channel can be configured in nrc_pkg/script/conf/US/ap_halow_open.conf. For WPA2/WPA3 modes, please refer to nrc_pkg/script/conf/US/ap_halow_[sae|owe|wpa2].conf files.
```
cd nrc_pkg/script
./start.py 1 0 US
```
#### Station (STA) running procedure
The following shows an example of STA running for US channel, and its channel can be configured in nrc_pkg/script/conf/US/sta_halow_open.conf. For WPA2/WPA3 modes, please refer to nrc_pkg/script/conf/US/sta_halow_[sae|owe|wpa2].conf files.
```
cd nrc_pkg/script
./start.py 0 0 US
```
#### Sniffer running procedure
The following shows an example of local mode sniffer running for US channel 159.
```
cd nrc_pkg/script
./start.py 2 0 US 159 0
```
