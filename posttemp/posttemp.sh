#!/bin/sh

# Linux sysfs USB structures: http://www.linux-usb.org/FAQ.html#i6

scriptname=`basename $0`
scriptdir=${0/$scriptname/}
logfile=$scriptdir/$scriptname.log

source $scriptdir/$scriptname-config
source $redirectlogscript

currts=`date +%s`

echo "reading $context1..."
echo "  extracting busnum and devnum for usb location $context1usblocation..."
busnum=`cat /sys/bus/usb/devices/$context1usblocation/busnum`
devnum=`cat /sys/bus/usb/devices/$context1usblocation/devnum`
echo "  got busname $busnum and devnum $devnum, reading."
value=`$pcsensor2 -q -b $busnum -d $devnum`
if [ ! -z "$value" ]; then
	echo -n "  got value $value, posting..."
	curl -s "$posturl/posttemp.php?p=$password&c=$context1&t=$currts&v=$value"
fi

echo "reading $context2..."
echo "  extracting busnum and devnum for usb location $context2usblocation..."
busnum=`cat /sys/bus/usb/devices/$context2usblocation/busnum`
devnum=`cat /sys/bus/usb/devices/$context2usblocation/devnum`
echo "  got busname $busnum and devnum $devnum, reading."
value=`$pcsensor2 -q -b $busnum -d $devnum`
if [ ! -z "$value" ]; then
	echo -n "  got value $value, posting..."
	curl -s "$posturl/posttemp.php?p=$password&c=$context2&t=$currts&v=$value"
fi

$logrotateifneeded $logfile
