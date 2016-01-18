/*  --------------------------------------------------------------------------------------------------------
 *  File:   radical_update.c
 *
 *  Desc:   对 radical_update.h 中声明的功能接口的实现. 
 *
 *          -----------------------------------------------------------------------------------
 *          < 习语 和 缩略语 > : 
 *
 *          -----------------------------------------------------------------------------------
 *          < 内部模块 or 对象的层次结构 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 功能实现的基本流程 > :
 *
 *          -----------------------------------------------------------------------------------
 *          < 关键标识符 > : 
 *
 *          -----------------------------------------------------------------------------------
 *          < 本模块实现依赖的外部模块 > : 
 *              ... 
 *          -----------------------------------------------------------------------------------
 *  Note:
 *
 *  Author: ChenZhen
 *  
 *  Log: 
	----Sun Jun 15 11:09:32 2014            init ver
 */


/* ---------------------------------------------------------------------------------------------------------
 * Include Files
 * ---------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"
#include "libxml/SAX2.h"
#include "libxml/xmlstring.h"

#include "common.h"
#include "roots.h"

#include "radical_update.h"

/* ---------------------------------------------------------------------------------------------------------
 * Local Macros 
 * ---------------------------------------------------------------------------------------------------------
 */

#define RU_CFG_XML          "/radical_update/radical_update.xml"                // RU : Radical Update
/**
 * 该文件若 存在, 标识 ru 有被应用到 system_partition.
 * 否则, 标识 ru "没有" 被应用到 system_partition, 比如 将 backup_of_fws_in_ota_ver restore 回到 system_partition 之后. 
 */
#define FLAG_FILE_RU_IS_APPLIED     "/radical_update/ru_is_applied"

#define BUSYBOX_PATH        "/sbin/busybox"

/* ---------------------------------------------------------------------------------------------------------
 * Local Typedefs 
 * ---------------------------------------------------------------------------------------------------------
 */


/**
 * 描述对 ru_item_cmd 的执行者实例.
 *
 * radical_update_cfg_xml 中的 "item" 元素描述 update 操作的对应单元(通常是文件), 以及相关的实施命令 (ru_item_cmd). 
 * 本征地, ru_item_cmd 用来描述部署当前 update_item 需要的操作, 这 和 RadicalUpdate_tryToApplyRadicalUpdate() 流程相关.
 * 但从中也可以得到对应的 RadicalUpdate_restoreFirmwaresInOtaVer() 流程相关中的 item_restore_cmd.
 * 
 * 在 apply 和 restore 流程中, 对 ru_item_cmd 的具体执行方式不同, 以本类型的不同实例描述. 
 */
typedef struct ru_item_cmd_executant_t {

    void* pExecutantData;

    int (*pfnExecuteItemCmd)(void* pExecutantData, const char* pOp, int argc, char** argv);
    
}   ru_item_cmd_executant_t;


/* ---------------------------------------------------------------------------------------------------------
 * External Function Prototypes (referenced in this file)
 * ---------------------------------------------------------------------------------------------------------
 */


/* ---------------------------------------------------------------------------------------------------------
 * Local Function Prototypes
 * ---------------------------------------------------------------------------------------------------------
 */

static xmlChar* dupAttrValueFromList(const xmlChar* pName,
                                     int attrNum,
                                     const xmlChar** ppAttributes);

static int parseCfgXmlAndCarryOut(ru_item_cmd_executant_t* pExecutant);

static void onStartElementNs(void *ctx,
                             const xmlChar *localname,
                             const xmlChar *prefix,
                             const xmlChar *URI,
                             int nb_namespaces,
                             const xmlChar **namespaces,
                             int nb_attributes,
                             int nb_defaulted,
                             const xmlChar **attributes);

static void onEndElementNs(void* ctx,
                           const xmlChar* localname,
                           const xmlChar* prefix,
                           const xmlChar* URI);


static int execItemApplyCmd(void* pExecutantData, const char* pOp, int argc, char** argv);

static int convertItemApplyCmdToRestoreAndExec(void* pExecutantData, const char* pOp, int argc, char** argv);

static int getFwFileBackupDirPath(const char* pPathInSystem, char* pBackupDirPath);

static int getFwFileBackupPath(const char* pPathInSystem, char* pBackupPath);

static int run(const char *filename, char *const argv[] );

static int setRuIsApplied();

static int setRuNotApplied();

/* ---------------------------------------------------------------------------------------------------------
 * Local Variables 
 * ---------------------------------------------------------------------------------------------------------
 */

static ru_item_cmd_executant_t sItemCmdExecutantInApply =
{
    NULL,
    execItemApplyCmd,
};

static ru_item_cmd_executant_t sItemCmdExecutantInRestore =
{
    NULL,
    convertItemApplyCmdToRestoreAndExec,
};

/* ---------------------------------------------------------------------------------------------------------
 * Global Variables
 * ---------------------------------------------------------------------------------------------------------
 */

/* ---------------------------------------------------------------------------------------------------------
 * Global Functions Implementation
 * ---------------------------------------------------------------------------------------------------------
 */

int RadicalUpdate_restoreFirmwaresInOtaVer()
{
    int ret = 0;
    
    /* 解析 radical_update_cfg_xml, 并根据其内容完成 restore 操作. */
    CHECK_FUNC_CALL( parseCfgXmlAndCarryOut(&sItemCmdExecutantInRestore) , ret, EXIT);

    CHECK_FUNC_CALL( setRuNotApplied() , ret, EXIT);

EXIT:
    return ret;
}


int RadicalUpdate_tryToApplyRadicalUpdate()
{
    int ret = 0;

    /* 解析 radical_update_cfg_xml, 并根据其内容完成 apply 操作. */
    CHECK_FUNC_CALL( parseCfgXmlAndCarryOut(&sItemCmdExecutantInApply) , ret, EXIT);

    CHECK_FUNC_CALL( setRuIsApplied() , ret, EXIT);

EXIT:
    return ret;
}


boolean RadicalUpdate_hasRuPkgBeenInstalled()
{
    return ( 0 == access(RU_CFG_XML, F_OK) );
}

boolean RadicalUpdate_isApplied()
{
    return ( RadicalUpdate_hasRuPkgBeenInstalled()
            && ( 0 == access(FLAG_FILE_RU_IS_APPLIED, F_OK) ) );
}


/* ---------------------------------------------------------------------------------------------------------
 * Local Functions Implementation
 * ---------------------------------------------------------------------------------------------------------
 */

/**
 * 从 attrNum 和 ppAttributes 制定的 attr val 列表中拷贝 名称为 pName 的那一项. 
 */
static xmlChar* dupAttrValueFromList(const xmlChar* pName,
                                     int attrNum,
                                     const xmlChar** ppAttributes)
{
    xmlChar* ret = NULL;
    int i;

	for ( i = 0; i < attrNum; i++ ) 
    {
		// V("attrname = %s", ppAttributes[i * 5] );
		if ( 0 == xmlStrcmp(ppAttributes[i * 5], pName) )
        {
			const xmlChar* str = xmlStrchr(ppAttributes[i * 5 + 3], '"');
			ret = xmlStrndup(ppAttributes[i * 5 + 3],
                             str - ppAttributes[i * 5 + 3] );
			V("value of attr '%s' is '%s'.", pName, ret);
			return ret;
		}
	}

	return NULL;
}

/**
 * 读取 radical_update_cfg_xml, 并根据其中的内容, 完成对 fws_in_ota_ver 的 restore 操作. 
 */
static int parseCfgXmlAndCarryOut(ru_item_cmd_executant_t* pExecutant)
{
    int ret = 0;
    
    FILE* pCfgXml = NULL;
    xmlParserCtxtPtr ctxt = NULL;
    
    xmlSAXHandler SAXHander;
	int readcount = 0;
    char chunk[1024];

    /*-------------------------------------------------------*/

    memset(&SAXHander, 0, sizeof(xmlSAXHandler));
    SAXHander.initialized = XML_SAX2_MAGIC;
    SAXHander.startElementNs = onStartElementNs;
    SAXHander.endElementNs = onEndElementNs;
    
	pCfgXml = fopen(RU_CFG_XML, "r");
	if ( NULL == pCfgXml ) {
		E("fail to open %s. err : %s. maybe no radical_update_pkg has been installed.", RU_CFG_XML, strerror(errno) );
        ret = 1;
        goto EXIT;
	}

    fseek(pCfgXml, 0, SEEK_SET);
    readcount = fread(chunk, 1, 4, pCfgXml);      // .Q : "4" ? 
    ctxt = xmlCreatePushParserCtxt(&SAXHander,
                                   pExecutant,      // user_data
                                   chunk,
                                   readcount,
                                   NULL);
    // V("pExecutant = %p.", pExecutant);
    // V("ctxt = %p.", ctxt);
    if ( NULL == ctxt )
    {
        SET_ERROR_AND_JUMP("fail to create parser ctxt.", ret, 1, EXIT);
    }

    while ( ( readcount = fread(chunk, 1, sizeof(chunk), pCfgXml) ) > 0 )
    {
        ret =  xmlParseChunk(ctxt, chunk, readcount, 0);
        if ( 0 != ret )
        {
            E("fail to parse chunk, ret = %d.", ret);
            xmlParserError(ctxt, "xmlParseChunk");      // .Q : 
            goto EXIT;
        }
    }

    CHECK_FUNC_CALL( xmlParseChunk(ctxt,
                                   chunk,
                                   0, 
                                   1)
                    , ret, EXIT);

EXIT:
    if ( NULL != pCfgXml )
    {
        fclose(pCfgXml);
        pCfgXml = NULL;
    }
    
    if ( NULL != ctxt )
    {
        xmlFreeParserCtxt(ctxt);
        ctxt = NULL;
    }
    xmlCleanupParser();
    
    return ret;
}

/**
 * apply fws_in_radical_update_ver 或 restore fws_in_ota_ver 流程中, 
 * 对 radical_update_cfg_xml 中 element (start)的解析和处理的具体实现. 
 */
static void onStartElementNs(void *ctx,
                             const xmlChar *localname,
                             const xmlChar *prefix __attribute__((unused)),
                             const xmlChar *URI __attribute__((unused)),
                             int nb_namespaces __attribute__((unused)),
                             const xmlChar **namespaces __attribute__((unused)),
                             int nb_attributes,
                             int nb_defaulted,
                             const xmlChar **attributes)
{
    int ret = 0;

	xmlChar* pValOfAttrOp = NULL;            // 元素 "item" 的 属性 "op" 的 value, 指向一个独立的 heap 实例. 
	xmlChar* pValOfAttrSrc = NULL;           // "src"
	xmlChar* pValOfAttrDest = NULL;          // "dest"
	xmlChar* pValOfAttrMode = NULL;          // "mode"
    
    ru_item_cmd_executant_t* pItemCmdExecutant = (ru_item_cmd_executant_t*)ctx;         // user_data, 定制对 item_cmd 的具体实现.

    char* argv[16];                   // 从 <item> 中解析得到的 op 的参数. 
    int argc = 0;

    /*-------------------------------------------------------*/
    // V("ctx = %p.", ctx);

	V("start element  : tag: %s, attr number %d, default attr num %d",
	    localname,
        nb_attributes,
        nb_defaulted);

	if ( 0 == xmlStrcmp(localname, BAD_CAST "item") ) 
    {
        pValOfAttrOp = dupAttrValueFromList(BAD_CAST "op", nb_attributes, attributes);
        if ( NULL == pValOfAttrOp )
        {
            SET_ERROR_AND_JUMP("can not find attr 'op', to bow out.", ret, -1, EXIT);
        }
        
        if ( 0 == xmlStrcmp(pValOfAttrOp, BAD_CAST "cp") )
        {
            pValOfAttrSrc = dupAttrValueFromList(BAD_CAST "src", nb_attributes, attributes);
            pValOfAttrDest = dupAttrValueFromList(BAD_CAST "dest", nb_attributes, attributes);
            pValOfAttrMode = dupAttrValueFromList(BAD_CAST "mode", nb_attributes, attributes);
            V("get a 'item' element : 'op' = %s; 'src' = %s; 'dest' = %s; 'mode' = %s.", 
                pValOfAttrOp,
                pValOfAttrSrc,
                pValOfAttrDest,
                pValOfAttrMode);

            argv[0] = (char*)pValOfAttrSrc;
            argv[1] = (char*)pValOfAttrDest;
            argv[2] = (char*)pValOfAttrMode;
            argc = 3;
        }
        else 
        {
            W("unexpected 'op' : %s.", pValOfAttrMode);
            goto EXIT;
        }
        
        if ( NULL != pItemCmdExecutant && NULL != pItemCmdExecutant->pfnExecuteItemCmd )
        {
            D("to exec item_cmd.");
            pItemCmdExecutant->pfnExecuteItemCmd(pItemCmdExecutant->pExecutantData, 
                                                 (char*)pValOfAttrOp,
                                                 argc,
                                                 argv);
        }
        else 
        {
            E("fatal : no ru_item_cmd_executant. ");
            goto EXIT;
        }
    }
    else
    {
        V("unhandlable tag : %s.", localname);
    }

EXIT:
    if ( NULL != pValOfAttrOp )
    {
		xmlFree(pValOfAttrOp);
    }
    if ( NULL != pValOfAttrSrc)
    {
		xmlFree(pValOfAttrSrc);
    }
    if ( NULL != pValOfAttrDest )
    {
		xmlFree(pValOfAttrDest);
    }
    if ( NULL != pValOfAttrMode )
    {
		xmlFree(pValOfAttrMode);
    }

    return;
}

/**
 * restore fws_in_ota_ver 流程中, 对 radical_update_cfg_xml 中 element (end) 的解析和处理的具体实现. 
 */
static void onEndElementNs(void* ctx __attribute__((unused)),
                           const xmlChar* localname,
                           const xmlChar* prefix __attribute__((unused)),
                           const xmlChar* URI __attribute__((unused)))
{
    V("element ends, tag : %s.", localname);
}

/**
 * 执行传入的 item_cmd(ru_item_apply_cmd).
 */
static int execItemApplyCmd(void* pExecutantData __attribute__((unused)), const char* pOp, int argc, char** argv)
{
    int ret = 0;
    V("enter. pOp = %s, argc = %d", pOp, argc);
    
	char* cmd[10];
    char fileBackupDirPath[PATH_MAX + 1];          // 当前 item (某个固件文件) 的(ota 版本的) backup 路径 中的目录的部分.  

    fileBackupDirPath[0] = '\0';

    /*-------------------------------------------------------*/
    
    if ( NULL == pOp 
        || ( 0 != argc && NULL == argv ) )
    {
        SET_ERROR_AND_JUMP("invalid arg.", ret, 1, EXIT);
    }

    /*-------------------------------------------------------*/

	if ( 0 == strcmp(pOp, "cp") )       // 操作为 "cp" 时的参数 : argv[0] : src; argv[1] : dest; argv[2] : mode.
    {
        char* pSrc = argv[0];     // 当前 file item 的 radical_update_ver 存储的绝对路径. 
        char* pDest = argv[1];    // 当前 file item 在 system_partition 中的绝对路径.
        char* pMode = argv[2];    // apply radical_update_ver 之后预期使用的 file mode. 

        CHECK_FUNC_CALL( getFwFileBackupDirPath(pDest, fileBackupDirPath) , ret, EXIT);
		D("to mkdir '%s'.", fileBackupDirPath);
		// mk dir 'fileBackupDirPath'.
		cmd[0] = BUSYBOX_PATH;
		cmd[1] = "mkdir";
		cmd[2] = "-p";
		cmd[3] = fileBackupDirPath;
		cmd[4] = NULL;
		run(cmd[0], cmd);
        
		//backup dest(fw_in_ota_ver)
		// cmd[0] = BUSYBOX_PATH;
		cmd[1] = "cp";
		cmd[2] = "-a";      // .! : -a, --archive : same as -dR --preserve=all  
		cmd[3] = pDest;
		cmd[4] = fileBackupDirPath;
		cmd[5] = NULL;
		run(cmd[0], cmd);

		// apply : cp src to des
		// cmd[0] = BUSYBOX_PATH;
		// cmd[1] = "cp";
		// cmd[2] = "-a";
		cmd[3] = pSrc;
		cmd[4] = pDest;
		cmd[5] = NULL;
		run(cmd[0], cmd);

		// chmod des file mode
		// cmd[0] = BUSYBOX_PATH;
		cmd[1] = "chmod";
		cmd[2] = pMode;
		cmd[3] = pDest;
		cmd[4] = NULL;
		run(cmd[0], cmd);
	}
	else 
    {
        W("unexpected 'pOp' : %s.", pOp);
        goto EXIT;
	}

EXIT:
    return ret;
}

/**
 * 将传入的 ru_item_apply_cmd, 转换为 item_restore_cmd, 并执行. 
 */
// .CP : 
static int convertItemApplyCmdToRestoreAndExec(void* pExecutantData __attribute__((unused)), const char* pOp, int argc, char** argv)
{
    int ret = 0;
    V("enter. pOp = %s, argc = %d", pOp, argc);
    
	char* cmd[10];
    char fwBackupPath[PATH_MAX + 1];          // 当前 item (某个固件文件) 的(ota 版本的) 的 backup 路径.

    fwBackupPath[0] = '\0';

    /*-------------------------------------------------------*/
    
    if ( NULL == pOp 
        || ( 0 != argc && NULL == argv ) )
    {
        SET_ERROR_AND_JUMP("invalid arg.", ret, 1, EXIT);
    }

    /*-------------------------------------------------------*/

	if ( 0 == strcmp(pOp, "cp") )       // 操作为 "cp" 时的参数 : argv[0] : src; argv[1] : dest; argv[2] : mode.
    {
        char* pSrc = argv[0];     // 当前 file item 的 radical_update_ver 存储的绝对路径. 
        char* pDest = argv[1];    // 当前 file item 在 system_partition 中的绝对路径.
        char* pMode = argv[2];    // apply radical_update_ver 之后预期使用的 file mode. 
        
        /* 若 *pSrc "不" 存在, 则 ... */
        if ( 0 != access(pSrc, F_OK) )
        {
            W("restore_src(%s) does not exist, maybe no ru_pkg has been installed, do not execute this cmd.", pSrc);
            goto EXIT;
        }
                
        /*
        // rm dest(fw_in_system_partition)
		cmd[0] = BUSYBOX_PATH;
		cmd[1] = "rm";
		cmd[2] = pDest;
		cmd[3] = NULL;
		run(cmd[0], cmd);
        */

        CHECK_FUNC_CALL( getFwFileBackupPath(pDest, fwBackupPath) , ret, EXIT);
		D("to restore fw from '%s'.", fwBackupPath);
        // restore backup of fw_in_ota_ver to system_partition 
		cmd[0] = BUSYBOX_PATH;
		cmd[1] = "cp";
		cmd[2] = "-a";
		cmd[3] = fwBackupPath;
		cmd[4] = pDest;
		cmd[5] = NULL;
		run(cmd[0], cmd);
	}
	else 
    {
        W("unexpected 'pOp' : %s.", pOp);
        goto EXIT;
	}

EXIT:
    return ret;
}

/**
 * 从制定的某个 固件文件(在 /system) 下的绝对路径, 获取对应的 ota 版本的 backup 路径中的目录的部分. 
 */
static int getFwFileBackupDirPath(const char* pPathInSystem, char* pBackupDirPath)
{
    int ret = 0;
    
    char* pDirPath = NULL;      // *pPathInSystem 中的 目录部分.

    /* 查找 *pPathInSystem 中的最后一个 '/'. */
    char* p = strrchr(pPathInSystem, '/');
    if ( NULL == p )
    {
        SET_ERROR_AND_JUMP("can not find '/' in '*pPathInSystem'.", ret, 1, EXIT);
    }
    pDirPath = strndup(pPathInSystem, p - pPathInSystem);
    
    strcpy(pBackupDirPath, "/radical_update/backup_of_fws_in_ota_ver");
    strcat(pBackupDirPath, "/.");       // *pDirPath 中为绝对路径. 
    strcat(pBackupDirPath, pDirPath);
    
EXIT:
    if ( NULL != pDirPath )
    {
        free(pDirPath);
    }

    return ret;
}

static int getFwFileBackupPath(const char* pPathInSystem, char* pBackupPath)
{
    int ret = 0;
    
    strcpy(pBackupPath, "/radical_update/backup_of_fws_in_ota_ver");
    strcat(pBackupPath, "/.");                  // *pPathInSystem 中为绝对路径. 
    strcat(pBackupPath, pPathInSystem);

EXIT:
    return ret;
}


static int run(const char *filename, char *const argv[] )
{
    struct stat s;
    int status;
    pid_t pid;

#ifdef ENABLE_VERBOSE_LOG
    char* pArg = NULL;
    uint i = 0;
    
    V("filename : %s.", filename);
    while ( 1 )
    {
        pArg = argv[i];
        i++;

        if ( NULL == pArg )
        {
            break;
        }
        else
        {
            V("argv[%d] : %s", i, argv[i] );
        }
    }
#endif
    if (stat(filename, &s) != 0) {
        E("cannot find '%s'", filename);
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        setpgid(0, getpid());
        /* execute */
        execv(filename, argv);
        E("can't execv %s (%s)", filename, strerror(errno) );
        /* exit */
        _exit(0);
    }

    if (pid < 0) {
        E("failed to fork and start '%s'", filename);
        return -1;
    }

    if (-1 == waitpid(pid, &status, WCONTINUED | WUNTRACED)) {
        E("wait for child error");
        return -1;
    }

    if ( 0 != WEXITSTATUS(status) )
    {
        E("executed '%s' return %d", filename, WEXITSTATUS(status) );
    }
    else
    {
        D("executed '%s' return %d", filename, WEXITSTATUS(status) );
    }

    return 0;
}

static int setRuIsApplied()
{
    int ret = 0;
    int fd = -1;
    
    fd = creat(FLAG_FILE_RU_IS_APPLIED,  S_IRUSR | S_IWUSR);
    if ( -1 == fd )
    {
        SET_ERROR_AND_JUMP("fail to create file, err : %s.", ret, errno, EXIT, strerror(errno) );
    }

EXIT:
    if ( -1 != fd )
    {
        close(fd);
    }

    return ret;
}


static int setRuNotApplied()
{
    int ret = 0;

    CHECK_FUNC_CALL( unlink(FLAG_FILE_RU_IS_APPLIED) , ret, EXIT);
EXIT:
    return ret;
}



