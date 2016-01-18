# !/bin/sh
ulimit -c unlimited

#execute command to run tests
./runtests -os macosx -ws cocoa -arch x86 -properties `pwd`/vm.properties 1> macosx.cocoa_consolelog.txt 2>&1

