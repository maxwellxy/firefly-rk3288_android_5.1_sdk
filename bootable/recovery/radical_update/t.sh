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


<<.
.

# 可以 kill 多个进程.
# adb shell kill $(pid se_client_for_test)

adb shell kill $(pid logcat)
adb shell logcat -c
# adb shell logcat -v threadtime > /home/chenzhen/workspace/logcat.log &
# adb shell logcat -v thread > /home/chenzhen/workspace/logcat.log &
adb shell logcat -v threadtime 2>&1 | tee $WD/logcat.log &


# ----------------------------------------------------- #
# 调用 fam_app, 预期成功的测试用例.

<<.
cd ../selftest
. deploy_fam_res.sh
cd -
adb shell my_test 
.

# ----------------------------------------------------- #
# 预期 fam 报错的测试用例.
# .WP : 

adb shell test_lib_radical_update

sleep 1

adb shell kill $(pid logcat)
