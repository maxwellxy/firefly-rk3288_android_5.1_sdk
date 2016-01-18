/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    TypeContants.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-11-17 下午06:07:18  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-11-17      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

public class TypeConstants {
	
	//SENSOR COMMAND
	public static final int TYPE_SENSOR_COMMAND = 0x0100;
	//SENSOR
	public static final int TYPE_SENSOR = 0x0101;
	//ENABLE GSENSOR
	public static final int TYPE_GSENSOR_ENABLED = 0x0102;
	//DISABLE GSENSOR
	public static final int TYPE_GSENSOR_DISABLED = 0x0103;
	//ENABLE GYROSCOPE
	public static final int TYPE_GYROSCOPE_ENABLED = 0x0104;
	//DISABLE GYROSCOPE
	public static final int TYPE_GYROSCOPE_DISABLED = 0x0105;

	//MOUSE COMMAND
	public static final int TYPE_MOUSE_COMMAND = 0x0200;
	//MOUSE
	public static final int TYPE_MOUSE = 0x0201;
	
	//SOFTKEY COMMAND
	public static final int TYPE_SOFTKEY_COMMAND = 0x0300;
	//SOFTKEY
	public static final int TYPE_SOFTKEY = 0x0301;
	
	//SCROLL COMMAND
	public static final int TYPE_SCROLL_COMMAND = 0x0400;
	//SCROLL
	public static final int TYPE_SCROLL = 0x0401;
	
	//WIMO COMMAND
	public static final int TYPE_WIMO_COMMAND = 0x0500;
	public static final int TYPE_WIMO = 0x0501;
	
	//DEVICE CHECK COMMAND
	public static final int TYPE_DEVICE_CHECK_COMMAND = 0x0600;
	public static final int TYPE_DEVICE_CHECK = 0x0601;
	
	//DEVICE DISCOVERY COMMAND
	public static final int TYPE_DEVICE_DISCOVERY_COMMAND = 0x700;
	public static final int TYPE_DEVICE_DISCOVERY = 0x701;
	
	//JOYSTICK COMMAND
	public static final int TYPE_JOYSTICK_COMMAND = 0x800;
	public static final int TYPE_JOYSTICK = 0x801;

	//TEXT COMMAND 
	public static final int TYPE_TEXT_COMMAND = 0x900;
	public static final int TYPE_TEXT = 0x901;
}
