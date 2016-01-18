@echo off
cd %1
REM test script

REM add the extra binaries to the system path
set PATH=%PATH%;%1\..\windowsBin

mkdir results

REM add Cloudscape plugin to junit tests zip file
zip eclipse-junit-tests-%3%.zip -rm eclipse

REM run all tests.  -vm argument used as is to eclipse launcher for target eclipse
call runtests.bat -vm %cd%\..\jdk6_17\jre\bin\javaw -properties vm.properties "-Dtest.target=performance" "-Dplatform=win32perf2" 1> %2 2>&1
exit