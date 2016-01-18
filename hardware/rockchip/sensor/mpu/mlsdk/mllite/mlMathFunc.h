/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
#ifndef INVENSENSE_INV_MATH_FUNC_H__
#define INVENSENSE_INV_MATH_FUNC_H__
#ifdef INV_INCLUDE_LEGACY_HEADERS
#include "mlMathFunc_legacy.h"
#endif

#define NUM_ROTATION_MATRIX_ELEMENTS (9)
#define ROT_MATRIX_SCALE_LONG  (1073741824)
#define ROT_MATRIX_SCALE_FLOAT (1073741824.0f)
#define ROT_MATRIX_LONG_TO_FLOAT( longval ) \
    ((float) ((longval) / ROT_MATRIX_SCALE_FLOAT ))
#define SIGNM(k)((int)(k)&1?-1:1)

#ifdef __cplusplus
extern "C" {
#endif

    static inline float inv_q30_to_float(long q30)
    {
        return (float) q30 / ((float) (1 << 30));
    }

    static inline double inv_q30_to_double(long q30)
    {
        return (double) q30 / ((double) (1 << 30));
    }

/* UMPL_ELIMINATE_64BIT Notes:
 * An alternate implementation using float instead of long long accumulators
 * is provided for q29_mult and q30_mult.
 * When long long accumulators are used and an alternate implementation is not
 * available, we eliminate the entire function and header with a macro.
 */

#ifndef UMPL_ELIMINATE_64BIT
    struct filter_long {
        int length;
        const long *b;
        const long *a;
        long *x;
        long *y;
    };

    void inv_filter_long(struct filter_long *state, long x);
#endif

    long inv_q29_mult(long a, long b);
    long inv_q30_mult(long a, long b);

#ifndef UMPL_ELIMINATE_64BIT
    long inv_q30_div(long a, long b);
    long inv_q_shift_mult(long a, long b, int shift);
#endif

    void inv_q_mult(const long *q1, const long *q2, long *qProd);
    void inv_q_add(long *q1, long *q2, long *qSum);
    void inv_q_normalize(long *q);
    void inv_q_invert(const long *q, long *qInverted);
    void inv_q_multf(const float *q1, const float *q2, float *qProd);
    void inv_q_addf(const float *q1, const float *q2, float *qSum);
    void inv_q_normalizef(float *q);
    void inv_q_norm4(float *q);
    void inv_q_invertf(const float *q, float *qInverted);
    void inv_quaternion_to_rotation(const long *quat, long *rot);
    unsigned char *inv_int32_to_big8(long x, unsigned char *big8);
    long inv_big8_to_int32(const unsigned char *big8);
    short inv_big8_to_int16(const unsigned char *big8);
    short inv_little8_to_int16(const unsigned char *little8);
    unsigned char *inv_int16_to_big8(short x, unsigned char *big8);
    float inv_matrix_det(float *p, int *n);
    void inv_matrix_det_inc(float *a, float *b, int *n, int x, int y);
    double inv_matrix_detd(double *p, int *n);
    void inv_matrix_det_incd(double *a, double *b, int *n, int x, int y);
    float inv_wrap_angle(float ang);
    float inv_angle_diff(float ang1, float ang2);
    void inv_quaternion_to_rotation_vector(const long *quat, long *rot);

#ifdef __cplusplus
}
#endif
#endif                          // INVENSENSE_INV_MATH_FUNC_H__
