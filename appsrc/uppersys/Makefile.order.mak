
#定义模块间依赖关系.也说明了模块的编译顺序,被依赖的模块首先编译.
#txtest_build:libbmcast_build

access:

access_host:toolfonts 

alarm:
alarm2:

asdpd:discoveryutility

core:librtc libwtdg
core_3in1:core
core_3in1_wifiDome:core
discoveryutility:

email:
eventlog:

filemanage:


librawstorage:

librtmpserver:


libwpa_cli:

mediaserver:librtmpserver librtspserver_v2

network:libwpa_cli
network_8m:

onvifserver:
onvifserver2:

polarssl:

record:librawstorage

smartserver:libpipemsg

sysinit:

toolfonts:

update:libwtdg

uritools:

zkdc:
zkreport:
