
#ifndef __INV_ML_FEATURES_H__
#define __INV_ML_FEATURES_H__

/**
 *  ml_features.h
 *  This header defines the features that are enabled
 *  for this build of MPL.
 *  The INV_FEATURE_ macros serve as a contract that members
 *  of the inv_obj struct will be non-null. This enables
 *  reduced feature builds to not allocate inv_obj members.
 */

#define INV_FEATURE_MAGNETOMETER
#define INV_FEATURE_PRESSURE

/* GYROTC options: _LEGACY or _UTC. Default LEGACY. */
#ifndef INV_FEATURE_GYROTC_UTC
#define INV_FEATURE_GYROTC_LEGACY
#else // UTC defined by preprocessor directive:
#ifdef INV_FEATURE_GYROTC_LEGACY
#error Feature GYROTC_LEGACY and GYROTC_UTC are both defined, should be mutually exclusive
#endif // ifdef _LEGACY
#endif // defined UTC

/* INV_FEATURE_ADVFUSION can be eliminated by UMPL_ELIMINATE_ADVFUSION */
#ifndef UMPL_ELIMINATE_ADVFUSION
#define INV_FEATURE_ADVFUSION
#define INV_ADVFEATURES_ENABLED 1
#else
#define INV_ADVFEATURES_ENABLED 0
#ifdef INV_FEATURE_ADVFUSION
#error UMPL_ELIMINATE_ADVFUSION and INV_FEATURE_ADVFUSION both defined.
#endif
#endif

/* New method for checking advanced fusion */
#if INV_ADVFEATURES_ENABLED == 0
#define IS_INV_ADVFEATURES_ENABLED(obj) 0
#else
#define IS_INV_ADVFEATURES_ENABLED(obj) ((obj).adv_fusion != NULL)
#endif

#define INV_FEATURE_CALMATS
#define INV_FEATURE_SYSSTRUCT


#endif /* __INV_ML_FEATURES_H__ */


