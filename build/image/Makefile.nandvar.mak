
ifeq (${FLASHMEDIA_NAME},NAND)
#{NAND_PAGE_16K, NAND_ECC_64BIT, 1824/*1824*/, &nand_ecc_default},
#{NAND_PAGE_16K, NAND_ECC_40BIT, 1200/*1152*/, &nand_ecc_default},
#{NAND_PAGE_16K, NAND_ECC_0BIT,  32 ,          &nand_ecc_default},
#
#{NAND_PAGE_8K, NAND_ECC_64BIT, 928 /*928*/,  &nand_ecc_default},
#{NAND_PAGE_8K, NAND_ECC_40BIT, 600 /*592*/,  &nand_ecc_default},
#{NAND_PAGE_8K, NAND_ECC_24BIT, 368 /*368*/,  &nand_ecc_default},
#{NAND_PAGE_8K, NAND_ECC_0BIT,  32,           &nand_ecc_default},
#
#{NAND_PAGE_4K, NAND_ECC_24BIT, 200 /*200*/,  &nand_ecc_default},	
#{NAND_PAGE_4K, NAND_ECC_8BIT,  128 /*88*/,   &nand_ecc_default},
#{NAND_PAGE_4K, NAND_ECC_0BIT,  32,           &nand_ecc_default},
#
#{NAND_PAGE_2K, NAND_ECC_24BIT, 128 /*116*/, &nand_ecc_default},
#{NAND_PAGE_2K, NAND_ECC_8BIT,  64  /*60*/,  &nand_ecc_default},
#{NAND_PAGE_2K, NAND_ECC_0BIT,  32,          &nand_ecc_default},

# Pagetype: 0=2KB,1=4KB,2=8KB,3=16KB.
# EccType:1=4bit/512B,2=16bit/1K,3=24bit/1K,4=28bit/1K,5=40bit/1K,6=64bit/1K.
ifeq (${FLASHPSK_NAME},2)
PageType=0
ifeq (${FLASHSSB_NAME},64)
EccType=1
else ifeq (${FLASHSSB_NAME},128)
EccType=3
else
EccType=1
endif

else ifeq (${FLASHPSK_NAME},4)
PageType=1
ifeq (${FLASHSSB_NAME},128)
EccType=1
else ifeq (${FLASHSSB_NAME},256)
EccType=3
else
EccType=1
endif

else ifeq (${FLASHPSK_NAME},8)
PageType=2
else ifeq (${FLASHPSK_NAME},16)
PageType=3
else
PageType=0
endif

PageNumPerBlock=$(shell expr ${FLASHBSK_NAME} / ${FLASHPSK_NAME} )
$(warning Pagetype=${PageType} EccType=${EccType} FLASHSSB_NAME=${FLASHSSB_NAME} PageNumPerBlock=${PageNumPerBlock})
endif
