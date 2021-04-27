# NRC7292 Software Package

## Notice
### Release roadmap
- v1.3.3 (2021.04.26)
- v1.3.2 (2020.09.16)
- v1.3.1 (2020.08.05)
- v1.3.0 (2020.05.30)

### Latest release
- [NRC7292_SW_PKG_v1.3.3](https://github.com/newracom/nrc7292_sw_pkg/releases/tag/v1.3.3)

### Release package contents
- host
  - cli_app
  - doc
  - evk
  - nrc_driver
  - tools
- host_kr_mic
  - cli_app
  - doc
  - evk
  - nrc_driver
  - tools

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
txpwr_val   = 17       # TX Power
maxagg_num  = 8        # 0(AMPDU off) or  >2(AMPDU on)
cqm_off     = 0        # 0(CQM on) or 1(CQM off)
fw_name     = 'uni_s1g.bin'
guard_int   = 'long'   # 'long'(LGI) or 'short'(SGI)
supplicant_debug = 0   # WPA Supplicant debug option : 0(off) or 1(on)
hostapd_debug    = 0   # Hostapd debug option    : 0(off) or 1(on)
power_save       = 0   # power save : 0(off) or 1(on)
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
Argument:
        sta_type      [0:STA   |  1:AP  |  2:SNIFFER  | 3:RELAY]
        security_mode [0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE]
        country       [US:USA  |  JP:Japan  |  TW:Taiwan  | KR:Korea | EU:EURO | CN:China]
        -----------------------------------------------------------
        channel       [S1G Channel Number]   * Only for Sniffer
        sniffer_mode  [0:Local | 1:Remote]   * Only for Sniffer
Example:
        OPEN mode STA for Korea                : ./start.py 0 0 KR
        Security mode AP for US                : ./start.py 1 1 US
        Local Sniffer mode on CH 40 for Japan  : ./start.py 2 0 JP 40 0
Note:
        sniffer_mode should be set as '1' when running sniffer on remote terminal
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
