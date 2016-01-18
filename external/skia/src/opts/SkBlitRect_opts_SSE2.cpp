/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitRect_opts_SSE2.h"
#include "SkBlitRow.h"
#include "SkColorPriv.h"
#include "SkUtils.h"

/*
 * Implement ColorRect32 using the SkSetPixels function.
 * Only applicable if the color's alpha is opaque.
 */
void ColorRect32_SSE2(SkPMColor* destination,
                      int width, int height,
                      size_t rowBytes, uint32_t color) {
    if (0 == color) {
        return;
    }

    if (SkGetPackedA32(color) == 0xFF) {
        SkSetPixelRect32((uint32_t*)destination, color, width, height, rowBytes);
    } else {
        SkBlitRow::ColorRect32(destination, width, height, rowBytes, color);
    }
}
