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
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <androidfw/ZipFileRO.h>

#include "ABIList.h"
#include "AppLibInfo.h"
#include "ELFParser.h"

#define APK_LIB "lib/"
#define APK_LIB_LEN (sizeof(APK_LIB) - 1)

#define LIB_PREFIX "/lib"
#define LIB_PREFIX_LEN (sizeof(LIB_PREFIX) - 1)

#define LIB_SUFFIX ".so"
#define LIB_SUFFIX_LEN (sizeof(LIB_SUFFIX) - 1)

using namespace android;
using namespace nativebridgehelper;

#define ARR_SIZE(x) sizeof(x)/sizeof(x[0])

const char* oemSpecific[] = {
    #include "OEMWhiteList"
};
const char* thirdPartySOes[] = {
    #include "ThirdPartySO"
};

bool nativebridgehelper::isInOEMWhiteList(const char* pkgName) {
    if (!pkgName) return false;

    bool ret = false;
    size_t len = ARR_SIZE(oemSpecific);
    for (size_t i = 0; i < len; i++) {
        if (oemSpecific[i] && strlen(pkgName) >= strlen(oemSpecific[i])) {
            if (!strcmp(pkgName, oemSpecific[i])) {
                ret = true;
                break;
            }
        }
    }

    return ret;
}

/*
 * ===========================================================================
 *     AppLibInfo
 * ===========================================================================
 */
AppLibInfo::AppLibInfo(void* apkHandle, const char* path, ABIList* al) {
    if (!apkHandle && !path) {
        return;
    }

    if (path) {
        ALOGV("apk is %s", path);
    }

    mAl = al;
    mSOList = new List<struct soInfo*>();
    memset(mSONum, 0, sizeof(mSONum));

    ZipFileRO* zipFile = NULL;
    if (apkHandle) {
        zipFile = reinterpret_cast<ZipFileRO*>(apkHandle);
    } else {
        zipFile = ZipFileRO::open(path);
    }

    void* cookie = NULL;
    if (!zipFile->startIteration(&cookie)) {
        return;
    }

    ZipEntryRO next = NULL;
    char fileName[SO_NAME_MAX];
    char* buffer = NULL;
    bool mixedX86AndARM = false;
    while ((next = zipFile->nextEntry(cookie)) != NULL) {
        if (zipFile->getEntryFileName(next, fileName, sizeof(fileName))) {
            continue;
        }

        // Make sure we're in the lib directory of the ZIP.
        // And make sure it is a .so file
        if (strncmp(fileName, APK_LIB, APK_LIB_LEN)) {
            continue;
        }

        // pick out directory like lib/xxxx/
        if (!strstr(fileName, ".so")) {
            continue;
        }

        ALOGV(".so: %s", fileName);

        // according to observation, any library file with name like
        // "*.wav.so" and "*.dat.so" is not a real .so files
        if (strstr(fileName, ".wav.so") || strstr(fileName, ".dat.so")) {
            continue;
        }

        // It is a real .so file, prepare to record
        // find abi name and focus on what we care: arm(s) and x86(s)
        const char* lastSlash = strrchr(fileName, '/');
        const char* cpuAbiOffset = fileName + APK_LIB_LEN;
        const size_t cpuAbiRegionSize = lastSlash - cpuAbiOffset;

        char tmpName[SO_NAME_MAX];
        memset(tmpName, 0, sizeof(tmpName));
        if (cpuAbiRegionSize < SO_NAME_MAX) {
            memcpy(tmpName, cpuAbiOffset, sizeof(char) * cpuAbiRegionSize);
        }
        size_t abiIdx = 0;
        if (!mAl->getIdxByName(tmpName, &abiIdx)) {
            continue;
        }
        if (mAl->isX86Compatible(abiIdx)) {
            if (mixedX86AndARM) {
                continue;
            }

            // check if ISV puts some ARM .so into lib/x86/
            if (buffer != NULL) {
                free(buffer);
                buffer = NULL;
            }

            size_t unCompLen = 0;
            if (!zipFile->getEntryInfo(next, NULL, &unCompLen, NULL, NULL, NULL, NULL)) {
                continue;
            }

            buffer = (char*)malloc(unCompLen * sizeof(char));
            memset(buffer, 0, sizeof(char) * unCompLen);

            if (zipFile->uncompressEntry(next, buffer, unCompLen * sizeof(char))) {
                if (!is_elf_from_buffer(buffer)) {
                    continue;
                }

                if (is_arm_elf_from_buffer(buffer)) {
                    mixedX86AndARM = true;
                    mSONum[abiIdx] = 0;
                    continue;
                }
            } else {
                ALOGE("%s: uncompress failed\n", fileName);
                continue;
            }
        }
        mSONum[abiIdx]++;

        // fill library name
        lastSlash++;
        if (!lastSlash) {
            continue;
        }
        const size_t libraryRegionSize = strlen(lastSlash);
        memset(tmpName, 0, sizeof(tmpName));
        memcpy(tmpName, (lastSlash + 3), sizeof(char) *
                (libraryRegionSize - 6));
        for (size_t j = 0; j < strlen(tmpName); j++) {
            tmpName[j] = tolower(tmpName[j]);
        }

        struct soInfo* soinfo = NULL;
        List<struct soInfo*>::iterator it = mSOList->begin();
        while (it != mSOList->end()) {
            if (!strcmp((*it)->name, tmpName)) {
            //if (cmpIgnPst((*it)->name, tmpName)) {
                soinfo = (*it);
                break;
            }
            it++;
        }
        if (!soinfo) {
            soinfo = (struct soInfo*)malloc(sizeof(struct soInfo));
            if (!soinfo) {
                ALOGE("MALLOC ERROR");
                break;
            }
            memset(soinfo, 0, sizeof(struct soInfo));
            memset(soinfo->name, 0, sizeof(soinfo->name));
            memcpy(soinfo->name, tmpName, sizeof(tmpName));
            addAndSort(soinfo);
        }
        soinfo->abiType[abiIdx] = true;
    }

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    zipFile->endIteration(cookie);
    //dump();
}

AppLibInfo::~AppLibInfo() {
    List<struct soInfo*>::iterator it = mSOList->begin();
    while (it != mSOList->end()) {
        if (*it) {
            free(*it);
        }
        it++;
    }
    delete mSOList;
}

void AppLibInfo::addAndSort(struct soInfo* info) {
    List<struct soInfo*>::iterator it = mSOList->begin();
    while (it != mSOList->end()) {
        if (0 > strcmp(info->name, (*it)->name)) {
            break;
        }
        it++;
    }
    mSOList->insert(it, info);
}

bool AppLibInfo::isKnownThirdPartySO(const char* name) {
    if (!name) return false;

    bool ret = false;
    size_t len = ARR_SIZE(thirdPartySOes);
    for (size_t i = 0; i < len; i++) {
        if (cmpIgnPst(name, thirdPartySOes[i])) {
            //if (!strcmp(name, thirdPartySOes[i])) {
            ret = true;
            break;
        }
    }

    if (ret) {
        ALOGV("%s is known 3rd party SO", name);
    }

    return ret;
}


void AppLibInfo::dump(void) {
    ALOGV("CURRENT APPLICAITON'S LIBRARY INFOMATION DUMP:");
    ALOGV("|name|:[%s  %s  %s  %s  %s]",
            mAl->getNameByIdx(0), mAl->getNameByIdx(1),
            mAl->getNameByIdx(2), mAl->getNameByIdx(3),
            mAl->getNameByIdx(4));
    List<struct soInfo*>::iterator it = mSOList->begin();
    while (it != mSOList->end()) {
        ALOGV("|%20s|:[%d  %d  %d  %d  %d]",
                (*it)->name, (*it)->abiType[0], (*it)->abiType[1],
                (*it)->abiType[2], (*it)->abiType[3], (*it)->abiType[4]);
        it++;
    }
}

bool AppLibInfo::isLinkHyphens(const char c) {
    return (c == '.' || c == '-' || c == '_');
}

bool AppLibInfo::isNumeric(const char c) {
    return ((c <= '9') && (c >= '0'));
}

bool AppLibInfo::detectX86Keywords(bool is64ABIType) {
    size_t idx = 0;
    if (is64ABIType) {
        mAl->getIdxByName(X8664ABI, &idx);
    } else {
        mAl->getIdxByName(X86ABI, &idx);
    }

    if (!mSONum[idx]) {
        return false;
    }

    static const char* x86KyWrd[] = {
        "intel", "intl", "atom", "x86", "x64",
    };
    size_t listLen = sizeof(x86KyWrd) / sizeof(x86KyWrd[0]);
    bool ret = false;
    List<struct soInfo*>::iterator it = mSOList->begin();
    while (it != mSOList->end()) {
        if ((*it)->abiType[idx]) {
            const char* name = (*it)->name;
            size_t i = 0;
            while (i < listLen) {
                char* p = strstr(name, x86KyWrd[i]);
                if (p 
                        && ((isLinkHyphens(*(p - 1)) || isLinkHyphens(*(p + 1)))
                            || (isNumeric(*(p - 1)) || isNumeric(*(p + 1))))) {
                    ret = true;
                    break;
                }
                i++;
            }
            if (ret) {
                break;
            }
        }
        it++;
    }

    return ret;
}

int AppLibInfo::whoAmI(int sysPrfr) {
    // sysPrfr comes from android detection result.
    // After previous filter, it should be "x86" or "x86-64"
    int ret = sysPrfr;
    do {
        const char* sysPrfrName = mAl->getNameByIdx(sysPrfr);
        bool is64ABIType = strstr(sysPrfrName, X8664ABI) ||
            strstr(sysPrfrName, ARM64ABI);

        size_t x86Idx = 0;
        size_t referenceARMIdx = 0;
        if (is64ABIType) {
            mAl->getIdxByName(X8664ABI, &x86Idx);
            mAl->getIdxByName(ARM64ABI, &referenceARMIdx);
        } else {
            mAl->getIdxByName(X86ABI, &x86Idx);

            size_t armv7Idx = 0;
            mAl->getIdxByName(ARMV7ABI, &armv7Idx);
            size_t armIdx = 0;
            mAl->getIdxByName(ARMABI, &armIdx);

            referenceARMIdx = mSONum[armv7Idx] ? armv7Idx : armIdx;
        }

        if (!mSONum[x86Idx] && !mSONum[referenceARMIdx]) {
            ALOGV("B0, pure JAVA");
            break;
        }

        if (!mSONum[x86Idx]) {
            ALOGV("B0, no any x86 .so");
            ret = referenceARMIdx;
            break;
        }

        // 0. only x86/ or x8664/ has .so(es)
        if (!mSONum[referenceARMIdx]) {
            ALOGV("B0, only x86 .so");
            break;
        }

        if (detectX86Keywords(is64ABIType)) {
            ALOGV("B1, x86 keywords");
            break;
        }

        size_t referenceARMISVSO = 0;
        size_t x86ISVSO = 0;
        bool found = false;
        List<struct soInfo*>::iterator it = mSOList->begin();
        while (it != mSOList->end()) {
            if (isKnownThirdPartySO((*it)->name)) {
                (*it)->isThirdPartySO = true;
                if ((*it)->abiType[referenceARMIdx] && !(*it)->abiType[x86Idx]) {
                    found = true;
                }
            } else {
                (*it)->isThirdPartySO = false;
                if ((*it)->abiType[x86Idx]) {
                    x86ISVSO++;
                }
                if ((*it)->abiType[referenceARMIdx]) {
                    referenceARMISVSO++;
                }
            }

            it++;
        }

        if (found) {
            ret = referenceARMIdx;
            ALOGV("B2, x86/ misses at least one 3rd party.so");
            break;
        }

        if (!x86ISVSO) {
            if (!referenceARMISVSO) {
                ALOGV("B3, apk just has 3rd party .so and x86 matches all");
            } else {
                ret = referenceARMIdx;
                ALOGV("B4, x86 misses ISV .so");
            }
        } else {
            ALOGV("B5, x86/ has at least one ISV .so");
        }

    } while (0);

    ALOGV("Actually, I am %s", mAl->getNameByIdx(ret));
    return ret;
}


bool AppLibInfo::cmpIgnPst(const char* s1, const char* s2) {
    bool ret = true;
    size_t i = 0;

    while (*(s1 + i) != '\0' && *(s2 + i) != '\0') {
        if (*(s1 + i) != *(s2 + i)) {
            break;
        }
        i++;
    }

    ret = ignPst(s1 + i) && ignPst(s2 + i);
//  ALOGV("cmpIgnPst. <%s>-<%s>,  %d", s1, s2, ret);
    return ret;
}

bool AppLibInfo::ignPst(const char* postfix) {
    if (!postfix) {
        return true;
    }

    static const char* knownPstList[] = {
            "sdk", "ndk",
            "arm", "armv", "neon", "noneon",
            "nvidia", "tegra", "qcom",
            "jellybean", "jb", "ics", "honeycomb", "gingerbread",
            "froyo", "hc", "gb", "kk", "cupcake", "eclair", "dount",
    };
    size_t listLen = ARR_SIZE(knownPstList);

    int offset = strlen(postfix) - 1;
    size_t n = 1;
    while (offset >= 0) {
        if (offset == 0 || isLinkHyphens(*(postfix + offset))) {
            const char* tmpStart = postfix + offset;
            while (isLinkHyphens(*tmpStart)) {
                tmpStart++;
                n--;
            }

//                ALOGV("check %s, n=%d", tmpStart, n);

            bool passFlag = false;
            for (size_t i = 0; i < listLen; i++) {
                if (!strncmp(tmpStart, knownPstList[i], n)) {
                    passFlag = true;
                    break;
                }
            }

            if (!passFlag) {
                if (*tmpStart == 'v') {
                    tmpStart++;
                    n--;
                }

//                    ALOGV("check number %s, n=%d", tmpStart, n);
                bool allNumeric = true;
                size_t i = 1;
                while (*tmpStart != '\0' && i <= n) {
                    if (!isNumeric(*tmpStart)) {
                        allNumeric = false;
                        break;
                    }
                    tmpStart++;
                    i++;
                }

                if (!allNumeric) {
                    return false;
                }
            }

            n = 1;
        } else {
            ++n;
        }

        --offset;
    }
    return true;
}
