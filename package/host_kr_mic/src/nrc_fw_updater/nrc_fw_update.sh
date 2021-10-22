#!/bin/bash

version=$1
md5sum=$2
location=$3
sdk_root=$4
station_type=$5
security_mode=$6
country=$7

echo "version: $version"
echo "md5sum: $md5sum"
echo "firmware location $location"
echo "sdk root: $sdk_root"
echo "station type : $station_type"
echo "security mode : $security_mode"
echo "country : $country"

fw_downloaded="/tmp/nrc_firmware.tar.gz"
extracted="/tmp/nrc_fw_extracted"
sdk_version_file="$sdk_root/VERSION-SDK.txt"

driver_dir=$sdk_root/sw/driver
firmware_dir=$sdk_root/sw/firmware
app_dir=$sdk_root/script

module_file=nrc.ko
bd_data_file=nrc7292_bd.dat
cspi_fw_file=nrc7292_cspi.bin
cli_app_file=cli_app

update_version_file() {
	cat <<EOF > $sdk_version_file
VERSION_MAJOR $1
VERSION_MINOR $2
VERSION_REVISION $3
EOF
}

wget $location -O $fw_downloaded

if [ $? -eq 0 ]; then
	md5sum_downloaded=$(md5sum $fw_downloaded | cut -f1 -d ' ')
	if [ "${md5sum}" = "${md5sum_downloaded}" ]; then
		echo "MD5SUM matches"
		mkdir -p $extracted
		zcat $fw_downloaded | tar -C $extracted -x
		if [ $? -eq 0 ]; then
			# copy files to appropriate folders
			echo "copying files to locations..."
			cp $extracted/$module_file $driver_dir/ 2> /dev/null
			if [ $? -eq 1 ]; then
				echo "$module_file not found..."
				exit 1
			fi
			cp $extracted/$bd_data_file $firmware_dir/ 2> /dev/null
			if [ $? -eq 1 ]; then
				echo "$bd_data_file not found..."
				exit 1
			fi

			cp $extracted/$cspi_fw_file $firmware_dir/ 2> /dev/null
			if [ $? -eq 1 ]; then
				echo "$cspi_fw_file not found..."
				exit 1
			fi

			cp $extracted/$cli_app_file  $app_dir/ 2> /dev/null
			if [ $? -eq 1 ]; then
				echo "$cli_app_file not found..."
				exit 1
			fi

			echo "Update $sdk_version_file..."
			major=$(echo $version | cut -f1 -d.)
			minor=$(echo $version | cut -f2 -d.)
			revision=$(echo $version | cut -f3 -d.)
			echo "$major $minor $revision"
			update_version_file $major $minor $revision

			echo "Restarting wifi..."
			pushd $app_dir
			./stop.py
			./start.py $station_type $security_mode $country
			popd
		fi
	else
		echo "Downloaded FW MD5SUM error"
	fi
else
	echo "Firmware download failed"
fi

exit 0
