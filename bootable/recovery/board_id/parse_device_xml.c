/*
 * parse_device.c
 *
 *  Created on: 2013-5-8
 *      Author: mmk
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
#include "parse_xml.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/xpath.h"
#include "libxml/SAX2.h"
#include "libxml/xmlstring.h"

#include "common.h"

static bool gIfMatch = false;
/* 当前机器上, 当前定制处理的设备实例机器类型的字串标识. 诸如 type="tp" dev="Goodix-TS"; 用来匹配 device.xml 中的对应 element. */
static xmlChar *gDev;
static xmlChar *gType;
/**
 * 根据 device.xml 的内容, 具体完成定制操作的策略回调函数. 
 * 对于 custom 操作, 是 custom.c, customHandler(). 
 * 对于 restore 操作, 是 restore.c, restoreHandler(). 
 */
static handlerFun gHandlerFun;



static xmlChar* getAttrValueByName(xmlChar *name, int attrNum, const xmlChar **attributes) {
	LOGV("getAttrValueByName: attr Num %d \n", attrNum);
	int i;
	for(i = 0; i < attrNum; i++) {
		LOGV("attrname = %s\n", attributes[i*5]);
		if(!xmlStrcmp(attributes[i*5], name)) {
			const xmlChar* str = xmlStrchr(attributes[i*5 + 3], '"');
			xmlChar* ret = xmlStrndup(attributes[i*5 + 3], str - attributes[i*5 + 3]);
			LOGV("value = %s \n", ret);
			return ret;
		}
	}

	return NULL;
}

static int attrCmp(xmlChar *attr, xmlChar *value) {
	if(attr == NULL && value == NULL) {
		return 0;
	}

	if(attr != NULL && value == NULL) {
		return -1;
	}

	if(attr == NULL && value != NULL) {
		return 0;
	}

	LOGV("attrcmp: attr=%s, value=%s \n", attr, value);
	if(*attr == '!') {
		if(!xmlStrcmp(attr + 1, value)) {
			return -1;
		}
	}else {
		if(xmlStrcmp(attr, value)) {
			return -1;
		}
	}

	return 0;
}

static void device_OnStartElementNs(
    void *ctx,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes)
{
	xmlChar *attr1 = NULL;
	xmlChar *attr2 = NULL;
	xmlChar *attr3 = NULL;

	if(!xmlStrcmp(localname, BAD_CAST "if")) {
		attr1 = getAttrValueByName(BAD_CAST "type", nb_attributes, attributes);
		attr2 = getAttrValueByName(BAD_CAST "dev", nb_attributes, attributes);

		gIfMatch = true;
		if(attrCmp(attr1, gType)) {
			gIfMatch = false;
		}

		if(attrCmp(attr2, gDev)) {
			gIfMatch = false;
		}

		if(gIfMatch) {
			LOGE("==========> catch a device condition\n");
		}
	}else if(!xmlStrcmp(localname, BAD_CAST "cp") || !xmlStrcmp(localname, BAD_CAST "CP")) {
		if(gIfMatch) {
			// do copy
			attr1 = getAttrValueByName(BAD_CAST "src", nb_attributes, attributes);
			attr2 = getAttrValueByName(BAD_CAST "des", nb_attributes, attributes);
			attr3 = getAttrValueByName(BAD_CAST "mode", nb_attributes, attributes);

			LOGD("cp %s %s %s\n", attr1, attr2, attr3);
			char *argv[3];
			argv[0] = (char *)attr1;
			argv[1] = (char *)attr2;
			argv[2] = (char *)attr3;
			gHandlerFun((char *)localname, 3, argv);
		}
	}else if(!xmlStrcmp(localname, BAD_CAST "rm") || !xmlStrcmp(localname, BAD_CAST "RM")) {
		if(gIfMatch) {
			// do remove
			attr1 = getAttrValueByName(BAD_CAST "src", nb_attributes, attributes);

			LOGD("rm %s \n", attr1);
			char *argv[1];
			argv[0] = (char *)attr1;
			gHandlerFun((char *)localname, 1, argv);
		}
	}else if(!xmlStrcmp(localname, BAD_CAST "set") || !xmlStrcmp(localname, BAD_CAST "SET")) {
		if(gIfMatch) {
			// do set
			attr1 = getAttrValueByName(BAD_CAST "name", nb_attributes, attributes);
			attr2 = getAttrValueByName(BAD_CAST "value", nb_attributes, attributes);
			LOGD("set %s=%s \n", attr1, attr2);
			char *argv[2];
			argv[0] = (char *)attr1;
			argv[1] = (char *)attr2;
			gHandlerFun((char *)localname, 2, argv);
		}
	}

	if(attr1 != NULL) {
		xmlFree(attr1);
	}

	if(attr2 != NULL) {
		xmlFree(attr2);
	}

	if(attr3 != NULL) {
		xmlFree(attr3);
	}
}


static void device_OnEndElementNs(
    void* ctx,
    const xmlChar* localname,
    const xmlChar* prefix,
    const xmlChar* URI )
{
	LOGV("end element tag: %s \n", localname);

	if(!xmlStrcmp(localname, BAD_CAST "if")) {
		gIfMatch = false;
	}
}


static void device_OnCharacters(void *ctx, const xmlChar *ch, int len)
{

}


int parse_device(FILE *f, char *dev, char *type, handlerFun handler)
{
	int readcount;
    char chunk[1024];

    gDev = BAD_CAST dev;
    gType = BAD_CAST type;
    gHandlerFun = handler;

    gIfMatch = false;

    xmlInitParser();

    xmlSAXHandler SAXHander;
    memset(&SAXHander, 0, sizeof(xmlSAXHandler));
    SAXHander.initialized = XML_SAX2_MAGIC;
    SAXHander.startElementNs = device_OnStartElementNs;
    SAXHander.endElementNs = device_OnEndElementNs;
    SAXHander.characters = device_OnCharacters;

    fseek(f, 0, SEEK_SET);
    readcount = fread(chunk, 1, 4, f);
    xmlParserCtxtPtr ctxt = xmlCreatePushParserCtxt(&SAXHander, NULL, chunk, readcount, NULL);


    while ((readcount = fread(chunk, 1, sizeof(chunk), f)) > 0)
    {
        if(xmlParseChunk(ctxt, chunk, readcount, 0))
        {
            xmlParserError(ctxt, "xmlParseChunk");
            return 1;
        }
    }
    xmlParseChunk(ctxt, chunk, 0, 1);

    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
    return 0;
}


