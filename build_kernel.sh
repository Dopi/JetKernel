#!/bin/bash

#Set CPU Environment Variable
if [ "$CPU_JOB_NUM" = "" ] ; then
        CPU_JOB_NUM=8
fi


Usage()
{
echo "build_kernel.sh - building script android kernel"
echo "  Usage: ./build_kernel.sh "
echo

exit 1
}

OPTION=-k
PRODUCT=r880

case "$PRODUCT" in
	
    r880)		
                MODULES="dpram vibetonz dpram_recovery"
                KERNEL_DEF_CONFIG=r880_android_defconfig
                ;;
    
	*)
		Usage
		;;
esac 

if [ ! $PWD_DIR ] ; then
	PWD_DIR=$(pwd)
fi

KERNEL_DIR=$PWD_DIR
MODULES_DIR=$PWD_DIR../JetKernel_modules


prepare_kernel()
{
	echo "*************************************"
	echo "*          prepare kernel           *"
	echo "*************************************"
	echo

	make -C $KERNEL_DIR ARCH=arm $KERNEL_DEF_CONFIG
	if [ $? != 0 ] ; then
	    exit 1
	fi
	make -C $KERNEL_DIR ARCH=arm prepare
	if [ $? != 0 ] ; then
	    exit 1
	fi
}

build_modules()
{
	echo "*************************************"
	echo "*           build modules           *"
	echo "*************************************"
	echo

	make -C $KERNEL_DIR ARCH=arm $KERNEL_DEF_CONFIG	
	if [ $? != 0 ] ; then
	    exit 1
	fi
	make -C $KERNEL_DIR ARCH=arm KBUILD_MODPOST_WARN=1 modules
	if [ $? != 0 ] ; then
	    exit 1
	fi

	for module in $MODULES
	do
		echo cd $MODULES_DIR/$module
		cd $MODULES_DIR/$module
		make KDIR=$KERNEL_DIR
	done 

}


build_kernel()
{
	if [ ! -f $KERNEL_DIR/.config ] ; then
		if [ ! -f $KERNEL_DIR/scripts/mod/modprobe ] ; then
			prepare_kernel
		fi
	fi

	echo "make " -C $KERNEL_DIR ARCH=arm $KERNEL_DEF_CONFIG
	make -C $KERNEL_DIR ARCH=arm $KERNEL_DEF_CONFIG
	if [ $? != 0 ] ; then
	    exit 1
	fi

	build_modules

	echo "*************************************"
	echo "*           build kernel            *"
	echo "*************************************"
	echo
	
	cd $KERNEL_DIR

	make -j$CPU_JOB_NUM
	if [ $? != 0 ] ; then
		exit $?
	fi
 
}



case "$OPTION" in
	-k)
		build_kernel
		;;
	*)
		Usage
		;;
esac 

exit 0
