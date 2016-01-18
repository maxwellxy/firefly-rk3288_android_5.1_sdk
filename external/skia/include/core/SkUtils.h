/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkUtils_DEFINED
#define SkUtils_DEFINED

#include "SkTypes.h"

///////////////////////////////////////////////////////////////////////////////

/** Similar to memset(), but it assigns a 16bit value into the buffer.
    @param buffer   The memory to have value copied into it
    @param value    The 16bit value to be copied into buffer
    @param count    The number of times value should be copied into the buffer.
*/
void sk_memset16(uint16_t dst[], uint16_t value, int count);
typedef void (*SkMemset16Proc)(uint16_t dst[], uint16_t value, int count);
SkMemset16Proc SkMemset16GetPlatformProc();

/** Similar to memset(), but it assigns a 32bit value into the buffer.
    @param buffer   The memory to have value copied into it
    @param value    The 32bit value to be copied into buffer
    @param count    The number of times value should be copied into the buffer.
*/
void sk_memset32(uint32_t dst[], uint32_t value, int count);
typedef void (*SkMemset32Proc)(uint32_t dst[], uint32_t value, int count);
SkMemset32Proc SkMemset32GetPlatformProc();

/** Similar to memcpy(), but it copies count 32bit values from src to dst.
    @param dst      The memory to have value copied into it
    @param src      The memory to have value copied from it
    @param count    The number of values should be copied.
*/
void sk_memcpy32(uint32_t dst[], const uint32_t src[], int count);
typedef void (*SkMemcpy32Proc)(uint32_t dst[], const uint32_t src[], int count);
SkMemcpy32Proc SkMemcpy32GetPlatformProc();

///////////////////////////////////////////////////////////////////////////////

/** Similar to memset(), but it assigns a 16bit color into the buffer.
    @param buffer     The memory to have the color copied into it
    @param color      The 16bit color value to be copied into buffer
    @param count      The number of times the color should be copied into the buffer.
    @param totalCount The total number of times the color will be copied into the buffer.
                      This is used to indicate the total size of all the operations
                      called in a loop.
*/
void sk_set_pixel_row16_portable(uint16_t dst[], uint16_t color, int count, int totalCount);
typedef void (*SkSetPixelRow16Proc)(uint16_t dst[], uint16_t color, int count, int totalCount);
SkSetPixelRow16Proc SkSetPixelRow16GetPlatformProc();

/** Similar to memset(), but it assigns a 32bit color into the buffer.
    @param buffer     The memory to have the color copied into it
    @param color      The 32bit color value to be copied into buffer
    @param count      The number of times the color should be copied into the buffer.
    @param totalCount The total number of times the color will be copied into the buffer.
                      This is used to indicate the total size of all the operations
                      called in a loop.
*/
void sk_set_pixel_row32_portable(uint32_t dst[], uint32_t color, int count, int totalCount);
typedef void (*SkSetPixelRow32Proc)(uint32_t dst[], uint32_t color, int count, int totalCount);
SkSetPixelRow32Proc SkSetPixelRow32GetPlatformProc();

#ifndef SkSetPixelRow16
extern SkSetPixelRow16Proc SkSetPixelRow16;
#endif

#ifndef SkSetPixelRow32
extern SkSetPixelRow32Proc SkSetPixelRow32;
#endif

///////////////////////////////////////////////////////////////////////////////

/** An extension of SetPixelRow16(), that sets a rectangular area instead.
    @param buffer   The memory to have the color copied into it
    @param color    The 16bit color value to be copied into buffer
    @param width    The width of the rectangle, in pixels.
    @param height   The height of the rectangle, in pixels.
    @param rowBytes The width of one row in the buffer, in bytes. This is the
                    value that will be added to 'buffer' to get to the next row.
*/
void sk_set_pixel_rect16_portable(uint16_t dst[], uint16_t color, int width, int height, int rowBytes);
typedef void (*SkSetPixelRect16Proc)(uint16_t dst[], uint16_t color, int width, int height, int rowBytes);
SkSetPixelRect16Proc SkSetPixelRect16GetPlatformProc();

/** An extension of SetPixelRow32(), that sets a rectangular area instead.
    @param buffer   The memory to have the color copied into it
    @param color    The 16bit color value to be copied into buffer
    @param width    The width of the rectangle, in pixels.
    @param height   The height of the rectangle, in pixels.
    @param rowBytes The width of one row in the buffer, in bytes. This is the
                    value that will be added to 'buffer' to get to the next row.
*/
void sk_set_pixel_rect32_portable(uint32_t dst[], uint32_t color, int width, int height, int rowBytes);
typedef void (*SkSetPixelRect32Proc)(uint32_t dst[], uint32_t color, int width, int height, int rowBytes);
SkSetPixelRect32Proc SkSetPixelRect32GetPlatformProc();

#ifndef SkSetPixelRect16
extern SkSetPixelRect16Proc SkSetPixelRect16;
#endif

#ifndef SkSetPixelRect32
extern SkSetPixelRect32Proc SkSetPixelRect32;
#endif

///////////////////////////////////////////////////////////////////////////////

#define kMaxBytesInUTF8Sequence     4

#ifdef SK_DEBUG
    int SkUTF8_LeadByteToCount(unsigned c);
#else
    #define SkUTF8_LeadByteToCount(c)   ((((0xE5 << 24) >> ((unsigned)c >> 4 << 1)) & 3) + 1)
#endif

inline int SkUTF8_CountUTF8Bytes(const char utf8[]) {
    SkASSERT(utf8);
    return SkUTF8_LeadByteToCount(*(const uint8_t*)utf8);
}

int         SkUTF8_CountUnichars(const char utf8[]);
int         SkUTF8_CountUnichars(const char utf8[], size_t byteLength);
SkUnichar   SkUTF8_ToUnichar(const char utf8[]);
SkUnichar   SkUTF8_NextUnichar(const char**);
SkUnichar   SkUTF8_PrevUnichar(const char**);

/** Return the number of bytes need to convert a unichar
    into a utf8 sequence. Will be 1..kMaxBytesInUTF8Sequence,
    or 0 if uni is illegal.
*/
size_t      SkUTF8_FromUnichar(SkUnichar uni, char utf8[] = NULL);

///////////////////////////////////////////////////////////////////////////////

#define SkUTF16_IsHighSurrogate(c)  (((c) & 0xFC00) == 0xD800)
#define SkUTF16_IsLowSurrogate(c)   (((c) & 0xFC00) == 0xDC00)

int SkUTF16_CountUnichars(const uint16_t utf16[]);
int SkUTF16_CountUnichars(const uint16_t utf16[], int numberOf16BitValues);
// returns the current unichar and then moves past it (*p++)
SkUnichar SkUTF16_NextUnichar(const uint16_t**);
// this guy backs up to the previus unichar value, and returns it (*--p)
SkUnichar SkUTF16_PrevUnichar(const uint16_t**);
size_t SkUTF16_FromUnichar(SkUnichar uni, uint16_t utf16[] = NULL);

size_t SkUTF16_ToUTF8(const uint16_t utf16[], int numberOf16BitValues,
                      char utf8[] = NULL);

inline bool SkUnichar_IsVariationSelector(SkUnichar uni) {
/*  The 'true' ranges are:
 *      0x180B  <= uni <=  0x180D
 *      0xFE00  <= uni <=  0xFE0F
 *      0xE0100 <= uni <= 0xE01EF
 */
    if (uni < 0x180B || uni > 0xE01EF) {
        return false;
    }
    if ((uni > 0x180D && uni < 0xFE00) || (uni > 0xFE0F && uni < 0xE0100)) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

class SkAutoTrace {
public:
    /** NOTE: label contents are not copied, just the ptr is
        retained, so DON'T DELETE IT.
    */
    SkAutoTrace(const char label[]) : fLabel(label) {
        SkDebugf("--- trace: %s Enter\n", fLabel);
    }
    ~SkAutoTrace() {
        SkDebugf("--- trace: %s Leave\n", fLabel);
    }
private:
    const char* fLabel;
};
#define SkAutoTrace(...) SK_REQUIRE_LOCAL_VAR(SkAutoTrace)

#endif
