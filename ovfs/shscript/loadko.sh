#!/bin/sh

cpuid=`devmem 0x1354021f | cut -c 3-`;

case "$cpuid" in
"00000099")
    # T33L
    cputype="T33L";
    ;;
"00000055")
    # T33ZL
    cputype="T33L";
    ;;
"000000AA")
    # T33N
    cputype="T33N";
    ;;
"00000033")
    # T33N
    cputype="T33N";
    ;;
"00000077")
    # T33ZN
    cputype="T33N";
    ;;
*)
    # NONE
    cputype="";
    ;;
esac

echo $cputype

insmod sinfo.ko


echo 1 > /proc/jz/sinfo/info

SENSOR_INFO=`cat /proc/jz/sinfo/info`
sensortype=`echo ${SENSOR_INFO#*:} | tr 'a-z' 'A-Z'`
echo ${sensortype}

g_sensor_type_1=$(echo $(grep "SensorModelSec" /usr/etc/factoryinfo.json) | sed -e "s/\"//g" -e "s/SensorModelSec://g" | cut -d ',' -f 1 | tr [a-z] [A-Z])

if [ $cputype == "T33L" ] ; then
insmod tx-isp-t33.ko clk_name=mpll isp_clk=325000000 clkv_name=mpll isp_clkv=325000000 direct_mode=1 ivdc_mem_line=1080 ivdc_threshold_line=800
elif [ $cputype == "T33N" ] ; then
insmod tx-isp-t33.ko clk_name=mpll isp_clk=300000000 clkv_name=mpll isp_clkv=300000000
else
	echo "${cputype} not support"
fi
sensorname=`echo ${sensortype} | tr 'A-Z' 'a-z'`
insmod sensor_${sensorname}_t33.ko

insmod audio.ko spk_gpio=-1 fragment_time=4

insmod tnpu.ko  cma_pool_size=0x264000
#insmod tnpu.ko parent_clk_name=vpll

insmod ourdrv/motor.ko

echo "insmod ko ok"

if [ -n "$sensortype" ] ; then
	echo "DetectSensorType=$sensortype"
	g_sensor_type=$(echo $(grep "SensorModel" /usr/etc/factoryinfo.json) | sed -e "s/\"//g" -e "s/SensorModel://g" | cut -d ',' -f 1 | tr [a-z] [A-Z])
	echo ${g_sensor_type}
	if [ $g_sensor_type != $sensortype ] ; then
		sed -i 's#\("SensorModel":\).*#\1"'$sensortype'",#g' /usr/etc/factoryinfo.json
	fi
	if [ $g_sensor_type_1 != "NONE" ] ; then 
		echo "sec ${g_sensor_type_1}"
		insmod sensor_${sensorname}s1_t32.ko
		if [ $g_sensor_type_1 != $sensortype ] ; then
		sed -i 's#\("SensorModelSec":\).*#\1"'$sensortype'",#g' /usr/etc/factoryinfo.json 
		fi
	else
		echo "sec sensor is none"
	fi
fi
