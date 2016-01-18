/*$_FOR_ROCKCHIP_RBOX_$*/
//$_rbox_$_modify_$_chenzhi_20120309: add for pppoe

#define LOG_TAG "android_pppoe_PppoeNative.cpp"

#include "jni.h"
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#define LOGD ALOGD
#define LOGE ALOGE
#define LOGV ALOGV

namespace android {
static jint setupPppoeNative(JNIEnv* env, 
                                    jobject clazz, 
                                    jstring user, 
                                    jstring iface, 
                                    jstring dns1, 
                                    jstring dns2, 
                                    jstring password)
{
    int attempt;
    int err = 0;
    char* cmd = (char*)malloc(128);
    char* prop = (char*)malloc(PROPERTY_VALUE_MAX);
    
    LOGD("%s", __FUNCTION__);
    
    char *c_user = (char *)env->GetStringUTFChars(user, NULL);
    char *c_iface = (char *)env->GetStringUTFChars(iface, NULL);
    char *c_dns1 = (char *)env->GetStringUTFChars(dns1, NULL);
    char *c_dns2 = (char *)env->GetStringUTFChars(dns2, NULL);
    char *c_password = (char *)env->GetStringUTFChars(password, NULL);

    if (!strcmp(c_dns1, "")) {
        strcpy(c_dns1, "server");
    }
    
    sprintf(cmd, "pppoe_setup:%s %s no %s %s %s NONE", 
            c_user, c_iface, c_dns1, c_dns2, c_password);

    LOGD("setprop ctl.start %s", cmd);

    if (property_set("ctl.start", cmd) < 0) {
        LOGE("Failed to start pppoe_setup");
        err = -1;
        goto end;
    }

    for (attempt = 50; attempt > 0;  attempt--) {
        property_get("net.pppoe.status", prop, "");
        if (!strcmp(prop, "setuped")) {
            break;
        }
        usleep(100000);  // 100 ms retry delay
    }
    
    if (attempt == 0) {
        LOGE("%s: Timeout waiting for pppoe-setup", __FUNCTION__);
        err = -1;
        goto end;
    }
    err = 0;
end:
    env->ReleaseStringUTFChars(user,c_user);
    env->ReleaseStringUTFChars(iface,c_iface);
    env->ReleaseStringUTFChars(dns1,c_dns1);
    env->ReleaseStringUTFChars(dns2,c_dns2);
    env->ReleaseStringUTFChars(password,c_password);

    free(cmd);
    free (prop);
    return err;
}

static jint startPppoeNative(JNIEnv* env, jobject clazz)
{
        int attempt;
        int err = 0;
        char* prop = (char *)malloc(PROPERTY_VALUE_MAX);
        
        LOGD("%s", __FUNCTION__);
    
        if (property_set("ctl.start", "pppoe_start") < 0) {
            LOGE("Failed to start pppoe_start");
            err = -1;
            goto end;
        }
        
        for (attempt = 300; attempt > 0;  attempt--) {
            property_get("net.pppoe.status", prop, "");
            if (!strcmp(prop, "connected")) {
                break;
            }
            usleep(100000);  // 100 ms retry delay
        }
        if (attempt == 0) {
            LOGE("%s: Timeout waiting for pppoe-start", __FUNCTION__);
            err = -1;
            goto end;
        }
        err = 0;
end:
    free(prop);
    return err;
}

static jint stopPppoeNative(JNIEnv* env, jobject clazz)
{
    int attempt;
    int err = 0;
    char* prop = (char *)malloc(PROPERTY_VALUE_MAX);
    
    LOGD("%s", __FUNCTION__);
    
    if (property_set("ctl.start", "pppoe_stop") < 0) {
        LOGE("Failed to start pppoe_stop");
        err = -1;
        goto end;
    }
    
    for (attempt = 100; attempt > 0;  attempt--) {
        property_get("net.pppoe.status", prop, "");
        if (!strcmp(prop, "disconnected")) {
            property_set("ctl.stop", "pppoe_stop");
            err = 0;
            goto end;
        }
        usleep(100000);  // 100 ms retry delay
    }
    property_set("ctl.stop", "pppoe_stop");
    err = -1;
end:
    free(prop);
    return err;
}

static jint isPppoeUpNative(JNIEnv* env, jobject clazz)
{
    LOGD("%s", __FUNCTION__);
    return 0;
}
    
/*---------------------------------------------------------------------------*/

/*
 * JNI registration.
 */
static JNINativeMethod gPppoeMethods[] = {
    /* name,                    method descriptor,                              funcPtr */
    { "setupPppoeNative",       "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",    (void *)setupPppoeNative },
    { "startPppoeNative",       "()I",                                          (void *)startPppoeNative },
    { "stopPppoeNative",        "()I",                                          (void *)stopPppoeNative },
    { "isPppoeUpNative",        "()I",                                          (void *)isPppoeUpNative },
};

int register_android_pppoe_PppoeNative(JNIEnv* env)
{
    return AndroidRuntime::registerNativeMethods(env, "com/android/server/pppoe/PppoeNetworkFactory", gPppoeMethods, NELEM(gPppoeMethods) );
}

/* User to register native functions */
extern "C"
jint Java_com_android_server_pppoe_PppoeNetworkFactory_registerNatives(JNIEnv* env, jclass clazz) {
   return AndroidRuntime::registerNativeMethods(env, "com/android/server/pppoe/PppoeNetworkFactory", gPppoeMethods, NELEM(gPppoeMethods) );
}
	
}


