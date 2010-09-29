#!/bin/bash

PRODUCT=SPH-M900
REV=00
OPTION=-k

case "$PRODUCT" in
	
	SPH-M900)
		REALPRODUCT=$PRODUCT
		MODULES="camera cmm g2d g3d jpeg mfc pp rotator btgpio vibetonz camera dpram wlan"
		PACKAGES="tscal"
		KERNEL_DEF_CONFIG=m900_android_defconfig 
		;;
esac 

if [ ! $PWD_DIR ] ; then
	PWD_DIR=$(pwd)
fi

WORK_DIR=$PWD_DIR/src
OUT_DIR=$PWD_DIR/OUTDIR

ANDROID_DIR=$WORK_DIR/open_src
ANDROID_OUT_DIR=$ANDROID_DIR/out/target/product/$PRODUCT
KERNEL_DIR=$WORK_DIR/kernel
MODULES_DIR=$WORK_DIR/modules
PACKAGES_DIR=$WORK_DIR/packages

change_kernel_config()
{
	cd $KERNEL_DIR

	cp arch/arm/configs/$1 .config

	if [ $2 -a $2 = with_initram ] ; then
            sed '/CONFIG_INITRAMFS_SOURCE/d' arch/arm/configs/$1 > .config
	    echo "CONFIG_INITRAMFS_SOURCE=\".initram_list\"" >> .config
	    echo "CONFIG_INITRAMFS_ROOT_UID=0" >> .config
            echo "CONFIG_INITRAMFS_ROOT_GID=0" >> .config
	else
	    sed -e '/CONFIG_BLK_DEV_INITRD/d' -e '/CONFIG_INITRAMFS/d' arch/arm/configs/$1 > .config
	    echo "# CONFIG_BLK_DEV_INITRD is not set" >> .config
	fi
	make oldconfig
}

create_initram_list()
{
	cd $KERNEL_DIR
	
	bash scripts/gen_initramfs_list.sh -u 0 -g 0 $ANDROID_OUT_DIR/root > .initram_list
	if [ $? != 0 ] ; then
		exit $?
	fi
}

build_kernel()
{
	echo *************************************
	echo *           build kernel            *
	echo *************************************
		
		cd $KERNEL_DIR

		rm -f usr/initramfs_data.cpio.gz
		
		echo make -j$CPU_JOB_NUM
		make -j$CPU_JOB_NUM
		if [ $? != 0 ] ; then
			exit $?
		fi

		case "$PRODUCT" in
			
	    	SPH-M900)
	      if [ $REALPRODUCT = SPH-M900 ] ; then
			     cd $KERNEL_DIR/arch/arm/boot
			     tar -cvf $OUT_DIR/$PRODUCT.zImage.tar zImage
				fi
				;;
		esac
}

build_modules()
{
	echo *************************************
	echo *           build modules           *
	echo *************************************

	echo rm -rf $ANDROID_OUT_DIR/root/lib/modules
	rm -rf $ANDROID_OUT_DIR/root/lib/modules

	echo mkdir -p $ANDROID_OUT_DIR/root/lib/modules
	mkdir -p $ANDROID_OUT_DIR/root/lib/modules

	for module in $MODULES
	do
		echo cd $MODULES_DIR/$module
		cd $MODULES_DIR/$module
		make KDIR=$KERNEL_DIR PRJROOT=$WORK_DIR && \
		    cp *.ko $ANDROID_OUT_DIR/root/lib/modules/
		if [ "$?" -ne 0 ]; then
		    echo "*ERROR* while building $modules"
		    exit -1
		fi
	done

}

build_packages()
{
	echo *************************************
	echo *           build packages           *
	echo *************************************

	for package in $PACKAGES
	do
		echo cd $PACKAGES_DIR/$package
		cd $PACKAGES_DIR/$package
		make KDIR=$KERNEL_DIR && \
		    make ROOT=$ANDROID_OUT_DIR/root install
		if [ "$?" -ne 0 ]; then
		    echo "*ERROR* while building $package"
		    exit -1
		fi
	done

}

if [ ! -d $OUT_DIR ] ; then
	mkdir -p $OUT_DIR
fi

case "$OPTION" in
	
	-k)
			change_kernel_config $KERNEL_DEF_CONFIG
			build_kernel
			build_modules
			build_packages
			create_initram_list
			change_kernel_config $KERNEL_DEF_CONFIG with_initram
			build_kernel
		;;
	*)
		Usage
		;;
esac 

echo ""
echo "Total spent time:"
times
	
exit 0