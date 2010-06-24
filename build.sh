#/bin/bash
ANDROIDSRC=../android
CCOMPILER=$ANDROIDSRC/prebuilt/linux-x86/toolchain/arm-eabi-4.3.1/bin/arm-eabi-
#scripts/gen_initramfs_list.sh -u 0 -g 0 $ANDROIDSRC/out/target/product/GT-I5700/root/ > .initram_list
#scripts/gen_initramfs_list.sh -u 0 -g 0 $ANDROIDSRC/out/target/product/GT-S8000/root/ > .initram_list
touch .initram_list
make CROSS_COMPILE=$CCOMPILER
