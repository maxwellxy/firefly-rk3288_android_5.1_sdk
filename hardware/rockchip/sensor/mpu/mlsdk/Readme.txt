**************************************************************************
**   InvenSense MPL Solution - README.TXT
**************************************************************************

This file briefly explains how to use the InvenSense MPL Solution 
and examples.  The release is conform to the Android Gingerbread
format.

The MPL contains the code for controlling the 
InvenSense MPU series devices, including activating and managing 
built in motion processing features.  
All of the application source code is in ANSI C and can be compiled in C 
or C++ environments.  This code is designed to work with all MPU devices.

*************************************************************************

Build instructions :

the static and shared makefiles build against the Android Framework structure
but do not use the underlying NDK structure. The Sensor manager contains 
makefiles to allow the user to drop the MF release package in the tree and 
build it as part of the Android tree build.  Please refer to the documentation
in the Sensor Manager release package for more details about this.

These makefiles expect to receive:
- the location of the Android framework root in the ANDROID_ROOT variable;
- the location of the kernel root folder in the KERNEL_ROOT variable;
- the location of the Android cross compiling toolchain, in the CROSS variable;
- the target platform of the build, in the PRODUCT variable;
- the target OS, in the TARGET variable;
- the device is use, in the DEVICE variable;
- the name of the advanced, binary library, as MPL_LIB_NAME=mplmpu;
- an optional VERBOSE variable to enable verbose output from the build.

See the top-level Makefile for details: 
- Android-static.mk
- Android-shared.mk
- Android-common.mk


Example on how to run the build processes:

MAKE_CMD="make \
    VERBOSE=0 \
    TARGET=android \
    CROSS=/Android/beagle-eclair/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi- \
    ANDROID_ROOT=/Android/beagle-eclair \
    KERNEL_ROOT=/Android/kernel \
    PRODUCT=beagleboard \
    DEVICE=MPU3050 \
    MPL_LIB_NAME=mplmpu \
"

$MAKE_CMD -f Android-static.mk
$MAKE_CMD -f Android-static.mk clean
$MAKE_CMD -f Android-shared.mk
$MAKE_CMD -f Android-shared.mk clean


The file COMMANDS.txt shows a reference of the make commands originally used 
to build both static and shared library.

Note that the value assigned to the DEVICE variable in the make command
must match the device the release is targeted to.


