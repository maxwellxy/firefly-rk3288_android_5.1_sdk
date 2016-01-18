/*
 * Copyright (C) 20[14] Intel Corporation.  All rights reserved.
 * Intel Confidential                                  RS-NDA # RS-8051151
 * This [file/library] contains Houdini confidential information of Intel Corporation
 * which is subject to a non-disclosure agreement between Intel Corporation
 * and you or your company.
 */

#ifndef ___LIBS_APPLICATIONLIBINFO_H
#define ___LIBS_APPLICATIONLIBINFO_H

#include <sys/types.h>
#include <utils/List.h>

#include "ABIList.h"

#define SO_NAME_MAX     128
using namespace android;
namespace nativebridgehelper {

struct soInfo {
    bool isThirdPartySO;
    char name[SO_NAME_MAX];
    bool abiType[ABI_TYPE_MAX];
};

class AppLibInfo {
public:
    AppLibInfo(void* apkHandle, const char* path, ABIList* al);
    ~AppLibInfo();

    int  whoAmI(int sysPrfrAbi);
private:
    void dump(void);
    void addAndSort(struct soInfo* info);
    bool isKnownThirdPartySO(const char* name);
    bool isLinkHyphens(const char c);
    bool isNumeric(const char c);
    bool detectX86Keywords(bool is64ABIType);
    bool cmpIgnPst(const char* s1, const char* s2);
    bool ignPst(const char* postfix);

    ABIList* mAl;
    List<struct soInfo*>* mSOList;
    size_t mSONum[ABI_TYPE_MAX];
};

bool isInOEMWhiteList(const char* pkgName);
}//namespace

#endif//___LIBS_APPLICATIONLIBINFO_H
