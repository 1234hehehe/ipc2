
#定义模块间依赖关系.也说明了模块的编译顺序,被依赖的模块首先编译.
#txtest_build:libbmcast_build

curl:libressl polarssl zlib

libressl:

polarssl:

miniupnpc:

mxml:

nginx:zlib libressl pcre

pcre:

polarssl:zlib

sqlite:

tinyxml:

zlib:

libuuid:

tcpdump: libpcap

libebml:

libmatroska:libebml
