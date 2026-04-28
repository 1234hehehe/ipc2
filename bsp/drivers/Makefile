.PHONY: clean build install

all: clean build install

clean:
	cd audio/oss3/;make clean;cd -;
	cd isp/tx-isp;make clean;cd -;
	cd misc/sensor_info;make clean;cd -;
	cd tnpu_driver;make clean;cd -;
	rm -fr sensors/out;
	rm -fr out;

build:
	cd audio/oss3/;make;
	cd isp/tx-isp;make;
	cd misc/sensor_info;make;
	cd tnpu_driver;make;
	cd sensors/;./mk_all.sh;cd -;

install:
	mkdir -p out;
	cp audio/oss3/audio.ko out/
	cp isp/tx-isp/tx-isp-PRJ008.ko out/tx-isp-t33.ko
	cp misc/sensor_info/sinfo.ko out/
	cp tnpu_driver/tnpu.ko out/
	cp sensors/out/* out/

