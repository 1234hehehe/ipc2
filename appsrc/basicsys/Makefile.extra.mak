
# 目标 extra_build_pretarget,会在当前Makefile的build依赖的目标集当中首先执行.
extra_build_pretarget:

# 目标 extra_build_posttarget,会在当前Makefile的build依赖的目标集当中最后执行.
extra_build_posttarget:
	@mkdir -p ${PUB_PATH};
	cp -rLpT ${OUT_PATH} ${PUB_PATH};

# 目标 extra_clean_pretarget,会在当前Makefile的clean依赖的目标集当中首先执行.
extra_clean_pretarget:

# 目标 extra_clean_posttarget,会在当前Makefile的clean依赖的目标集当中最后执行.
extra_clean_posttarget:
ifeq ($(if $(wildcard ${PUB_PATH}),x),x)
	rm -rf ${PUB_PATH};
endif
