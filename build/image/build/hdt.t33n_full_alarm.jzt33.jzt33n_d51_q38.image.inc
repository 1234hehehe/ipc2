
# 被引用的打包脚本需要按照Makefile语法实现这些目标.
# update_fs -- 更新文件系统中的文件.
# mkimg_fs -- 制作文件系统的镜像文件.

FIELNAME:=${buildyear}${buildmonth}${buildday}-${SHORTNAME_NAME}-${PROJECT_NAME}-${HARDWARE_NAME}-${SENSORMODEL_NAME}

HWID_OPTB=$(patsubst %,-b %,${HWID_NAME})

init_fs:
	@echo "Cleaning fs.";
	@rm -rf fs;

update_fs: init_fs update_fs_uboot update_fs_kernel update_fs_appfs

mkimg_fs:
	@cd fs; \
	${MKFLASHFS} -f uboot.bin 256 -f factory.bin 64 -f config.bin 2048 -f kernel.bin 1728 -f rootfs.bin 1344 -f media.bin 1024 -f custom_ro.bin 9920 -o ${RELEASE_COPY_PATH}/${FIELNAME}.flash; \
	${MKFLASHFS} ${HWID_OPTB} -p uboot.bin 0 256 -p kernel.bin 2368 1728 -p rootfs.bin 4096 1344 -p custom_ro.bin 6464 9920 -o ${RELEASE_COPY_PATH}/${FIELNAME}.rpu.update; \
	${MKFLASHFS} ${HWID_OPTB} -p uboot.bin 0 256 -p kernel.bin 2368 1728 -p rootfs.bin 4096 1344 -p media.bin 5440 1024 -p custom_ro.bin 6464 9920 -o ${RELEASE_COPY_PATH}/${FIELNAME}.dingzhi.update; \
	${MKFLASHFS} ${HWID_OPTB} -p uboot.bin 0 256 -p config.bin 320 2048 -p kernel.bin 2368 1728 -p rootfs.bin 4096 1344 -p media.bin 5440 1024 -p custom_ro.bin 6464 9920 -o ${RELEASE_COPY_PATH}/${FIELNAME}.uboot.update; \
	${MKFLASHFS} ${HWID_OPTB} -p custom_ro.bin 6464 9920 -o ${RELEASE_COPY_PATH}/${FIELNAME}.update; \
	cd - > /dev/null;

update_fs_uboot:
	@mkdir -p fs;
	@cp ${ROOT}/bsp/out/uboot.bin fs/uboot.bin

update_fs_kernel:
	@mkdir -p fs;
	@cp ${ROOT}/bsp/out/kernel.bin fs/kernel.bin
	@cp ${ROOT}/bsp/out/rootfs.bin fs/rootfs.bin

update_fs_appfs:
	@cp -af ${APPFS_PATH}/fs/factory.bin fs/factory.bin;
	@cp -af ${APPFS_PATH}/fs/config.bin fs/config.bin;
	@cp -af ${APPFS_PATH}/fs/media.bin fs/media.bin;
	@cp -af ${APPFS_PATH}/fs/custom_ro.bin fs/custom_ro.bin;