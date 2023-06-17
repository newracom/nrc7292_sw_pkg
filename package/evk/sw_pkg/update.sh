#!/bin/bash

if [ -d "/home/pi/nrc_pkg_bk" ]; then
 echo "Remove backup folder"
 rm -rf /home/pi/nrc_pkg_bk
fi
if [ -d "/home/pi/nrc_pkg" ]; then
 echo "Backup previous package"
 mv /home/pi/nrc_pkg /home/pi/nrc_pkg_bk
fi
sleep 1

echo "Copy new package"
echo "apply nrc_pkg "
cp -r ./nrc_pkg/  /home/pi/nrc_pkg/

echo "Change mode"
cd /home/pi/nrc_pkg
sudo chmod -R 755 *
sudo chmod -R 777 /home/pi/nrc_pkg/script/*
sudo chmod -R 777 /home/pi/nrc_pkg/sw/firmware/copy
sleep 1

echo "Done"
