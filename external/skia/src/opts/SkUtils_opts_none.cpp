
/*
 * Copyright 2009 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkUtils.h"

SkMemset16Proc SkMemset16GetPlatformProc() {
    return NULL;
}

SkMemset32Proc SkMemset32GetPlatformProc() {
    return NULL;
}

SkSetPixels16Proc SkSetPixelRow16GetPlatformProc() {
    return NULL;
}

SkSetPixels32Proc SkSetPixelRow32GetPlatformProc() {
    return NULL;
}

SkSetPixels16Proc SkSetPixelRect16GetPlatformProc() {
    return NULL;
}

SkSetPixels32Proc SkSetPixelRect32GetPlatformProc() {
    return NULL;
}

SkMemcpy32Proc SkMemcpy32GetPlatformProc() {
   return NULL;
}
