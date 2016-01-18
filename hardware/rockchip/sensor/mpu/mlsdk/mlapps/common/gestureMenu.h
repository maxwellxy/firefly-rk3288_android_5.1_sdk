/*
 $License:
    Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.
 $
 */
/***************************************************************************** *
 * $Id: gestureMenu.h 5705 2011-06-28 19:32:29Z nroyer $ 
 ******************************************************************************/
/**
 * @defgroup 
 * @brief  
 *
 * @{
 *      @file     gestureMenu.h
 *      @brief    
 *
 *
 */

#ifndef __GESTUREMENU_H__
#define __GESTUREMENU_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
    typedef struct sGestureMenuParams {
        /* Tap Params */
        int xTapThreshold;
        int yTapThreshold;
        int zTapThreshold;
        int tapTime;
        int nextTapTime;
        int maxTaps;
        float shakeRejectValue;

        /* Shake Params */
        int xShakeThresh;
        int yShakeThresh;
        int zShakeThresh;
        int xSnapThresh;
        int ySnapThresh;
        int zSnapThresh;
        int shakeTime;
        int nextShakeTime;
        int shakeFunction;
        int maxShakes;

        /* Yaw rotate params */
        int yawRotateTime;
        int yawRotateThreshold;

        /* Orientation */
        float orientationThreshold;
        int sensorsIndex;
        unsigned long available_sensors;
    } tGestureMenuParams;

    void     PrintGestureMenu(tGestureMenuParams const * const params) ;
    inv_error_t GestureMenuProcessChar(tGestureMenuParams * const params,char ch);
    void     PrintGesture(gesture_t* gesture);
    void     PrintOrientation(unsigned short orientation);
    void     GestureMenuSetDefaults(tGestureMenuParams * const params);
    void GestureMenuSetAvailableSensors(tGestureMenuParams * const params,
                                        unsigned long available_sensors);
    inv_error_t GestureMenuSetMpl(tGestureMenuParams const * const params);

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif // __GESTUREMENU_H__
