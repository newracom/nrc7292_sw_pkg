# NRC7292 Software Package for Host mode (Linux OS)

## Notice
### Release roadmap
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
- [NRC7292_SW_PKG_v1.3.4_rev4](https://github.com/newracom/nrc7292_sw_pkg/releases/tag/v1.3.4_rev04)

### Release package contents
- host: NRC7292 software package for global regulatory domains
- host_kr_mic: NRC7292 software package for South Korea MIC frequency regulation
- host_kr_usn: NRC7292 software package for South Korea USN frequency regulation

## NRC7292 Software Package User Guide
### (Optional) Install toolchain
To build host driver, GNU ARM embedded toolchain is needed.
Please follow commands to install toolchain by using toolchain in this repository.
```
tar -xf gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2
vi ~/.bashrc
  export PATH="~/gcc-arm-none-eabi-7-2018-q2-update/bin:$PATH"
```
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
sudo dtc -I dts -O dtb -o newracom.dtbo newracom-overlay.dts
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
cd host/evk/binary
cp ./cli_app ~/nrc_pkg/script
cp ./nrc.ko ~/nrc_pkg/sw/driver
cp ./nrc7292_cspi.bin ~/nrc_pkg/sw/firmware
```
#### (Optional) Build host driver
You can get the pre-built host driver in nrc_pkg/sw/driver, but you can build it by youself by following below commands.
```
cd host/nrc_driver/source/nrc_driver/nrc
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
model             = 7292      # 7292 or 7192
fw_download       = 1         # 0(FW Download off) or 1(FW Download on)
fw_name           = 'uni_s1g.bin'
##################################################################################
# DEBUG Conf.
# NRC Driver log
driver_debug      = 0         # NRC Driver debug option : 0(off) or 1(on)
#--------------------------------------------------------------------------------#
# WPA Supplicant Log (STA Only)
supplicant_debug  = 0         # WPA Supplicant debug option : 0(off) or 1(on)
#--------------------------------------------------------------------------------#
# HOSTAPD Log (AP Only)
hostapd_debug     = 0         # Hostapd debug option    : 0(off) or 1(on)
#################################################################################
# CSPI Conf. (Default)
spi_clock         = 20000000  # SPI Master Clock Frequency
spi_bus_num       = 0         # SPI Master Bus Number
spi_cs_num        = 0         # SPI Master Chipselect Number
spi_gpio_irq      = 5         # CSPI_EIRQ GPIO Number, BBB is 60 recommanded.
spi_gpio_poll     = -1        # CSPI_EIRQ GPIO Polling Interval (if negative, irq mode)
#--------------------------------------------------------------------------------#
# FT232H USB-SPI Conf. (FT232H CSPI Conf)
ft232h_usb_spi    = 0         # FTDI FT232H USB-SPI bridge : 0(off) or 1(on)
#################################################################################
# RF Conf.
# Board Data includes TX Power per MCS and CH
txpwr_val         = 17       # TX Power
txpwr_max_default = 24       # Board Data Max TX Power
bd_download       = 0        # 0(Board Data Download off) or 1(Board Data Download on)
bd_name           = 'nrc7292_bd.dat'
#--------------------------------------------------------------------------------#
# Calibration usage option
#  If this value is changed, the device should be restarted for applying the value
cal_use            = 1       # 0(disable) or 1(enable)
##################################################################################
# PHY Conf.
guard_int         = 'long'   # Guard Interval ('long'(LGI) or 'short'(SGI))
##################################################################################
# MAC Conf.
# AMPDU (Aggregated MPDU)
#  Enable AMPDU for full channel utilization and throughput enhancement
ampdu_enable      = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# 1M NDP (Block) ACK (AP Only)
#  Enable 1M NDP ACK on 2/4MHz BW for robustness (default: 2M NDP ACK on 2/4MH BW)
#  STA should follow, if enabled on AP
#  Note: if enabled, max # of mpdu in ampdu is restricted to 8 (default: max 16)
ndp_ack_1m        = 0        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# NDP Probe Request
#  For STA, "scan_ssid=1" in wpa_supplicant's conf should be set to use
ndp_preq          = 1        # 0 (Legacy Probe Req) 1 (NDP Probe Req)
#--------------------------------------------------------------------------------#
# CQM (Channel Quality Manager) (STA Only)
#  STA can disconnect according to Channel Quality (Beacon Loss or Poor Signal)
#  Note: if disabled, STA keeps connection regardless of Channel Quality
cqm_enable        = 1        # 0 (disable) or 1 (enable)
#--------------------------------------------------------------------------------#
# RELAY (Do NOT use! it will be deprecated)
relay_type        = 1        # 0 (wlan0: STA, wlan1: AP) 1 (wlan0: AP, wlan1: STA)
#--------------------------------------------------------------------------------#
# Power Save (STA Only)
#  4-types PS: (0)Always on (1)Modem_Sleep (2)Deep_Sleep(TIM) (3)Deep_Sleep(nonTIM
#  Modem Sleep : turn off only RF while PS (Fast wake-up but less power save)
#   Deep Sleep : turn off almost all power (Slow wake-up but much more power save)
#     TIM Mode : check beacons during PS to receive BU from AP
#  nonTIM Mode : Not check beacons during PS (just wake up by TX or EXT INT)
power_save        = 0        # STA (power save type 0~3)
ps_timeout        = '3s'     # STA (timeout before going to sleep) (min:1000ms)
sleep_duration    = '3s'     # STA (sleep duration only for nonTIM deepsleep) (min:1000ms)
listen_interval   = 1000     # STA (listen interval in BI unit) (max:65535)
#--------------------------------------------------------------------------------#
# BSS MAX IDLE PERIOD (aka. keep alive) (AP Only)
#  STA should follow (i.e STA should send any frame before period),if enabled on AP
#  Period is in unit of 1000TU(1024ms, 1TU=1024us)
#  Note: if disabled, AP removes STAs' info only with explicit disconnection like deauth
bss_max_idle_enable = 1      # 0 (disable) or 1 (enable)
bss_max_idle        = 180    # time interval (e.g. 60: 614400ms) (1 ~ 65535)
#--------------------------------------------------------------------------------#
# Mesh Options (Mesh Only)
#  SW encryption by MAC80211 for Mesh Point
sw_enc              = 0     # 0 (disable), 1 (enable)
#  Manual Peering & Static IP
peer                = 0     # 0 (disable) or Peer MAC Address
static_ip           = 0     # 0 (disable) or Static IP Address
#--------------------------------------------------------------------------------#
# Self configuration (AP Only)
#  AP scans the clearest CH and then starts with it
self_config       = 0        # 0 (disable)  or 1 (enable)
prefer_bw         = 4        # 0: no preferred bandwidth, 1: 1M, 2: 2M, 4: 4M
dwell_time        = 100      # max dwell is 1000 (ms), min: 10ms, default: 100ms
#--------------------------------------------------------------------------------#
# Credit num of AC_BE for flow control between host and target (Internal use only)
credit_ac_be      = 40        # number of buffer (min: 40, max: 120)
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
        sta_type      [0:STA   |  1:AP  |  2:SNIFFER  | 3:RELAY |  4:MESH]
        security_mode [0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE | 4:WPS-PBC]
        country       [US:USA  |  JP:Japan  |  TW:Taiwan  | EU:EURO | CN:China |
                       AU:Australia  |  NZ:New Zealand]
        -----------------------------------------------------------
        channel       [S1G Channel Number]   * Only for Sniffer
        sniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer
        mesh_mode     [0:MPP | 1:MP | 2:MAP] * Only for Mesh
        mesh_peering  [Peer MAC address]     * Only for Mesh
        mesh_ip       [Static IP address]    * Only for Mesh
Example:
        OPEN mode STA for US                : ./start.py 0 0 US
        Security mode AP for US                : ./start.py 1 1 US
        Local Sniffer mode on CH 40 for Japan  : ./start.py 2 0 JP 40 0
        SAE mode Mesh AP for US                : ./start.py 4 3 US 2
        Mesh Point with static ip              : ./start.py 4 3 US 1 192.168.222.1
        Mesh Point with manual peering         : ./start.py 4 3 US 1 8c:0f:fa:00:29:46
        Mesh Point with manual peering & ip    : ./start.py 4 3 US 1 8c:0f:fa:00:29:46 192.168.222.1
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
