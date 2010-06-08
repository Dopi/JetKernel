#/bin/bash
CCOMPILER=../android/prebuilt/linux-x86/toolchain/arm-eabi-4.3.1/bin/arm-eabi-
ANDROIDSRC=../SPH_M900_MR2_Kernel/src/open_src
scripts/gen_initramfs_list.sh -u 0 -g 0 $ANDROIDSRC/out/target/product/SPH-M900/root/ > .initram_list
make CROSS_COMPILE=$CCOMPILER
