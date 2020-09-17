#!/bin/sh
#export KDIR=/mnt/sdb1/qsdk/qca/src/linux-3.14
export KDIR=/mnt/sdb1/qsdk/build_dir/target-arm_cortex-a7_uClibc-1.0.14_eabi/linux-ipq806x/linux-3.14.77
export NRCLINUXHOSTIDR=/mnt/sdb1/qsdk/build_dir/target-arm_cortex-a7_uClibc-1.0.14_eabi/linux-ipq806x/backports-20161201-3.14.77-9ab3068
export ARCH=arm
export STAGING_DIR=/mnt/sdb1/qsdk/staging_dir
export CROSS_COMPILE=/mnt/sdb1/qsdk/staging_dir/toolchain-arm_cortex-a7_gcc-4.8-linaro_uClibc-1.0.14_eabi/bin/arm-openwrt-linux-
echo $KDIR
echo $NRCLINUXHOSTIDR
echo $STAGING_DIR
echo $ARCH
echo $CROSS_COMPILE
make
