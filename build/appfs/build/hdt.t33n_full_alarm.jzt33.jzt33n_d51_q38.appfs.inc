
# 被引用的打包脚本需要按照Makefile语法实现这些目标.
# update_fs -- 更新文件系统中的文件.
# update_ver -- 更新文件系统的版本.
# mkimg_fs -- 制作文件系统的镜像文件.

ISDOME=1

init_fs:
	@echo "Cleaning fs.";
	@rm -rf fs && mkdir -p fs/custom;

update_fs: init_fs update_fs_config mkimg_fs_config \
	update_fs_ko tar_fs_ko update_fs_bin tar_fs_bin update_fs_lib tar_fs_lib \
	update_fs_custom_smart update_fs_media

update_ver: edit_fs_config mkimg_fs_config
	@[ -d $(shell dirname ${CUSTOM_PATH}) ] || mkdir -p $(shell dirname ${CUSTOM_PATH});
	@[ -d $(shell dirname ${MEDIA_PATH}) ] || mkdir -p $(shell dirname ${MEDIA_PATH});
	@echo "SwVersion=\"V4.3\"" > ${CUSTOM_PATH}/version;
	@echo "HwVersion=\"V4.0\"" >> ${CUSTOM_PATH}/version;
	@echo "Hardware=\"${HARDWARE_NAME}\"" >> ${CUSTOM_PATH}/version;
	@echo "BuildDate=\"${buildyear}${buildmonth}${buildday}\"" >> ${CUSTOM_PATH}/version;
#   @echo "ProductDate=\"${buildyear}${buildmonth}${buildday}\"" >> ${CUSTOM_PATH}/version;
	@echo "LensSupport=${AUTOLENS_SUPPORT}" >> ${CUSTOM_PATH}/version;
	@echo "LensDrvType=\"${AUTOLENS_DRIVE}\"" >> ${CUSTOM_PATH}/version;
	@echo "LensType=\"${AUTOLENS_TYPE}\"" >> ${CUSTOM_PATH}/version;
	@echo "IsofDome=\"${ISDOME}\"" >> ${CUSTOM_PATH}/version;

	@cat fs/config/factoryinfo.json | sed 's/,/\n/g' | grep "ProductDate" | sed 's/"ProductDate"/\ProductDate/g' | sed 's/:/\=/g' | sed 's/[[:space:]]//g' >> ${CUSTOM_PATH}/version;

mkimg_fs: mkimg_fs_config tar_fs_ko tar_fs_bin tar_fs_lib mkimg_fs_custom_smart mkimg_fs_media tar_appfs finish_fs

KO_PATH:=fs/ko

BIN_PATH:=fs/bin

LIB_PATH:=fs/lib

CUSTOM_PATH:=fs/custom

MEDIA_PATH:=fs/media

#### 更新config,factory目录文件 ############
load_fs_config:
	@echo "Preparing fs/config";
	@mkdir -p fs/config;
	@cp ${ROOT}/ovfs/shscript/factoryinfo.json fs/config
	@sed -i -e 's#\("LensSupport":\).*[0-9]#\1 ${AUTOLENS_SUPPORT}#g' \
	-e 's#\("LensDrvType":\).*\"#\1"'${AUTOLENS_DRIVE}'"#g' \
	-e 's#\("LensType":\).*\"#\1"'${AUTOLENS_TYPE}'"#g' \
	-e 's#\("Customer":\).*\"#\1"'${CUSTOMER_NAME}'"#g' \
	-e 's#\("Project":\).*\"#\1"'${PROJECT_NAME}'"#g' \
	-e 's#\("Hardware":\).*\"#\1"'${HARDWARE_NAME}'"#g' \
	-e 's#\("SensorModel":\).*\"#\1"'${SENSORMODEL_NAME}'"#g' \
	-e 's#\("Platform":\).*#\1"'${PLATFORM_NAME}'",#g' fs/config/factoryinfo.json;

	@cp ${ROOT}/ovfs/bin/ntsclientcon fs/config/ntsclientcon
	@cp ${ROOT}/ovfs/bin/umeye4ovfs fs/config/umeye4ovfs
	@cp ${ROOT}/ovfs/shscript/fps_param.json fs/config/fps_param.json

update_productdate:
	@sed -i -e 's#\("ProductDate":\).*\"#\1"'${buildyear}${buildmonth}${buildday}-m'"#g' fs/config/factoryinfo.json;

edit_fs_config:
	@sed -i -e 's#\("BuildDate":\).*#\1"'${buildyear}${buildmonth}${buildday}'",#g' \
	-e 's#\("SensorModel":\).*\"#\1"'${SENSORMODEL_NAME}'"#g' fs/config/factoryinfo.json;
	
update_fs_config: load_fs_config edit_fs_config update_productdate

mkimg_fs_config:
	${MKJFFS2IMG} -l -e 0x8000 -d fs/config -o fs/config.bin;
	@mkdir -p fs/factory && touch fs/factory/demo.txt
	${MKJFFS2IMG} -l -e 0x10000 -d fs/factory -o fs/factory.bin;
	
#### 更新ko目录文件 ############
update_fs_ko:
	@echo "Preparing ${KO_PATH}";
	@mkdir -p ${KO_PATH};
	@cp ${ROOT}/bsp/out/ko/sinfo.ko ${KO_PATH}/sinfo.ko
	@cp ${ROOT}/bsp/out/ko/audio.ko ${KO_PATH}/audio.ko
	@cp ${ROOT}/bsp/out/ko/tx-isp-t33.ko ${KO_PATH}/tx-isp-t33.ko
	@cp ${ROOT}/bsp/out/ko/sensor_gc5603_t33.ko ${KO_PATH}/sensor_gc5603_t33.ko
	@cp ${ROOT}/bsp/out/ko/sensor_gc4653_t33.ko ${KO_PATH}/sensor_gc4653_t33.ko
	@cp ${ROOT}/bsp/out/ko/sensor_gc2083_t33.ko ${KO_PATH}/sensor_gc2083_t33.ko
	@cp ${ROOT}/bsp/out/ko/tnpu.ko ${KO_PATH}/tnpu.ko
	@mkdir -p ${KO_PATH}/ourdrv;
	@cp ${ROOT}/ovfs/shscript/loadko.sh ${KO_PATH}/loadko.sh
	@chmod a+x ${KO_PATH}/loadko.sh;

tar_fs_ko:
	tar -C fs --xz -Jcf fs/custom/ko.tar.xz ko --totals

#### 更新bin目录文件 ############
update_fs_bin:
	@echo "Preparing ${BIN_PATH}";
	@mkdir -p ${BIN_PATH};
	@cp ${ROOT}/appsrc/pub/jzt33/bin/ntpclient ${BIN_PATH}/ntpclient
	@cp ${ROOT}/ovfs/bin/ovfs_boardsystem ${BIN_PATH}/ovfs_boardsystem
	@cp ${ROOT}/ovfs/bin/gpiotool ${BIN_PATH}/gpiotool

	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_core_2063 ${BIN_PATH}/ovfs_core
	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_mediaserver ${BIN_PATH}/ovfs_mediaserver
	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_network ${BIN_PATH}/ovfs_network
#	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_access_host ${BIN_PATH}/ovfs_access_host
	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_record ${BIN_PATH}/ovfs_record

	@cp ${ROOT}/ovfs/bin/mkfs.ext4 ${BIN_PATH}/mkfs.ext4
	@cp ${ROOT}/ovfs/bin/fsck.ext4 ${BIN_PATH}/fsck.ext4

	@cp ${ROOT}/appsrc/pub/jzt33/bin/pureupdate ${BIN_PATH}/update
	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_webserver ${BIN_PATH}/ovfs_webserver
	@cp ${ROOT}/appsrc/pub/jzt33/bin/uritool ${BIN_PATH}/uritool

	@cp ${ROOT}/appsrc/pub/jzt33/bin/ovfs_eventlog ${BIN_PATH}/ovfs_eventlog

	@cp ${ROOT}/appsrc/pub/jzt33/bin/sysinit ${BIN_PATH}/sysinit

	@cp ${ROOT}/ovfs/bin/autoupgrade fs/custom/autoupgrade
	@cp ${ROOT}/ovfs/shscript/udhcpc.script ${BIN_PATH}/udhcpc.script
	@cp ${ROOT}/ovfs/shscript/udhcpc6.script ${BIN_PATH}/udhcpc6.script

	@cp ${ROOT}/appsrc/pub/jzt33/bin/wtdgops ${BIN_PATH}/wtdgops

	@cp ${ROOT}/ovfs/bin/impdbg ${BIN_PATH}/impdbg
	@cp ${ROOT}/ovfs/bin/logcat ${BIN_PATH}/logcat

	@chmod a+x -R fs/bin;
	@chmod a+x -R fs/bin/*;

tar_fs_bin:
	@cp -af fs/bin ${CUSTOM_PATH}/;
	
#### 更新lib目录文件 ############
update_fs_lib:
	@echo "Preparing ${LIB_PATH}";
	@mkdir -p ${LIB_PATH};
	@cp ${ROOT}/appsrc/pub/jzt33/lib/libaccess.so ${LIB_PATH}/libaccess.so
	@cp ${ROOT}/appsrc/pub/jzt33/lib/libcommon.so ${LIB_PATH}/libcommon.so
	@cp ${ROOT}/appsrc/pub/jzt33/lib/libcurl.so ${LIB_PATH}/libcurl.so
	@cp -r ${ROOT}/appsrc/pub/jzt33/lib/libssl.s* ${LIB_PATH}/.
	@cp -r ${ROOT}/appsrc/pub/jzt33/lib/libcrypto.s* ${LIB_PATH}/.
	@cp ${ROOT}/appsrc/pub/jzt33/lib/libsqlite3.so ${LIB_PATH}/libsqlite3.so
	@cp ${ROOT}/appsrc/pub/jzt33/lib/libmodule.so ${LIB_PATH}/libmodule.so
	@cp -r ${ROOT}/appsrc/pub/jzt33/lib/libmxml* ${LIB_PATH}/.

	@cp ${ROOT}/ovfs/lib/libalog.so ${LIB_PATH}/libalog.so
	@cp ${ROOT}/ovfs/lib/libaudioProcess.so ${LIB_PATH}/libaudioProcess.so
	@cp ${ROOT}/ovfs/lib/libimp.so ${LIB_PATH}/libimp.so
	@cp ${ROOT}/ovfs/lib/libsysutils.so ${LIB_PATH}/libsysutils.so

	@cp ${ROOT}/ovfs/lib/libboardsys_smart.so ${LIB_PATH}/libboardsys_smart.so
	@cp ${ROOT}/ovfs/lib/libfont.so ${LIB_PATH}/libfont.so

	@cp ${ROOT}/ovfs/lib/libosdnvrfont.so ${LIB_PATH}/libosdnvrfont.so
	@cp ${ROOT}/ovfs/lib/libptz_sdk.so ${LIB_PATH}/libptz_sdk.so
	@cp ${ROOT}/ovfs/lib/libsp_modbus.so ${LIB_PATH}/libsp_modbus.so

	@cp ${ROOT}/ovfs/lib/libmultiobjectDet_inf.so ${LIB_PATH}/libmultiobjectDet_inf.so
	@cp ${ROOT}/ovfs/lib/libaie.so ${LIB_PATH}/libaie.so
	@cp ${ROOT}/ovfs/lib/liburanus_core.so ${LIB_PATH}/liburanus_core.so
	@cp ${ROOT}/ovfs/lib/liburanus_extend.so ${LIB_PATH}/liburanus_extend.so
	@cp ${ROOT}/ovfs/lib/liburanus_tensor.so ${LIB_PATH}/liburanus_tensor.so

	@cp ${ROOT}/appsrc/pub/jzt33/lib/librestcomm.so ${LIB_PATH}/librestcomm.so
	@cp ${ROOT}/appsrc/pub/jzt33/lib/librtspserver_v2.so ${LIB_PATH}/librtspserver_v2.so

	@cp ${ROOT}/appsrc/pub/jzt33/lib/libonvif.so ${LIB_PATH}/libonvif.so

	@cp ${ROOT}/appsrc/pub/jzt33/lib/libstreamqueue.so ${LIB_PATH}/libstreamqueue.so
	@cp ${ROOT}/appsrc/pub/jzt33/lib/libz.so ${LIB_PATH}/libz.so
	mips-t33-linux-uclibc-gnu-strip ${LIB_PATH}/*

	@ln -sf libcurl.so				${LIB_PATH}/libcurl.so.4;

tar_fs_lib:
	@cp -af fs/lib ${CUSTOM_PATH}/;

#### 更新custom目录文件 ############
update_fs_custom_smart:
	@echo "Preparing ${CUSTOM_PATH}";
	@mkdir -p ${CUSTOM_PATH};
	@cp ${ROOT}/ovfs/shscript/appinstall ${CUSTOM_PATH}/appinstall
	@cp ${ROOT}/ovfs/shscript/update_factoryinfo.sh ${CUSTOM_PATH}/update_factoryinfo.sh
	@cp ${ROOT}/ovfs/shscript/mount_mmc_raw ${CUSTOM_PATH}/mount_mmc
	@cp ${ROOT}/ovfs/shscript/update_pre ${CUSTOM_PATH}/update_pre
	@cp ${ROOT}/ovfs/shscript/audio_param.json ${CUSTOM_PATH}/audio_param.json
	@cp ${ROOT}/ovfs/shscript/enc_param.json ${CUSTOM_PATH}/enc_param.json

	@cp ${ROOT}/ovfs/lib/libisp_gc5603.so ${CUSTOM_PATH}/libisp_gc5603.so
	@cp ${ROOT}/ovfs/lib/libisp_gc4653.so ${CUSTOM_PATH}/libisp_gc4653.so
	@cp ${ROOT}/ovfs/lib/libisp_gc2083.so ${CUSTOM_PATH}/libisp_gc2083.so

	mkdir -p ${CUSTOM_PATH}/host_libs;

	mkdir -p ${CUSTOM_PATH}/protocol;
	@cp ${ROOT}/ovfs/lib/libsp_pelcoD.so ${CUSTOM_PATH}/protocol/libsp_pelcoD.so
	@cp ${ROOT}/ovfs/lib/libsp_pelcoP.so ${CUSTOM_PATH}/protocol/libsp_pelcoP.so

	@mkdir -p ${CUSTOM_PATH}/etc/sensor/;
	@cp ${ROOT}/ovfs/model/gc5603-PRJ008-day.bin ${CUSTOM_PATH}/etc/sensor/gc5603-PRJ008-day.bin
	@cp ${ROOT}/ovfs/model/gc5603-PRJ008-night.bin ${CUSTOM_PATH}/etc/sensor/gc5603-PRJ008-night.bin
	@cp ${ROOT}/ovfs/model/gc5603-PRJ008-cust.bin ${CUSTOM_PATH}/etc/sensor/gc5603-PRJ008-cust.bin
	@cp ${ROOT}/ovfs/model/gc4653-PRJ008-day.bin ${CUSTOM_PATH}/etc/sensor/gc4653-PRJ008-day.bin
	@cp ${ROOT}/ovfs/model/gc4653-PRJ008-night.bin ${CUSTOM_PATH}/etc/sensor/gc4653-PRJ008-night.bin
	@cp ${ROOT}/ovfs/model/gc4653-PRJ008-cust.bin ${CUSTOM_PATH}/etc/sensor/gc4653-PRJ008-cust.bin
	@cp ${ROOT}/ovfs/model/gc2083-PRJ008-day.bin ${CUSTOM_PATH}/etc/sensor/gc2083-PRJ008-day.bin
	@cp ${ROOT}/ovfs/model/gc2083-PRJ008-night.bin ${CUSTOM_PATH}/etc/sensor/gc2083-PRJ008-night.bin
	@cp ${ROOT}/ovfs/model/gc2083-PRJ008-cust.bin ${CUSTOM_PATH}/etc/sensor/gc2083-PRJ008-cust.bin

	@cp ${ROOT}/ovfs/shscript/webrtc_profile.ini ${CUSTOM_PATH}/etc/webrtc_profile.ini

	@cp -ar ${ROOT}/ovfs/nginx ${CUSTOM_PATH}/
	@cp ${ROOT}/appsrc/pub/jzt33/bin/nginx ${CUSTOM_PATH}/nginx/sbin/nginx;
	@mv ${CUSTOM_PATH}/nginx/html/static/images/logo.png ${CUSTOM_PATH}/nginx/html/static/images/logo_default.png;
	@ln -sf /usr/etc/logo.png ${CUSTOM_PATH}/nginx/html/static/images/logo.png
	@ln -sf /tmp/nginx/logs ${CUSTOM_PATH}/nginx/logs
	@ln -sf /tmp/nginx/proxy_temp ${CUSTOM_PATH}/nginx/proxy_temp
	@ln -sf /tmp/nginx/fastcgi_temp ${CUSTOM_PATH}/nginx/fastcgi_temp
	@ln -sf /dev/exportcfg.tgz ${CUSTOM_PATH}/nginx/html/exportcfg.tgz
	@ln -sf /tmp/mmc/mmc1/log/AlarmPic ${CUSTOM_PATH}/nginx/html/AlarmPic

	@mkdir -p ${CUSTOM_PATH}/model_file;
	@cp ${ROOT}/ovfs/model/personvehicleDet-800K-block.mgk ${CUSTOM_PATH}/model_file/model.mgk

	@mkdir -p ${CUSTOM_PATH}/font;
	@cp ${ROOT}/ovfs/font/servicestatus ${CUSTOM_PATH}/font/servicestatus
	@cp ${ROOT}/ovfs/font/syfont-normal.ttf ${CUSTOM_PATH}/font/syfont.ttf

	@ln -sf /media/res ${CUSTOM_PATH}/res;

	@ln -sf /media/soundFile ${CUSTOM_PATH}/soundFile;

	@chmod a+x -R fs/custom;

#### 更新media目录文件 ############
update_fs_media:
	@echo "Preparing ${MEDIA_PATH}";
	@mkdir -p ${MEDIA_PATH}/res/custom
	@mkdir -p ${MEDIA_PATH}/res/default
	@cp ${ROOT}/ovfs/custom/* ${MEDIA_PATH}/res/custom/
	@cp ${ROOT}/ovfs/cfgfiles/* ${MEDIA_PATH}/res/default/
	@cp -af fs/config/factoryinfo.json ${MEDIA_PATH}/res;

	@cp -ar ${ROOT}/ovfs/soundfile ${MEDIA_PATH}/

	@mkdir ${MEDIA_PATH}/web;
	@mv ${CUSTOM_PATH}/nginx/html/static/images/logo_default.png ${MEDIA_PATH}/web/logo.png
	@mv ${CUSTOM_PATH}/nginx/html/static/lang ${MEDIA_PATH}/web/
	@ln -sf /media/web/lang ${CUSTOM_PATH}/nginx/html/static/lang

	@chmod a+x -R fs/custom;
mkimg_fs_media:
	rm -rf fs/media.bin; ${MKSQUASHFSIMG} ${MEDIA_PATH} fs/media.bin -b 64K -comp xz -no-xattrs;

mkimg_fs_custom_smart:
	rm -rf fs/custom_ro.bin; ${MKSQUASHFSIMG} fs/custom fs/custom_ro.bin -b 64k -comp xz -no-xattrs;

tar_appfs:
	@cp -a fs/custom fs/update && rm -fr fs/update/soundFile && tar -czf fs/appfs.tgz -C fs update && rm -fr fs/update;

finish_fs:
	@printf "finish fs\n"
	@mkdir -p ${IMAGE_PATH}/fs;
	@cp -af fs/appfs.tgz ${IMAGE_PATH}/fs/appfs.tgz;
	@cp -af fs/factory.bin ${IMAGE_PATH}/fs/factory.bin;
	@cp -af fs/config.bin ${IMAGE_PATH}/fs/config.bin;
	@cp -af fs/media.bin ${IMAGE_PATH}/fs/media.bin;
	@cp -af fs/custom_ro.bin ${IMAGE_PATH}/fs/custom_ro.bin;
