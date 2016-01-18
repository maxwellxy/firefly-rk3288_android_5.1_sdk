/* $_FOR_ROCKCHIP_RBOX_$ */
//$_rbox_$_modify_$_zhengyang_20120220: Rbox android display manager class

/*
 * Copyright (C) 2007 The Android Open Source Project
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

package android.os;

import android.content.Context;
import android.os.IBinder;
import android.os.IDisplayDeviceManagementService;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.util.Log;
import android.view.IWindowManager;
import android.view.Display;
import android.graphics.Rect;

public class DisplayOutputManager {
	private static final String TAG = "DisplayOutputManager";
	private final boolean DBG = true;
	public final int MAIN_DISPLAY = 0;
	public final int AUX_DISPLAY = 1;

	public final int DISPLAY_IFACE_TV = 1;
	public final int DISPLAY_IFACE_YPbPr = 2;
	public final int DISPLAY_IFACE_VGA = 3;
	public final int DISPLAY_IFACE_HDMI = 4;
	public final int DISPLAY_IFACE_LCD = 5;

	private final String DISPLAY_TYPE_TV = "TV";
	private final String DISPLAY_TYPE_YPbPr = "YPbPr";
	private final String DISPLAY_TYPE_VGA = "VGA";
	private final String DISPLAY_TYPE_HDMI = "HDMI";
	private final String DISPLAY_TYPE_LCD = "LCD";

	public final int DISPLAY_OVERSCAN_X = 0;
	public final int DISPLAY_OVERSCAN_Y = 1;
	public final int DISPLAY_OVERSCAN_LEFT = 2;
	public final int DISPLAY_OVERSCAN_RIGHT = 3;
	public final int DISPLAY_OVERSCAN_TOP = 4;
	public final int DISPLAY_OVERSCAN_BOTTOM = 5;
	public final int DISPLAY_OVERSCAN_ALL = 6;

	public final int DISPLAY_3D_NONE = -1;
	public final int DISPLAY_3D_FRAME_PACKING = 0;
	public final int DISPLAY_3D_TOP_BOTTOM = 6;
	public final int DISPLAY_3D_SIDE_BY_SIDE_HALT = 8;


	private int m_main_iface[] = null;
	private int m_aux_iface[] = null;

	private IDisplayDeviceManagementService mService;
	private IWindowManager mWM;

	public DisplayOutputManager() throws RemoteException {
		IBinder b = ServiceManager.getService("display_device_management");
		if(b == null) {
			Log.e(TAG, "Unable to connect to display device management service! - is it running yet?");
			return;
		}
		mWM = IWindowManager.Stub.asInterface(ServiceManager.checkService(
        						Context.WINDOW_SERVICE));
		mService = IDisplayDeviceManagementService.Stub.asInterface(b);
		try {
			// Get main display interface
			String[] display_iface = mService.listInterfaces(MAIN_DISPLAY);
			if(DBG) Log.d(TAG, "main display iface num is " + display_iface.length);
			if(display_iface != null && display_iface.length > 0) {
				m_main_iface = new int[display_iface.length];
				for(int i = 0; i < m_main_iface.length; i++) {
					if(DBG) Log.d(TAG, display_iface[i]);
					m_main_iface[i] = ifacetotype(display_iface[i]);
				}
			}
			else
				m_main_iface = null;
		} catch (Exception e) {
	            Log.e(TAG, "Error listing main interfaces :" + e);
	        }

	        try {
			// Get aux display interface
			String[] display_iface = mService.listInterfaces(AUX_DISPLAY);
			if(DBG) Log.d(TAG, "aux display iface num is " + display_iface.length);
			if(display_iface != null && display_iface.length > 0) {
				m_aux_iface = new int[display_iface.length];
				for(int i = 0; i < m_aux_iface.length; i++) {
					if(DBG) Log.d(TAG, display_iface[i]);
					m_aux_iface[i] = ifacetotype(display_iface[i]);
				}
			}
			else
				m_aux_iface = null;
	        } catch (Exception e) {
	            Log.e(TAG, "Error listing aux interfaces :" + e);
	        }

		if (m_main_iface == null && m_aux_iface == null)
	        	Log.e(TAG, "There is no display interface.");
			//throw new IllegalArgumentException("There is no display interface.");
	}

	private int ifacetotype(String iface) {
		int ifaceType;
		if(iface.equals(DISPLAY_TYPE_TV)) {
			ifaceType = DISPLAY_IFACE_TV;
		} else if(iface.equals(DISPLAY_TYPE_YPbPr)) {
			ifaceType = DISPLAY_IFACE_YPbPr;
		} else if(iface.equals(DISPLAY_TYPE_VGA)) {
			ifaceType = DISPLAY_IFACE_VGA;
		} else if(iface.equals(DISPLAY_TYPE_HDMI)) {
			ifaceType = DISPLAY_IFACE_HDMI;
		} else if(iface.equals(DISPLAY_TYPE_LCD)) {
			ifaceType = DISPLAY_IFACE_LCD;
		} else {
			ifaceType = 0;
		}
		return ifaceType;
	}

	private String typetoface(int type) {
		String iface;

		if(type == DISPLAY_IFACE_TV)
			iface = DISPLAY_TYPE_TV;
		else if(type == DISPLAY_IFACE_YPbPr)
			iface = DISPLAY_TYPE_YPbPr;
		else if(type == DISPLAY_IFACE_VGA)
			iface = DISPLAY_TYPE_VGA;
		else if(type == DISPLAY_IFACE_HDMI)
			iface = DISPLAY_TYPE_HDMI;
		else if(type == DISPLAY_IFACE_LCD)
			iface = DISPLAY_TYPE_LCD;
		else
			return null;
		return iface;
	}

	public int getDisplayNumber() {
		int number = 0;

		if(m_main_iface != null)
			number = 1;
		if(m_aux_iface != null)
			number = 2;
	
		return number;
	}

	public int[] getIfaceList(int display) {
		if(display == MAIN_DISPLAY)
			return m_main_iface;
		else if(display == AUX_DISPLAY)
			return m_aux_iface;
		else
			return null;
	}

	public int getCurrentInterface(int display) {
		try {
			String iface = mService.getCurrentInterface(display);
			return ifacetotype(iface);
		} catch (Exception e) {
			Log.e(TAG, "Error get current Interface :" + e);
			return 0;
		}
	}

	public String[] getModeList(int display, int type) {
		String iface = typetoface(type);
		if(iface.equals(null))
			return null;
		try {
			return mService.getModelist(display, iface);
		} catch (Exception e) {
			Log.e(TAG, "Error get list mode :" + e);
			return null;
		}
	}

	public String getCurrentMode(int display, int type) {
		String iface = typetoface(type);
		if(iface.equals(null))
			return null;

		try {
			return mService.getMode(display, iface);
	        } catch (Exception e) {
			Log.e(TAG, "Error get current mode :" + e);
			return null;
	        }
	}

	public void setInterface(int display, int type, boolean enable ) {
		try {
			String iface = typetoface(type);
			if(iface.equals(null))
				return;
			mService.enableInterface(display, iface, enable);
		} catch (Exception e) {
			Log.e(TAG, "Error set interface :" + e);
			return;
		}
	}

	public void setMode(int display, int type, String mode) {
		String iface = typetoface(type);
		if(iface.equals(null))
			return;

		try {
			mService.setMode(display, iface, mode);
			if(display == MAIN_DISPLAY) {
				int m, n;
				int div = mode.indexOf('x');
	            if (div <= 0 || div >= (mode.length()-1)) {
	            	Log.e(TAG, "Error: bad size " + mode);
	                return;
	            }
	            int div2 = mode.indexOf('p');
	            if (div2 <= 0 || div2 >= (mode.length()-1)) {
	                div2 = mode.indexOf('i');
	                if (div2 <= 0 || div2 >= (mode.length()-1)) {
	                	Log.e(TAG, "Error: bad size " + mode);
	                	return;
	                }
	            }
	            String mstr = mode.substring(0, div);
	            String nstr = mode.substring(div+1, div2);
	            try {
	                m = Integer.parseInt(mstr);
	                n = Integer.parseInt(nstr);
	            } catch (NumberFormatException e) {
	            	Log.e(TAG, "Error: bad number " + e);
	                return;
	            }
	            Log.d(TAG, "set display size " + m +" " + n);
				if(setWMDisplaySize(display, m, n) == 0) {
//					setDisplaySize(MAIN_DISPLAY, m, n);
				}
			}
		} catch (Exception e) {
			Log.e(TAG, "Error set mode :" + e);
			return;
		}
	}

	public boolean getUtils() {
		String enable;

		try {
			enable = mService.getEnableSwitchFB();
		} catch (Exception e) {
			Log.e(TAG, "Error getUtils :" + e);
			return false;
		}
		if(enable.equals("true"))
			return true;
		else
			return false;
	}

	public void switchNextDisplayInterface() {
		try {
			mService.switchNextDisplayInterface(MAIN_DISPLAY);
		} catch (Exception e) {
			Log.e(TAG, "Error set interface :" + e);
			return;
		}
	}

	public void setOverScan(int display, int direction, int value) {
		try {
			mService.setScreenScale(display, direction, value);
//			if(display == MAIN_DISPLAY) {
//				Rect rect = getOverScan(MAIN_DISPLAY);
//				if(direction == DISPLAY_OVERSCAN_X) {
//					rect.left = value;
//					rect.right = value;
//				} else if(direction == DISPLAY_OVERSCAN_Y) {
//					rect.top = value;
//					rect.bottom = value;
//				} else if(direction == DISPLAY_OVERSCAN_LEFT)
//					rect.left = value;
//				else if(direction == DISPLAY_OVERSCAN_RIGHT)
//					rect.right = value;
//				else if(direction == DISPLAY_OVERSCAN_TOP)
//					rect.top = value;
//				else if(direction == DISPLAY_OVERSCAN_BOTTOM)
//					rect.bottom = value;
//				mWM.setOverscan(Display.DEFAULT_DISPLAY, rect.left, rect.top, rect.right, rect.bottom);
//			}
		}catch (Exception e) {
			Log.e(TAG, "Error setScreenScale :" + e);
			return;
		}
	}

	public Rect getOverScan(int display) {
		String OverScan;
		if(display == MAIN_DISPLAY)
			OverScan = SystemProperties.get("persist.sys.overscan.main");
		else if(display == AUX_DISPLAY)
			OverScan = SystemProperties.get("persist.sys.overscan.aux");
		else {
			return new Rect(100,100,100,100);
		}

		String split = OverScan.substring(9);
		String [] value = split.split("\\,");
		if (value != null) {
			Rect rect = new Rect();
			rect.left = Integer.parseInt(value[0]);
			rect.top = Integer.parseInt(value[1]);
			rect.right = Integer.parseInt(value[2]);
			rect.bottom = Integer.parseInt(value[3]);
			return rect;
		}
		return new Rect(100,100,100,100);
	}

	public void setDisplaySize(int display, int width, int height)
	{
		String displaypolicy = SystemProperties.get("persist.sys.display.policy", "manual");
		if(displaypolicy.equals("auto") == true) {
			if (width >= 0 && height >= 0) {
				try {
					Log.d(TAG, "setDisplaySize " + display + " " + width + " " + height);
	        			mService.setDisplaySize(display, width, height);
				} catch (RemoteException e) {
					Log.e(TAG, "Error setFramebufferSize :" + e);
				}
    			}
    		}
	}

	private int setWMDisplaySize(int display, int width, int height)
	{

		String displaypolicy = SystemProperties.get("persist.sys.display.policy", "manual");

		if(displaypolicy.equals("manual") == true)
			return -1;

		if (mWM == null) {
			 Log.e(TAG, "Error setDisplaySize get widow manager");
			 return -1;
		}
		try {
		    if (width >= 0 && height >= 0) {
		        mWM.setForcedDisplaySize(display, width, height);
		        return 0;
		    } else {
		        mWM.clearForcedDisplaySize(display);
		        return -1;
		    }
		} catch (RemoteException e) {
			Log.e(TAG, "Error setDisplaySize :" + e);
		}
		return 0;
	}

	public int get3DModes(int display, int type)
	{
		String iface = typetoface(type);
		if(iface.equals(null))
			return 0;

		try {
			return mService.get3DModes(display, iface);
		} catch (Exception e) {
			Log.e(TAG, "Error get 3d modes :" + e);
			return 0;
		}
	}

	public int getCur3DMode(int display, int type)
	{
		String iface = typetoface(type);
		if(iface.equals(null))
			return -1;

		try {
			return mService.getCur3DMode(display, iface);
		} catch (Exception e) {
			Log.e(TAG, "Error get cur 3d mode :" + e);
			return -1;
		}
	}

	public void set3DMode(int display, int type, int mode)
	{
		String iface = typetoface(type);
		if(iface.equals(null))
			return;

		try {
			mService.set3DMode(display, iface, mode);
		} catch (Exception e) {
			Log.e(TAG, "Error set 3d modes :" + e);
			return;
		}
	}

	public int saveConfig()
	{
		try {
			return mService.saveConfig();
		} catch (Exception e) {
			Log.e(TAG, "Error save :" + e);
			return -1;
		}
	}

	/*
	 * brightness: [-128, 127], default 0
	 */
	public int setBrightness(int display, int brightness)
	{
		if (brightness < -32 || brightness > 31) {
			Log.e(TAG, "setBrightness out of range " + brightness);
			return -1;
		}
		try {
			mService.setBrightness(display, brightness);
		} catch (Exception e) {
			Log.e(TAG, "Error set brightness :" + e);
			return -1;
		}
		return 0;
	}

	/*
	 * contrast: [0, 1.992], default 1;
	 */
	public int setContrast(int display, float contrast)
	{
		if (contrast < 0 || contrast > 1.992) {
			Log.e(TAG, "setContrast out of range " + contrast);
			return -1;
		}
		try {
			mService.setContrast(display, contrast);
		} catch (Exception e) {
			Log.e(TAG, "Error set Contrast :" + e);
			return -1;
		}
		return 0;
	}

	/*
	 * saturation: [0, 1.992], default 1;
	 */
	public int setSaturation(int display, float saturation)
	{	
		if (saturation < 0 || saturation > 1.992) {
			Log.e(TAG, "setContrast out of range " + saturation);
			return -1;
		}
		try {
			mService.setSaturation(display, saturation);
		} catch (Exception e) {
			Log.e(TAG, "Error set sat_con :" + e);
			return -1;
		}
		return 0;
	}

	/*
	 * degree: [-30, 30], default 0
	 */
	public int setHue(int display, float degree)
	{
		if (degree < -30 || degree > 30) {
			Log.e(TAG, "Error set hue out of range " + degree);
			return -1;
		}
		try {
			mService.setHue(display, degree);
		} catch (Exception e) {
			Log.e(TAG, "Error set hue :" + e);
			return -1;
		}
		return 0;
	}

}
