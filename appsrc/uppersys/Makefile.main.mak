
# 根据实际情况添加头文件路径
# 自动搜索和添加的搜索路径如下定义.
INC_PATH_+=${CUR_PATH} ${CUR_PATH}/inc ${INC_PATH_OWN} ${IN_PATH}/inc ${ROOT_PATH}/in/inc/intern ${ROOT_PATH}/in/inc/extern ${OUT_PATH}/inc ${PUB_PATH}/inc
INC_PATH=$(strip $(foreach path,${INC_PATH_},$(shell test -d ${path} && echo ${path})))

# 根据实际情况添加库文件路径
# 自动搜索和添加的搜索路径如下定义.
LIB_PATH_+=${CUR_PATH} ${CUR_PATH}/lib ${IN_PATH}/lib ${ROOT_PATH}/in/lib ${OUT_PATH}/lib ${LIB_PATH_OWN} ${PUB_PATH}/lib
LIB_PATH=$(strip $(foreach path,${LIB_PATH_},$(shell test -d ${path} && echo ${path})))

## <推断>
INC_PATH_FLAGS+=$(patsubst %,-I%,${INC_PATH})
LIB_PATH_FLAGS+=$(patsubst %,-L%,${LIB_PATH})

CFLAGS:=${CPU_FLAGS}
CFLAGS+=${OPT_FLAGS} ${OPT_FLAGS_OWN}
CFLAGS+=${MAC_FLAGS} ${MAC_FLAGS_OWN}
CFLAGS+=${INC_PATH_FLAGS}
#CFLAGS+=${LIB_PATH_FLAGS}
#CFLAGS+=${LNK_FLAGS_OWN}
CFLAGS_LINK=${LIB_PATH_FLAGS} ${LNK_FLAGS_OWN} ${LIB_FILE_OWN}


##<source and objects>
SUBDIRS=$(shell find . -type d | sed -e "s/^\.\/*//")
FILT=$(foreach dir,${SUBDIRS},$(if $(wildcard ${dir}/[Mm]akefile),${dir} ${dir}/%))
SRCDIRS=$(filter-out ${FILT},${SUBDIRS})

OBJS_C=$(patsubst %.c,%.o,$(shell find . -maxdepth 1 -name "*.c"))
OBJS_CPP=$(patsubst %.cpp,%.o,$(shell find . -maxdepth 1 -name "*.cpp"))
OBJS_C+=$(foreach dir,${SRCDIRS},$(patsubst %.c,%.o,$(shell find ${dir} -name "*.c")))
OBJS_CPP+=$(foreach dir,${SRCDIRS},$(patsubst %.cpp,%.o,$(shell find ${dir} -name "*.cpp")))
OBJS_:=$(patsubst ./%,%,${OBJS_C} ${OBJS_CPP})


# 过滤obj目标文件
ifneq (${INCLUDE_OBJS}x,x)
OBJS:=$(filter ${INCLUDE_OBJS},${OBJS_})
else ifneq (${EXCLUDE_OBJS}x,x)
OBJS=$(filter-out ${EXCLUDE_OBJS},${OBJS_})
else
OBJS:=${OBJS_}
endif

# 增加前缀,区分不同平台.
OBJS:=$(patsubst %.o,objs/${PLATFORM_EIGEN}/%.o,${OBJS})

##<dependence tree>
MKFILES=$(shell find . -mindepth 2 -maxdepth 2 -type f -name "[Mm]akefile" | sed -e "s/^\.\/*//" | sort)
DEPTREE=$(if ${MKFILES},$(foreach file,${MKFILES},$(shell dirname ${file})))
DEPTREE_BUILD=$(if ${DEPTREE},$(patsubst %,%,${DEPTREE}))
DEPTREE_CLEAN=$(if ${DEPTREE},$(patsubst %,%_clean,${DEPTREE}))
DEPTREE_DISTCLEAN=$(if ${DEPTREE},$(patsubst %,%_distclean,${DEPTREE}))

ifeq (${STRIPED},n)
define COMPILE_DLIB
	$(call COMPILE_AND_PRINT,($(CXX) $(CFLAGS) -rdynamic -Wl,--gc-sections -o $@ -shared $^ ${CFLAGS_LINK} ${LIBS_LINK}))
endef
else
define COMPILE_DLIB
	$(call COMPILE_AND_PRINT,($(CXX) $(CFLAGS) -rdynamic -Wl,--gc-sections -o $@ -shared $^ ${CFLAGS_LINK} ${LIBS_LINK} && ${STRIP} $@))
endef
endif

ifeq (${STRIPED},n)
define COMPILE_EXE
	$(call COMPILE_AND_PRINT,($(CXX) $(CFLAGS) -Wl,--gc-sections -o $@ $^ ${CFLAGS_LINK} ${LIBS_LINK}))
endef
else
define COMPILE_EXE
	$(call COMPILE_AND_PRINT,($(CXX) $(CFLAGS) -Wl,--gc-sections -o $@ $^ ${CFLAGS_LINK} ${LIBS_LINK} && ${STRIP} $@))
endef
endif

define COMPILE_SLIB
	$(call COMPILE_AND_PRINT,$(LINK) $@ $^)
endef

.PHONY: clean build ${DEPTREE_BUILD}

# 如果存在Makefile.extra.mak,就包含该文件.该文件描述模块间的依赖关系.
ifeq ($(shell test -f ${CUR_PATH}/Makefile.extra.mak && echo true), true)
build: extra_build_pretarget ${DEPTREE_BUILD} self_build export_files extra_build_posttarget
clean: extra_clean_pretarget ${DEPTREE_CLEAN} self_clean extra_clean_posttarget
distclean: extra_clean_pretarget ${DEPTREE_DISTCLEAN} self_distclean extra_clean_posttarget
include ${CUR_PATH}/Makefile.extra.mak
else
build: ${DEPTREE_BUILD} self_build export_files
clean: ${DEPTREE_CLEAN} self_clean
distclean: ${DEPTREE_DISTCLEAN} self_distclean
endif

##<target>
ifeq (${TARGET_TYPE},none)
#nothing
export_files:

# 编译可执行文件
else ifeq (${TARGET_TYPE},exe)
TARGET_EXE:=objs/${PLATFORM_EIGEN}/${TARGET_NAME}
${TARGET_EXE}:${OBJS}
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_EXE)
export_files:
	@mkdir -p ${PUB_PATH}/bin
	cp -af ${TARGET_EXE} ${PUB_PATH}/bin

# 编译库文件
else ifeq (${TARGET_TYPE},lib)
TARGET_SLIB:=objs/${PLATFORM_EIGEN}/lib${TARGET_NAME}.a
TARGET_DLIB:=objs/${PLATFORM_EIGEN}/lib${TARGET_NAME}.so
${TARGET_SLIB}:${OBJS}
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_SLIB)
${TARGET_DLIB}:${OBJS}
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_DLIB)
export_files:
	@mkdir -p ${PUB_PATH}/lib ${PUB_PATH}/inc
	cp -af ${TARGET_SLIB} ${PUB_PATH}/lib
	cp -af ${TARGET_DLIB} ${PUB_PATH}/lib
	@if [ -n "${INC_FILE_EXPORT}" ] ; then echo "cp -af ${INC_FILE_EXPORT} ${PUB_PATH}/inc"; cp -af ${INC_FILE_EXPORT} ${PUB_PATH}/inc; fi

# 编译静态库文件
else ifeq (${TARGET_TYPE},libs)
TARGET_SLIB:=objs/${PLATFORM_EIGEN}/lib${TARGET_NAME}.a
${TARGET_SLIB}:${OBJS}
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_SLIB)
export_files:
	@mkdir -p ${PUB_PATH}/lib ${PUB_PATH}/inc
	cp -af ${TARGET_SLIB} ${PUB_PATH}/lib
	@if [ -n "${INC_FILE_EXPORT}" ] ; then echo "cp -af ${INC_FILE_EXPORT} ${PUB_PATH}/inc"; cp -af ${INC_FILE_EXPORT} ${PUB_PATH}/inc; fi

# 编译动态库文件
else ifeq (${TARGET_TYPE},libd)
TARGET_DLIB:=objs/${PLATFORM_EIGEN}/lib${TARGET_NAME}.so
${TARGET_DLIB}:${OBJS}
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_DLIB)
export_files:
	@mkdir -p ${PUB_PATH}/lib ${PUB_PATH}/inc
	cp -af ${TARGET_DLIB} ${PUB_PATH}/lib
	@if [ -n "${INC_FILE_EXPORT}" ] ; then echo "cp -af ${INC_FILE_EXPORT} ${PUB_PATH}/inc"; cp -af ${INC_FILE_EXPORT} ${PUB_PATH}/inc; fi

endif


${DEPTREE_BUILD}:
#	@echo Building $@
	@make -C $(patsubst %,%,$@) build

${DEPTREE_CLEAN}:
	@echo Cleaning "$(patsubst %_clean,%,$@)"
	@make -C $(patsubst %_clean,%,$@) clean
	
${DEPTREE_DISTCLEAN}:
	@echo Cleaning "$(patsubst %_distclean,%,$@)"
	@make -C $(patsubst %_distclean,%,$@) distclean
	
build_info:
	@echo -e "${GREEN}Building target ${TARGET_TYPE} ${TARGET_NAME} with compiler ${COMPILER_PREFIX}...${NORMAL}"
self_build: build_info ${TARGET_SLIB} ${TARGET_DLIB} ${TARGET_EXE}
self_clean:
	@rm -rf ${OBJS} ${TARGET_SLIB} ${TARGET_DLIB} ${TARGET_EXE} ${OUT_PATH}
self_distclean:
	@rm -rf objs out pub

objs/${PLATFORM_EIGEN}/%.o:%.c
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_AND_PRINT,$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -c -o $@ -shared $^)

objs/${PLATFORM_EIGEN}/%.o:%.cpp
	@[ -d `dirname $@` ] || mkdir -p `dirname $@`
	$(call COMPILE_AND_PRINT,$(CXX) $(CFLAGS) -ffunction-sections -fdata-sections -c -o $@ -shared $^)

# 如果存在Makefile.order.mak,就包含该文件.该文件描述模块间的依赖关系.
ifeq ($(shell test -f ${CUR_PATH}/Makefile.order.mak && echo true), true)
include ${CUR_PATH}/Makefile.order.mak
endif

