#!/bin/bash
# 生成 svn 状态信息
svn_info_txt=`svn info`; ret_info=$?
unset svn_info_txt
if [ $ret_info -eq 0 ]
then {
    revision=`git describe`;		
    svn_version=`svn info | sed -n 9p | grep -o '[0-9]\+'`;
    svn log -r$svn_version > svn_log; ret_log=$?
    if [ $ret_log -eq 0 ]
    then {
        svn_log=`sed '1d;2d;3d;$d' svn_log | tr '\n' ' '`;
    } fi
    rm svn_log -f
} fi
#svn_diff=`svn diff`; ret_diff=$?
#if [ $ret_diff -eq 0 ]
#then {
#    svn_log=`sed '1d;2d;3d;$d' svn_log`;
#} fi
sf_author=$LOGNAME
sf_date=`date -R`

# 处理生成的状态信息
echo "#define SF_COMPILE_INFO      \"author:  $sf_author time: $sf_date version: ${svn_version:-0}, on $revision.\"" > svn_info.h
echo "#define SF_COMPILE_INFO      \"author:  $sf_author time: $sf_date version: ${svn_version:-0}, on $revision.\"" > ./libvpu/vpu_api/include/svn_info.h
echo "#define SVN_VERSION          ${svn_version:-0}" >> svn_info.h
echo "#define SVN_VERSION_INFO     \"sf_log:  ${svn_log:-"svn info does not exist"}\"" >> svn_info.h
#echo "#define SVN_VERSION_DIFF     \"sf_diff: ${svn_diff:-"svn diff does not exist"}\"" >> svn_info.h

# 为wfd添加版本信息
echo "#define WFD_COMPILE_INFO      \"author:  $sf_author time: $sf_date version: ${svn_version:-0}, on $revision.\"" > ./wifi-display/source/wfd_svn_info.h
echo "#define WFD_VERSION          ${svn_version:-0}" >> ./wifi-display/source/wfd_svn_info.h
echo "#define WFD_VERSION_INFO     \"sf_log:  ${svn_log:-"svn info does not exist"}\"" >> ./wifi-display/source/wfd_svn_info.h
#echo "#define SVN_VERSION_DIFF     \"sf_diff: ${svn_diff:-"svn diff does not exist"}\"" >> ./wifi-display/source/wfd_svn_info.h
