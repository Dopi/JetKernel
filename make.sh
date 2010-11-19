#!/bin/sh
CCOMPILER=/opt/arm-2010q1/bin/arm-none-linux-gnueabi-
make CROSS_COMPILE=$CCOMPILER ARCH=arm $@