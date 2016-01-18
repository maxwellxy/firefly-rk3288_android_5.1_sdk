/*  --------------------------------------------------------------------------------------------------------
 *  File:   recovery_utils.h 
 *
 *  Desc:
 *
 *          -----------------------------------------------------------------------------------
 *          < Ï°Óï ºÍ ËõÂÔÓï > : 
 *
 *          -----------------------------------------------------------------------------------
 *  Usage:		
 *
 *  Note:
 *
 *  Author: ChenZhen
 *  
 *  Log:
 *        
 *  --------------------------------------------------------------------------------------------------------
 */


#ifndef __RECOVERY_UTILS_H__
#define __RECOVERY_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------------------------------------
 *  Include Files
 * ---------------------------------------------------------------------------------------------------------
 */

#include "bootloader.h"
#include "common.h"



/* ---------------------------------------------------------------------------------------------------------
 *  Macros Definition 
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 *  Types and Structures Definition
 * ---------------------------------------------------------------------------------------------------------
 */

typedef struct bootloader_message bootloader_message_t;

/* ---------------------------------------------------------------------------------------------------------
 *  Global Functions' Prototype
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 *  Inline Functions Implementation 
 * ---------------------------------------------------------------------------------------------------------
 */

inline void dumpBootLoaderMessage(const bootloader_message_t* pThis, unsigned char indentNum)
{
    char indents[16];
    setIndents(indents, indentNum);
    
    fprintf(stdout, "%s command : %s \n", indents, pThis->command);
    fprintf(stdout, "%s status : %s \n", indents, pThis->status);
    fprintf(stdout, "%s recovery : %s \n", indents, pThis->recovery);
    fprintf(stdout, "%s stage : %s \n", indents, pThis->stage);
}

inline void dumpCmdArgs(int argc, char** argv, unsigned char indentNum) {
    char indents[16];
    setIndents(indents, indentNum);
    int i;
    
    if ( 0 == argc )
    {
        fprintf(stdout, "%s no arguments. \n", indents);
        return;
    }

    fprintf(stdout, "%s %d argument(s) : \n", indents, argc);
    for ( i = 0; i < argc; i++ )
    {
        fprintf(stdout, "%s %s \n", indents, argv[i] );
    }
}


#ifdef __cplusplus
}
#endif

#endif /* __RECOVERY_UTILS_H__ */

