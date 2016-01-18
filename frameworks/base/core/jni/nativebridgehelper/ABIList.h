/*
 * Copyright (C) 20[14] Intel Corporation.  All rights reserved.
 * Intel Confidential                                  RS-NDA # RS-8051151
 * This [file/library] contains Houdini confidential information of Intel Corporation
 * which is subject to a non-disclosure agreement between Intel Corporation
 * and you or your company.
 */

#ifndef __ABILIST_H__
#define __ABILIST_H__

#include <utils/KeyedVector.h>
#include <sys/types.h>
#include <ScopedUtfChars.h>

using namespace android;
namespace nativebridgehelper {
#define ARMABI      "armeabi"
#define ARMV7ABI    "armeabi-v7a"
#define ARM64ABI    "arm64_v8a"
#define X86ABI      "x86"
#define X8664ABI    "x86_64"
#define ABI_TYPE_MAX    5

class ABIList {
public:
    ABIList(Vector<ScopedUtfChars*>* list);
    ~ABIList();

    bool isX86Compatible(size_t idx);
    bool getIdxByName(const char* name, size_t* idx);
    const char* getNameByIdx(int idx);
private:
    bool isX86Compatible(const char* name);
    size_t mABINum;
    Vector<ScopedUtfChars*>* mList;
};
}//namespace
#endif//__ABILIST_H__
