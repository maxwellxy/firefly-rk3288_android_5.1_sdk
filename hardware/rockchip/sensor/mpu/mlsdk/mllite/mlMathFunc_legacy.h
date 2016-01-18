#ifdef INV_USE_LEGACY_NAMES
#ifndef INV_INV_MATH_FUNC_LEGACY_H__
#define INV_INV_MATH_FUNC_LEGACY_H__

#define FilterLong inv_filter_long
#define q29_mult inv_q29_mult
#define q30_mult inv_q30_mult
#define MLQMult inv_q_mult
#define MLQAdd inv_q_add
#define MLQNormalize inv_q_normalize
#define MLQInvert inv_q_invert
#define MLQMultf inv_q_mult_float
#define MLQAddf inv_q_add_float
#define MLQNormalizef inv_q_normalize_float
#define MLNorm4 inv_q_norm4
#define MLQInvertf inv_q_invert_float
#define quaternionToRotationMatrix inv_quaternion_to_rotation
#define Long32ToBig8 inv_int32_to_big8
#define Big8ToLong32 inv_big8_to_int32
#define Short16ToBig8 inv_int16_to_big8
#define matDet inv_matrix_det
#define matDetInc inv_matrix_det_inc
#define matDetd inv_matrix_det_dbl
#define matDetIncd inv_matrix_det_inc_dbl
#define MLWrap inv_wrap_angle
#define MLAngDiff inv_angle_diff

#endif
#endif
