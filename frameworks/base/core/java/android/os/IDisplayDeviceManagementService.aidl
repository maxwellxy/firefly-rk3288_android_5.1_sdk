/* $_FOR_ROCKCHIP_RBOX_$ */
//$_rbox_$_modify_$_zhengyang_20120220: AIDL file for rbox android display management service.

/* //device/java/android/android/os/IDisplayDeviceManagementService.aidl
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

package android.os;


/**
 * @hide
 */
interface IDisplayDeviceManagementService
{
    /**
     ** GENERAL
     **/

     /**
     * Returns a list of currently known display interfaces
     */
    String[] listInterfaces(int display);
    
    /**
     * Returns the current enabled display interfaces
     */
    String getCurrentInterface(int display);
    
    /**
     * Returns a list of supported display modes
     */
    String[] getModelist(int display, String iface);
    
    /**
     * Returns current display mode
     */
    String getMode(int display, String iface);
    
    /**
     * Enable selected display interface
     */
    void enableInterface(int display, String iface, boolean enable);
    
    /**
     * Set display mode
     */
    void setMode(int display, String iface, String mode);

	/**
     * Switch next display interface
     */
    void switchNextDisplayInterface(int display);
    
	/**
     * Get siwtch framebuffer info
     */
	String getEnableSwitchFB();
	
	/**
     * Set display screen scale value
     */
    void setScreenScale(int display, int direction, int value);
    
    /**
     * Switch framebuffer
     */
    void setDisplaySize(int display, int width, int height);
    
    /**
     * Get Supported 3D Modes
     */
    int get3DModes(int display, String iface);
    
    /**
     * Get Current 3D Mode
     */
    int getCur3DMode(int display, String iface);
    
    /**
     * Set 3D Mode
     */
    void set3DMode(int display, String iface, int mode);
    /**
     * saveConfig
     */
    int saveConfig();
    
    /**
     * Set brightness
     */
    void setBrightness(int display, int brightness);
    /**
     * Set contrast
     */
    void setContrast(int display, float contrast);
    /**
     *Set saturation
     */
    void setSaturation(int display, float saturation);
    /**
      *Set Hue
    */
    void setHue(int display, float degree);
}
