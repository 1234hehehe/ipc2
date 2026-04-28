
#定义模块间依赖关系.也说明了模块的编译顺序,被依赖的模块首先编译.
#txtest_build:libbmcast_build


opensrc:

thirdpart: opensrc

basicsys: opensrc thirdpart

lowersys: basicsys opensrc thirdpart

uppersys: basicsys opensrc thirdpart
