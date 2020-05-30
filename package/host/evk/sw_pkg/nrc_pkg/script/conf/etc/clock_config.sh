#!/bin/bash

RPI3BREV="2082"

if cat /proc/cpuinfo | grep -q $RPI3BREV; then
 CPUMAX=1200000
else
 CPUMAX=1400000
fi

echo $CPUMAX | sudo tee -a /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
echo $CPUMAX | sudo tee -a /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
echo $CPUMAX | sudo tee -a /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
echo $CPUMAX | sudo tee -a /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq

echo "Done"

