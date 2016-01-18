/* $_FOR_ROCKCHIP_RBOX_$ */
//$_rbox_$_modify_$_zhengyang_20120220: Rbox android display management service

/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server;


import android.content.Context;
import android.content.Intent;
import android.os.IDisplayDeviceManagementService;
import android.os.SystemProperties;
import android.util.Slog;
import android.util.Log;

/**
 * @hide
 */

class DisplayDeviceManagementService extends IDisplayDeviceManagementService.Stub {
	private static final String TAG = "DisplayDeviceManagementService";
    private static final boolean DBG = true;
    private static final String DISPLAYD_TAG = "DisplaydConnector";

    class DisplaydResponseCode {
    	public static final int InterfaceListResult		= 110;
    	public static final int ModeListResult			= 111;
    	public static final int CommandOkay				= 200;
    	public static final int OperationFailed         = 400;
        public static final int InterfaceChange			= 600;
    }

    /**
     * Binder context for this service
     */
    private Context mContext;

    /**
     * connector object for communicating with netd
     */
    private NativeDaemonConnector mConnector;


    /**
     * Constructs a new NetworkManagementService instance
     *
     * @param context  Binder context for this service
     */
    public DisplayDeviceManagementService(Context context) {
        mContext = context;

        if ("simulator".equals(SystemProperties.get("ro.product.device"))) {
            return;
        }

        if ("unknown".equals(SystemProperties.get("ro.target.product", "unknown"))) {
            return;
        }

        mConnector = new NativeDaemonConnector(
                new DisplaydCallbackReceiver(), "displayd", 10, DISPLAYD_TAG, 160, null);
        Thread thread = new Thread(mConnector, DISPLAYD_TAG);
        thread.start();
        if(DBG) Slog.d(TAG, "DisplayDeviceManagementService start");
    }

    /**
     * Notify our observers of an interface addition.
     */
    private void notifyInterfaceAdded(String iface) {
    	if(DBG) Slog.d(TAG, iface + " insert");
    	Intent in = null;
    	in = new Intent("android.display.action.insert");
        mContext.sendBroadcast(in);
    }

    /**
     * Notify our observers of an interface removal.
     */
    private void notifyInterfaceRemoved(String iface) {
    	if(DBG) Slog.d(TAG, iface + " remove");
    	Intent in = null;
    	in = new Intent("android.display.action.remove");
        mContext.sendBroadcast(in);
    }

    /**
     * Notify our observers of an interface link status change
     */
    private void notifyInterfaceLinkStatusChanged(String iface) {
    	if(DBG) Slog.d(TAG, iface + " changed");
    	Intent in = null;
    	in = new Intent("android.display.action.change");
        mContext.sendBroadcast(in);
    }

    //
    // Displayd Callback handling
    //

    class DisplaydCallbackReceiver implements INativeDaemonConnectorCallbacks {
        public void onDaemonConnected() {
 //           NetworkManagementService.this.onConnected();
            new Thread() {
                public void run() {
                }
            }.start();
        }
        public boolean onCheckHoldWakeLock(int code) {
            return false;
        }
        public boolean onEvent(int code, String raw, String[] cooked) {
            if (code == DisplaydResponseCode.InterfaceChange) {
                /*
                 * a network interface change occured
                 * Format: "NNN Iface added <name>"
                 *         "NNN Iface removed <name>"
                 *         "NNN Iface changed <name> <up/down>"
                 */
                if (cooked.length < 4 || !cooked[1].equals("Iface")) {
                    throw new IllegalStateException(
                            String.format("Invalid event from daemon (%s)", raw));
                }
                if (cooked[2].equals("added")) {
                    notifyInterfaceAdded(cooked[3]);
                    return true;
                } else if (cooked[2].equals("removed")) {
                    notifyInterfaceRemoved(cooked[3]);
                    return true;
                } else if (cooked[2].equals("changed")) {
                    notifyInterfaceLinkStatusChanged(cooked[3]);
                    return true;
                }
                throw new IllegalStateException(
                        String.format("Invalid event from daemon (%s)", raw));
            }
            return false;
        }
    }

    //
    // IDisplayManagementService members
    //

    public String[] listInterfaces(int display) throws IllegalStateException {
//        mContext.enforceCallingOrSelfPermission(
//                android.Manifest.permission.ACCESS_NETWORK_STATE, "DisplayDeviceManagementService");
 		try {
            return NativeDaemonEvent.filterMessageList(
                    mConnector.executeForList("interface", "list", display), DisplaydResponseCode.InterfaceListResult);
        } catch (NativeDaemonConnectorException e) {
           // throw e.rethrowAsParcelableException();
            return null;
        }

    }

    public String getCurrentInterface(int display) throws IllegalStateException {
    	final NativeDaemonEvent event;
        try {
            event = mConnector.execute("interface", "getcur", display);
        } catch (NativeDaemonConnectorException e) {
            throw e.rethrowAsParcelableException();
        }
        event.checkCode(DisplaydResponseCode.CommandOkay);
        return event.getMessage();
    }

    public String[] getModelist(int display, String iface) throws IllegalStateException {
    	try {
            return NativeDaemonEvent.filterMessageList(
                    mConnector.executeForList("mode", "list", display, iface), DisplaydResponseCode.ModeListResult);
        } catch (NativeDaemonConnectorException e) {
           // throw e.rethrowAsParcelableException();
            Log.e(TAG, "Error no mode list :" + e);
            return null;
        }
    }

    public String getMode(int display, String iface) throws IllegalStateException {
    	final NativeDaemonEvent event;
        try {
            event = mConnector.execute("mode", "get", display, iface);
        } catch (NativeDaemonConnectorException e) {
            throw e.rethrowAsParcelableException();
        }
        event.checkCode(DisplaydResponseCode.CommandOkay);
        return event.getMessage();
    }

    public void enableInterface(int display, String iface, boolean enable) throws IllegalStateException {
    	try{
    		String cmd = String.format("interface set %d %s %sable", display, iface, (enable ? "en" : "dis"));
    		if(DBG) Slog.d(TAG, "send cmd " + cmd);
    		mConnector.execute("interface", "set", display, iface, enable);
    	} catch (NativeDaemonConnectorException e) {
            throw new IllegalStateException(
            "Cannot communicate with native daemon to enable interface");
    	}
    }

    public void setMode(int display, String iface, String mode) throws IllegalStateException {
    	try{
    		String cmd = String.format("mode set %d %s %s", display, iface, mode);
    		if(DBG) Slog.d(TAG, "send cmd " + cmd);
    		mConnector.execute("mode", "set", display, iface, mode);
    	} catch (NativeDaemonConnectorException e) {
            throw new IllegalStateException(
            "Cannot communicate with native daemon to set mode");
    	}
    }

    public void switchNextDisplayInterface(int display) throws IllegalStateException {
    	try{
    		String cmd = String.format("interface switch %d", display);
    		if(DBG) Slog.d(TAG, "send cmd " + cmd);
    		mConnector.execute("interface", "switch", display);
    	} catch (NativeDaemonConnectorException e) {
            throw new IllegalStateException(
            "Cannot communicate with native daemon to set mode");
    	}
    }

	public String getEnableSwitchFB() throws IllegalStateException {
		final NativeDaemonEvent event;
        try {
            event = mConnector.execute("utils", "switch");
        } catch (NativeDaemonConnectorException e) {
            throw e.rethrowAsParcelableException();
        }
        event.checkCode(DisplaydResponseCode.CommandOkay);
        return event.getMessage();
	}

	public void setScreenScale(int display, int direction, int value) throws IllegalStateException {
		try{
    		String cmd = String.format("utils scaleset %d %d %d", display, direction, value);
    		if(DBG) Slog.d(TAG, "send cmd " + cmd);
    		mConnector.execute("utils", "scaleset", display, direction, value);
    	} catch (NativeDaemonConnectorException e) {
            throw new IllegalStateException(
            "Cannot communicate with native daemon to set mode");
    	}
	}

	public void setDisplaySize(int display, int width, int height) throws IllegalStateException {
		try{
    		String cmd = String.format("utils switchfb %d %dx%d", display, width, height);
    		if(DBG) Slog.d(TAG, "send cmd " + cmd);
    		mConnector.execute("utils", "switchfb", display, width, height);
    	} catch (NativeDaemonConnectorException e) {
            throw new IllegalStateException(
            "Cannot communicate with native daemon to switch framebuffer");
    	}
	}

	public int get3DModes(int display, String iface) throws IllegalStateException {
		final NativeDaemonEvent event;
        try {
            event = mConnector.execute("mode", "get3dmodes", display, iface);
        } catch (NativeDaemonConnectorException e) {
            throw e.rethrowAsParcelableException();
        }
        event.checkCode(DisplaydResponseCode.CommandOkay);
        return Integer.parseInt(event.getMessage());
	}

	public int getCur3DMode(int display, String iface) throws IllegalStateException {
		final NativeDaemonEvent event;
        try {
            event = mConnector.execute("mode", "get3dmode", display, iface);
        } catch (NativeDaemonConnectorException e) {
            throw e.rethrowAsParcelableException();
        }
        event.checkCode(DisplaydResponseCode.CommandOkay);
        return Integer.parseInt(event.getMessage());
	}

	public void set3DMode(int display, String iface, int mode) throws IllegalStateException {
		try{
			String cmd = String.format("mode set3dmode %d %s %d", display, iface, mode);
			if(DBG) Slog.d(TAG, "send cmd " + cmd);
			mConnector.execute("mode", "set3dmode", display, iface, mode);
		} catch (NativeDaemonConnectorException e) {
			throw new IllegalStateException(
			"Cannot communicate with native daemon to set mode");
		}
	}

	public void setBrightness(int display, int brightness) throws IllegalStateException {
		try{
			String cmd = String.format("utils brightness %d %d ", display, brightness);
			if(DBG) Slog.d(TAG, "send cmd " + cmd);
			mConnector.execute("utils", "brightness", display, brightness);
		} catch (NativeDaemonConnectorException e) {
			throw new IllegalStateException(
			"Cannot communicate with native daemon to set brightness");
		}
	}

	public void setContrast(int display, float contrast) throws IllegalStateException {
		try{
			String cmd = String.format("utils contrast %d %f ", display, contrast);
			if(DBG) Slog.d(TAG, "send cmd " + cmd);
			mConnector.execute("utils", "contrast", display, contrast);
		} catch (NativeDaemonConnectorException e) {
			throw new IllegalStateException(
			"Cannot communicate with native daemon to set contrast");
		}
	}

	public void setSaturation(int display, float saturation) throws IllegalStateException {
		try{
			String cmd = String.format("utils sat_con %d %f ", display, saturation);
			if(DBG) Slog.d(TAG, "send cmd " + cmd);
			mConnector.execute("utils", "saturation", display, saturation);
		} catch (NativeDaemonConnectorException e) {
			throw new IllegalStateException(
			"Cannot communicate with native daemon to set sat_con");
		}
	}

	public void setHue(int display, float degree) throws IllegalStateException {
		try{
			String cmd = String.format("utils hue %d %f", display, degree);
			if(DBG) Slog.d(TAG, "send cmd " + cmd);
			mConnector.execute("utils", "hue", display, degree);
		} catch (NativeDaemonConnectorException e) {
			throw new IllegalStateException(
			"Cannot communicate with native daemon to set hue");
		}
	}

	public int saveConfig() throws IllegalStateException {
		final NativeDaemonEvent event;
		try {
			event = mConnector.execute("utils", "save");
		} catch (NativeDaemonConnectorException e) {
			throw e.rethrowAsParcelableException();
		}
		event.checkCode(DisplaydResponseCode.CommandOkay);
		return Integer.parseInt(event.getMessage());
	}
}




