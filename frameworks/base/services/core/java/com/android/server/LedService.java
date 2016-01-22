package com.android.server;

import android.os.ILedService;

/**************************************************************************
 * File name   : LedService
 * Description : for firefly-rk3288
 * Author      : qiyei2015(1273482124@qq.com) in chengdu
 * <p/>
 * Version       Date       Author         modefy
 * 1.0           16-1-18    qiyei2015      create
 **************************************************************************/
public class LedService extends ILedService.Stub{

    private static final String TAG = "VibratorService";

    /*call native c function to access hardware*/

    public LedService(){

       native_ledOpen();

    }

    public int ledCtrl(int which,int status){

        return native_ledCtrl(which,status);
    }

    /*native function,jni call c function*/
    public static native int native_ledCtrl(int which,int status);
    public static native int native_ledOpen();
    public static native void native_ledClose();

}

