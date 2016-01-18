# 定义对 ERR 信号的处理函数.
function errtrap {
	es=$?
	echo "ERROR line $1: Command exited with status $es."
	exit $es
}
# 注册 ERR trap.
trap 'errtrap $LINENO' ERR


# ----------------------------------------------------- #
# .T : 这里开始编写脚本. 

<<.
.

. $pd'/build/envsetup.sh'

# mm

. remount.sh

BIN_TARGET_PATH='/system/bin'


adb push $rcod/system/bin/test_lib_radical_update $BIN_TARGET_PATH
adb shell chmod 777 $BIN_TARGET_PATH/test_lib_radical_update 


LIB_TARGET_PATH='/system/lib'
# LIB_TARGET_PATH='/epay/usr/lib'

# adb push $cod/system/lib/lib_security_config.so $LIB_TARGET_PATH

. t.sh

