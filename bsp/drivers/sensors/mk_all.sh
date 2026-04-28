#/sh/bin
if [ ! -d out ] ; then 
	mkdir -p out
fi
for file in *; do
	echo $file
	if [ -d $file ] ; then 
		cd $file;	
		if [ -f Makefile ] ; then 
			make clean && make && cp sensor_${file}_PRJ008.ko ../out/sensor_${file}_t33.ko;
		fi
		cd ../;
	fi
done
