#!/bin/bash
set -e

if [ "$1" = "clean" ]; then
	rm -rf *.o *.so *.dump *.file *.bin out release
	exit 0
fi

#export PATH=/disk0/users/jwzhang/work/tnpu/store/mips-gcc540-glibc222-r3.3.7/bin:$PATH

MIPS_GCC=mips-linux-gnu-gcc
MIPS_AR=mips-linux-gnu-ar

#CFLAGS='-mfp32 -O2 -pthread -fPIC'
CFLAGS='-mfp32 -O2 -fPIC'

${MIPS_GCC} ${CFLAGS} -I.. ./tnpu_moniter.c -o tnpu_moniter_glibc.out -Wall
${MIPS_GCC} ${CFLAGS} -I.. ./tnpu_moniter.c -o tnpu_moniter_uclibc.out -muclibc 
