#!/bin/sh
#

LATENCY_TIME=1
SPI_BUS_NUM=3
GPIO_BASE_NUM=500

while getopts "l:s:g:h" option; do
case $option in
	l) 
		LATENCY_TIME=$OPTARG
		;;

	s) 
		SPI_BUS_NUM=$OPTARG
		;;

	g) 
		GPIO_BASE_NUM=$OPTARG
		;;

	h)
		echo "Usage: $0 [options]"
		echo "Options:"
		echo "  -l     set the latency time in msec (default : 1 msec)"
		echo "  -s     set the SPI bus number (default : $SPI_BUS_NUM)"
		echo "  -g     set the GPIO base number (default : $GPIO_BASE_NUM)"
		echo "  -h     print this help"
		exit
		;;

	\?) 
		echo "Invalid Option"
		exit
esac
done

MODULE_PARAMS="latency=$LATENCY_TIME spi_bus_num=$SPI_BUS_NUM gpio_base_num=$GPIO_BASE_NUM"

echo "sudo insmod ./drivers/spi/spi-ft232h/spi-ft232h.ko $MODULE_PARAMS"
sudo insmod ./drivers/spi/spi-ft232h/spi-ft232h.ko $MODULE_PARAMS

ls /sys/class/gpio
ls /sys/class/spi_master
