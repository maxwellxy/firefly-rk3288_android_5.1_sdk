/*
 * sdtool.h
 *
 *  Created on: 2014-3-20
 *      Author: mmk
 */

#ifndef SDTOOL_H_
#define SDTOOL_H_
//#include <map>
#include <vector>
#include <string>
#include <sstream>

typedef struct {
	char* name;
	char* value;
}RKSdBootCfgItem;
typedef struct{
	std::string strKey;
	std::string strValue;
}STRUCT_SD_CONFIG_ITEM,*PSTRUCT_SD_CONFIG_ITEM;
typedef std::vector<STRUCT_SD_CONFIG_ITEM> VEC_SD_CONFIG;


#endif /* SDTOOL_H_ */
