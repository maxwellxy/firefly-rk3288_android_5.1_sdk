/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RECOVERY_COMMON_H
#define RECOVERY_COMMON_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// #define LOGE(...) ui_print("E:" __VA_ARGS__)

#define LOGE(fmt, args...)\
{\
    ui_print("E:" fmt, ## args);\
    fprintf(stdout, "E/ [File] : %s; [Line] : %d; [Func] : %s; " fmt, __FILE__, __LINE__, __FUNCTION__, ## args);\
}
#define E(fmt, args...)     LOGE(fmt "\n", ## args)

// #define LOGW(...) fprintf(stdout, "W:" __VA_ARGS__)
#define LOGW(fmt, args...)\
    { fprintf(stdout, "W/ [File] : %s; [Line] : %d; [Func] : %s; " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); }
#define W(fmt, args...)     LOGW(fmt "\n", ## args)

// #define LOGI(...) fprintf(stdout, "I:" __VA_ARGS__)
#define LOGI(fmt, args...)\
{ \
    fprintf(stdout, "I/ [File] : %s; [Line] : %d; [Func] : %s; " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); \
}
#define I(fmt, args...)     LOGI(fmt "\n", ## args)


#define ENABLE_DEBUG_LOG
#ifdef ENABLE_DEBUG_LOG
// #define LOGD(...) fprintf(stdout, "D:" __VA_ARGS__)
#define LOGD(fmt, args...)\
    { fprintf(stdout, "D/ [File] : %s; [Line] : %d; [Func] : %s; " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); }
#ifdef D
#undef D
#endif
#define D(fmt, args...)     LOGD(fmt "\n", ## args)
#else
#define LOGD(...) do {} while (0)
#endif


#define ENABLE_VERBOSE_LOG
#ifdef ENABLE_VERBOSE_LOG
// #define LOGV(...) fprintf(stdout, "V:" __VA_ARGS__)
#define LOGV(fmt, args...)\
    { fprintf(stdout, "V/ [File] : %s; [Line] : %d; [Func] : %s; " fmt, __FILE__, __LINE__, __FUNCTION__, ## args); }
#define V(fmt, args...)     LOGV(fmt "\n", ## args)
#else
#define LOGV(...) do {} while (0)
#endif

/**
 * 调用函数, 并检查返回值, 根据返回值决定是否跳转到指定的错误处理代码. 
 * @param functionCall
 *          对特定函数的调用, 该函数的返回值必须是 表征 成功 or err 的 整型数. 
 *          这里, 被调用函数 "必须" 是被定义为 "返回 0 表示操作成功". 
 * @param result
 *		    用于记录函数返回的 error code 的 整型变量, 通常是 "ret" or "result" 等.
 * @param label
 *		    若函数返回错误, 程序将要跳转到的 错误处理处的 标号, 通常就是 "EXIT". 
 */
#define CHECK_FUNC_CALL(functionCall, result, label) \
{\
	if ( 0 != ( (result) = (functionCall) ) )\
	{\
		E("Function call returned error : " #result " = %d.", result);\
		goto label;\
	}\
}

/**
 * 在特定条件下, 判定 error 发生, 对变量 'retVar' 设置 'errCode', 
 * Log 输出对应的 Error Caution, 然后跳转 'label' 指定的代码处执行. 
 * @param msg
 *          纯字串形式的提示信息. 
 * @param retVar
 *		    标识函数执行状态或者结果的变量, 将被设置具体的 Error Code. 
 *		    通常是 'ret' or 'result'. 
 * @param errCode
 *          表征特定 error 的常数标识, 通常是 宏的形态. 
 * @param label
 *          程序将要跳转到的错误处理代码的标号, 通常就是 'EXIT'. 
 * @param args...
 *          对应 'msgFmt' 实参中 '%s', '%d', ... 等 转换说明符 的具体可变长实参. 
 */
#define SET_ERROR_AND_JUMP(msgFmt, retVar, errCode, label, args...) \
{\
    E("To set '" #retVar "' to %d('" #errCode "') : " msgFmt, (errCode), ## args);\
	(retVar) = (errCode);\
	goto label;\
}

/**
 * 返回指定数组中的元素个数. 
 * @param array
 *      有效的数组实例标识符. 
 */
#define ELEMENT_NUM(array)      ( sizeof(array) /  sizeof(array[0]) )

/*-------------------------------------------------------*/

/** 使用 D(), 以十进制的形式打印变量 'var' 的 value. */
#define D_DEC(var) \
{ \
    long long num = (var); \
    D(#var " = %lld.", num); \
}

/** 使用 D(), 以十六进制的形式打印变量 'var' 的 value. */
#define D_HEX(var) \
{ \
    long long num = (var); \
    D(#var " = 0x%llx.", num); \
}

/** 使用 D(), 以十六进制的形式 打印指针类型变量 'ptr' 的 value. */
#define D_PTR(ptr)  D(#ptr " = %p.", ptr);

/** 使用 D(), 打印 char 字串. */
#define D_STR(pStr) \
{\
    if ( NULL == pStr )\
    {\
        D(#pStr" = NULL.");\
    }\
    else\
    {\
        D(#pStr" = '%s'.", pStr);\
    }\
}

/**
 * 在 'pIndentsBuf' 指向的 buf 中, 从头插入 'indentNum' 个 '\t' 字符(缩进), 并跟上一个 '\0'. 
 * 通常 pIndentsBuf 指向一个 16 字节的 buffer. 
 */
inline static void setIndents(char* pIndentsBuf, unsigned char indentNum)
{
    unsigned char i = 0; 

    const unsigned char MAX_NUM_OF_INDENT = 16 - sizeof('\0');
    if ( indentNum > MAX_NUM_OF_INDENT )
    {
        indentNum = MAX_NUM_OF_INDENT;
    }

    *pIndentsBuf = '\0';
    for ( i = 0; i < indentNum; i++ )
    {
        strcat(pIndentsBuf, "\t");
    }
    *(pIndentsBuf + indentNum) = '\0';
}

/*---------------------------------------------------------------------------*/

#define STRINGIFY(x) #x
#define EXPAND(x) STRINGIFY(x)

typedef struct fstab_rec Volume;

/*-------------------------------------------------------*/
typedef unsigned char boolean;
#ifdef TRUE
#undef TRUE
#endif
#define TRUE        ( (boolean)(1) )

#ifdef FALSE
#undef FALSE
#endif
#define FALSE       ( (boolean)(0) )
/*-------------------------------------------------------*/

#define RU_PARTITION_MOUNT_PATH     "/radical_update"
#define SYSTEM_PARTITION_MOUNT_PATH     "/system"

// fopen a file, mounting volumes and making parent dirs as necessary.
FILE* fopen_path(const char *path, const char *mode);

void ui_print(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif  // RECOVERY_COMMON_H
