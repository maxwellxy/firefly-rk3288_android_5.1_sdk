/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    RemoteControlRequest.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-19 10:13:55  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-19      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import static com.android.server.wm.remotecontrol.DataTypesConvert.changeIntToByte;
import static com.android.server.wm.remotecontrol.DataTypesConvert.changeByteToInt;
import android.util.Log;

//import com.android.rockchip.common.core.http.HTTP;
//import com.android.rockchip.common.core.upnp.Device;

public class RemoteControlRequest {
	private static final String TAG = "RemoteControlRequest";
	private static final boolean DEBUG = true;
	private void LOG(String msg){
		if (DEBUG) {
			Log.d(TAG,msg);
		}
	}

	public static final int REMOTE_CONTROL_PORT = 56456;
	public static final int SOFTKEY_REMOTE_CONTROL_PORT = REMOTE_CONTROL_PORT + 1;
	public static final int GSENSOR_REMOTE_CONTROL_PORT = REMOTE_CONTROL_PORT + 2;
	public static final int CONNECT_STATE_PORT = REMOTE_CONTROL_PORT + 3;
	public static final int MIN_MSG_LEN = 4;

	//CONTROL-TYPE -- 2bytes
	private int controlType;
	//data length -- 2bytes
	private int length;
	//DATA
	private byte[] data;
	private UDPSocket uSocket;
	private String requestHost = "";
	private int requestPort = -1;
	
	protected boolean isBadMsg = false;
	
	public RemoteControlRequest(){
	}
	
	public RemoteControlRequest(byte[] msg){
		decodeMessage(msg);
	}
	
	public RemoteControlRequest(UDPPacket packet){
		decodeMessage(packet.getData());
	}
	
	protected byte[] encodeData(){
		return new byte[0];
	}
	
	protected void decodeData(byte[] data){
	}
	
	/**
	 * send request
	 */
	public boolean post(int Port){
		if(uSocket==null){
			uSocket = new UDPSocket();
		}
		if(Port > 0){
			//send to the default specified port
			requestPort = Port;
		}
		return uSocket.post(requestHost, requestPort, encodeMessage());
	}
	
	public boolean close(){
		if(uSocket!=null){
			uSocket.close();
		}
		return true;
	}
	
//	public void setRequestHost(Device device){
//		String postURL = device.getRootDevice().getURLBase();
//		if (postURL == null || postURL.length() <= 0)
//			postURL = device.getRootDevice().getLocation();
//		
//		String reqHost = HTTP.getHost(postURL);
//		int reqPort = HTTP.getPort(postURL);
//		setRequestHost(reqHost);
//		setRequestPort(reqPort);
//	}
	
	/**
	 * encodeMessage
	 * @return
	 */
	public byte[] encodeMessage() {
		int len = MIN_MSG_LEN;
		data = encodeData();
		length = data.length;
		if(data!=null)
			len += data.length;
		byte[] msg = new byte[len];
		//controlType 2
		byte[] tmp = changeIntToByte(controlType, 2);
		msg[0] = tmp[0];
		msg[1] = tmp[1];
		//Length 2
		tmp = changeIntToByte(length, 2);
		msg[2] = tmp[0];
		msg[3] = tmp[1];
		//DATA
		fillData(msg, data, 4, len-1);
		return msg;
	}
	
	/**
	 * decodeMessage
	 * @param msg
	 */
	public void decodeMessage(byte[] msg){
		isBadMsg = false;
		if(msg==null||msg.length<MIN_MSG_LEN){
			LOG("Bad message. Message is null or too short");
			isBadMsg = true;
			return;
		}
		length = changeByteToInt(msg, 2, 3);
		if(length + MIN_MSG_LEN != msg.length){
			LOG("Bad message. Message length error. ");
			isBadMsg = true;
			return;
		}
		//controlType 2
		controlType = changeByteToInt(msg, 0, 1);
		//DATA
		data = fetchData(msg, 4, msg.length-1);
		if(data==null || data.length != length){
//			LOG("data error");
			isBadMsg = true;
			return;
		}
		decodeData(data);
	}
	
	public void fillData(byte[] msg, byte[] data, int start, int end){
		if(data==null||start>end) return;
		for(int i=start,j=0; i<=end&&j<data.length; i++,j++){
			msg[i] = data[j];
		}
	}

	public byte[] fetchData(byte[] msg, int start, int end){
		if(start>end) return null;
		byte[] tData = new byte[end-start+1];
		for(int i=start,j=0; i<=end; i++,j++){
			tData[j] = msg[i];
		}
		return tData;
	}
	
	public int getControlType() {
		return controlType;
	}
	
	public int getCommandType() {
		return controlType&0xFF00;
	}

	public void setControlType(int controlType) {
		this.controlType = controlType;
	}

	public int getLength() {
		if(data==null) return 0;
		return data.length;
	}
	/**
	 * @hide
	 */
	public void setLength(int length) {
		this.length = length;
	}
	public String getStringData() {
		if(data==null) return null;
		return new String(data);
	}
	public byte[] getData() {
		return data;
	}
	
	public void setRequestHost(String host)
	{
		requestHost = host;
	}
	public String getRequestHost()
	{
		return requestHost;
	}
	public void setRequestPort(int host)
	{
		requestPort = host;
	}
	public int getRequestPort()
	{
		return requestPort;
	}
}
