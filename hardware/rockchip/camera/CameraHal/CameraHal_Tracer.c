#include "CameraHal_Tracer.h"
#include "ebase/trace.h"

static int gCameraHalLogLevel;

int getTracerLevel(void)
{
    return gCameraHalLogLevel;
}

int setTracerLevel(int new_level)
{
    gCameraHalLogLevel = new_level;
    ALOGD("gCameraHalLogLevel: %d",gCameraHalLogLevel);
    if (gCameraHalLogLevel == 0) {
        SET_TRACE_LEVEL(TRACE_NOTICE0);
    } else if (gCameraHalLogLevel == 1) {
        SET_TRACE_LEVEL(TRACE_NOTICE1);
    } else if (gCameraHalLogLevel == 2) {
        SET_TRACE_LEVEL(INFO);
    }

    return 0;
}


