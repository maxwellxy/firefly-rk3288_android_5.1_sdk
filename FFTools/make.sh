#!/bin/bash

usage()
{
    echo "usage: $(basename $0) [-j make_thread]"
}

MAKE_THEARD=1
while getopts "j:" arg
do
	case $arg in
		 j)
			MAKE_THEARD=$OPTARG
			;;
		 ?) #当有不认识的选项的时候arg为?
			usage
			exit 1
			;;
	esac
done

pushd u-boot/
make rk3288_box_defconfig
make -j $MAKE_THEARD
popd

pushd kernel/ 
make firefly-rk3288_defconfig
make firefly-rk3288.img -j $MAKE_THEARD
popd

source  build.sh 
make installclean
make -j $MAKE_THEARD
./mkimage.sh

echo "FireFly build Android finish!"
