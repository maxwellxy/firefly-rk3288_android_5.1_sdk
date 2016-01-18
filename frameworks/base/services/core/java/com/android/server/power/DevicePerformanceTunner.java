package com.android.server.power;

import android.content.Context;
import android.os.PowerManager;


public final class DevicePerformanceTunner {
    private final static String TAG = DevicePerformanceTunner.class.getSimpleName();

    private static PowerManager mPowerManager;
	private static DevicePerformanceTunner mDevicePerformanceTunner = null;

	public static DevicePerformanceTunner getInstance(Context context) {
        if(mDevicePerformanceTunner == null)
            mDevicePerformanceTunner = new DevicePerformanceTunner(context);
        return mDevicePerformanceTunner;
	}

	private DevicePerformanceTunner(Context context) {
	    mPowerManager = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
	}

    public void setPerformanceMode(int mode) {
        if (mPowerManager  != null) {
            mPowerManager.setPerformanceMode(mode);
        }
    }

}
