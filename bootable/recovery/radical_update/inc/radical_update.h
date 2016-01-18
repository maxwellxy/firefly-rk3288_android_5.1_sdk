/*  --------------------------------------------------------------------------------------------------------
 *  File:   radical_update.h 
 *
 *  Desc:   声明 recovery 组件 libradical_update_recovery.a 对外提供的功能接口.
 *          
 *          因为实现中使用的静态变量, 所以本模块 "不是" 现成安全的. 
 *
 *          -----------------------------------------------------------------------------------
 *          < 习语 和 缩略语 > : 
 *
 *          -----------------------------------------------------------------------------------
 *  Usage:		
 *
 *  Note:
 *
 *  Author: ChenZhen
 *  
 *  Log:
	----Sun Jun 15 11:06:55 2014            init ver.
 *        
 *  --------------------------------------------------------------------------------------------------------
 */


#ifndef __RADICAL_UPDATE_H__
#define __RADICAL_UPDATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------------------------------------
 *  Include Files
 * ---------------------------------------------------------------------------------------------------------
 */
#include <linux/kernel.h>

#include "common.h"


/* ---------------------------------------------------------------------------------------------------------
 *  Macros Definition 
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 *  Types and Structures Definition
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 *  Global Functions' Prototype
 * ---------------------------------------------------------------------------------------------------------
 */

/**
 * 根据 radical_update/radical_update.xml 中的配置,
 * 将存储在 radical_update_partition 中的 fws_in_ota_ver 恢复回到的 system_partition 中.
 *
 * 为兼容 ota_update 和 radical_update 机制, 在 ota_update 操作之前 "必须" 调用本函数. 
 * 
 * 调用本函数前, 调用者必须 mount "/system" 和 "/radical_update". 
 * 若本函数成功完成, 还将 复位 内部标志 ru_is_applied. 
 */
int RadicalUpdate_restoreFirmwaresInOtaVer();

/**
 * 尝试执行 radical_update, 具体包含如下操作 : 
 * 先将相关的 fws 备份到 radical_update/backup_of_fws_in_ota_ver 下, 
 * 然后将 radical_update/radical_update_fws 中的内容, update 到 system_partition. 
 * 
 * 本函数通常在 ru_pkg 成功安装之后执行.
 * ru_pkg 用于更新 radical_update/, 即 ru_parition 中的内容. 
 * 
 * 调用本函数前, 调用者必须 mount "/system" 和 "/radical_update". 
 * 若本函数成功完成, 还将 置位 内部标志 ru_is_applied. 
 */
int RadicalUpdate_tryToApplyRadicalUpdate();


/**
 * 返回 系统中是否已经安装过 ru_pkg.
 * 
 * 调用本函数前, 调用者必须 mount "/radical_update". 
 */
boolean RadicalUpdate_hasRuPkgBeenInstalled();

/**
 * 返回 ru (radical_update) 是否已经被 应用(更新) 到 system_partition, 即内部标志 ru_is_applied 的状态. 
 * 
 * 调用本函数前, 调用者必须 mount "/radical_update". 
 */
boolean RadicalUpdate_isApplied();

/* ---------------------------------------------------------------------------------------------------------
 *  Inline Functions Implementation 
 * ---------------------------------------------------------------------------------------------------------
 */

#ifdef __cplusplus
}
#endif

#endif /* __RADICAL_UPDATE_H__ */

