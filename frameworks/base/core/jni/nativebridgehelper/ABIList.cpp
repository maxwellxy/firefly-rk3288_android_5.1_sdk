/*
 * Copyright (C) 20[14] Intel Corporation.  All rights reserved.
 * Intel Confidential                                  RS-NDA # RS-8051151
 * This [file/library] contains Houdini confidential information of Intel Corporation
 * which is subject to a non-disclosure agreement between Intel Corporation
 * and you or your company.
 */
#define LOG_TAG "NativeLibraryHelper"
//#define LOG_NDEBUG 0

#include <cutils/log.h>
#include <string.h>
#include "ABIList.h"

using namespace android;
using namespace nativebridgehelper;
/*
 * ===========================================================================
 *     ABIList
 * ===========================================================================
 */

ABIList::ABIList(Vector<ScopedUtfChars*>* list) {
    mList = list;
    mABINum = mList->size();
}

ABIList::~ABIList() {
}

bool ABIList::isX86Compatible(size_t idx) {
    return isX86Compatible(getNameByIdx(idx));
}

bool ABIList::isX86Compatible(const char* name) {
    return !name || (0 == strcmp(name, X86ABI)) ||
        (0 == strcmp(name, X8664ABI));
}

bool ABIList::getIdxByName(const char* name, size_t* idx) {
    if (!name) {
        return false;
    }

    bool found = false;
    *idx = mABINum;

    size_t i = 0;
    while (i < mABINum) {
        if (!strcmp((mList->itemAt(i))->c_str(), name)) {
            *idx = i;
            found = true;
            break;
        }
        i++;
    }

    return found;
}

const char* ABIList::getNameByIdx(int idx) {
    if (0 > idx || (size_t)idx >= mABINum) {
        return "UnkNowN";
    }

    return (mList->itemAt(idx))->c_str();
}
