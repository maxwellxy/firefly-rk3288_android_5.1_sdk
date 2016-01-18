/*
 * xml_helper.c
 *
 *  Created on: 2013-4-27
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

extern void customHandler(char *command, int argc, char **argv);

static volatile bool gIfMatch = false;
static xmlChar *gArea;
static xmlChar *gLocal;
static xmlChar *gLanguage;
static xmlChar *gOperator;
static xmlChar *gReserve;
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

static void cust_OnStartElementNs( void *ctx,
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
	xmlChar *attr4 = NULL;
	xmlChar *attr5 = NULL;
	char odex_path[128];
	char *odex_argv[1];

	LOGV("start element tag: %s, attr number %d, default attr num %d, gIfMatch=%d \n",
	    localname, nb_attributes, nb_defaulted, gIfMatch);

	if(!xmlStrcmp(localname, BAD_CAST "if")) {
		attr1 = getAttrValueByName(BAD_CAST "language", nb_attributes, attributes);
		attr2 = getAttrValueByName(BAD_CAST "region", nb_attributes, attributes);
		attr3 = getAttrValueByName(BAD_CAST "operator", nb_attributes, attributes);
		attr4 = getAttrValueByName(BAD_CAST "reserve", nb_attributes, attributes);
		attr5 = getAttrValueByName(BAD_CAST "area", nb_attributes, attributes);

		gIfMatch = true;
		if(attrCmp(attr1, gLanguage)) {
			gIfMatch = false;
		}

		if(attrCmp(attr2, gLocal)) {
			gIfMatch = false;
		}

		if(attrCmp(attr3, gOperator)) {
			gIfMatch = false;
		}

		if(attrCmp(attr4, gReserve)) {
			gIfMatch = false;
		}

		if(attrCmp(attr5, gArea)) {
			gIfMatch = false;
		}

		if(gIfMatch) {
			LOGI("==========> catch a cust condition\n");
		}
	}else if(!xmlStrcmp(localname, BAD_CAST "cp") || !xmlStrcmp(localname, BAD_CAST "CP")) {
		if(gIfMatch) {
			// do copy
			attr1 = getAttrValueByName(BAD_CAST "src", nb_attributes, attributes);
			attr2 = getAttrValueByName(BAD_CAST "des", nb_attributes, attributes);
			attr3 = getAttrValueByName(BAD_CAST "mode", nb_attributes, attributes);

			LOGD("cp %s %s %s\n", attr1, attr2, attr3);
			char* argv[3];
			argv[0] = (char *)attr1;
			argv[1] = (char *)attr2;
			argv[2] = (char *)attr3;
			gHandlerFun((char *)localname, 3, argv);

			//rm odex file
			if(strstr(argv[1], "system/app/") != NULL && strstr(argv[1], "apk") != NULL) {
				char *tmp = strstr(argv[1], "apk");
				memset(odex_path, 0, sizeof(odex_path));
				strncpy(odex_path, argv[1], tmp - argv[1]);
				strcat(odex_path, "odex");
				LOGD("rm %s\n", odex_path);
				odex_argv[0] = odex_path;
				customHandler("rm", 1, odex_argv);
			}
		}
	}else if(!xmlStrcmp(localname, BAD_CAST "rm") || !xmlStrcmp(localname, BAD_CAST "RM")) {
		if(gIfMatch) {
			// do remove
			attr1 = getAttrValueByName(BAD_CAST "src", nb_attributes, attributes);

			LOGD("rm %s \n", attr1);
			char *argv[1];
			argv[0] = (char *)attr1;
			gHandlerFun((char *)localname, 1, argv);

			//rm odex file
			if(strstr(argv[0], "system/app/") != NULL && strstr(argv[0], "apk") != NULL) {
				char *tmp = strstr(argv[0], "apk");
				memset(odex_path, 0, sizeof(odex_path));
				strncpy(odex_path, argv[0], tmp - argv[0]);
				strcat(odex_path, "odex");
				LOGD("rm %s\n", odex_path);
				odex_argv[0] = odex_path;
				gHandlerFun("rm", 1, odex_argv);
			}
		}
	}else if(!xmlStrcmp(localname, BAD_CAST "set") || !xmlStrcmp(localname, BAD_CAST "SET")) {
		if(gIfMatch) {
			// do remove
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

	if(attr4 != NULL) {
		xmlFree(attr4);
	}

	if(attr5 != NULL) {
		xmlFree(attr5);
	}
}


static void cust_OnEndElementNs(
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


static void cust_OnCharacters(void *ctx, const xmlChar *ch, int len)
{

}


int parse_area(FILE *f,
		char *area,
		char *local,
		char *language,
		char *operator,
		char *reserve,
		handlerFun handler)
{
	int readcount;
    char chunk[1024];

    gArea = (xmlChar*)area;
    gLocal = BAD_CAST local;
    gLanguage = BAD_CAST language;
    gOperator = BAD_CAST operator;
    gReserve = BAD_CAST reserve;
    gHandlerFun = handler;

    gIfMatch = false;

    xmlInitParser();

    xmlSAXHandler SAXHander;
    memset(&SAXHander, 0, sizeof(xmlSAXHandler));
    SAXHander.initialized = XML_SAX2_MAGIC;
    SAXHander.startElementNs = cust_OnStartElementNs;
    SAXHander.endElementNs = cust_OnEndElementNs;
    SAXHander.characters = cust_OnCharacters;

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
