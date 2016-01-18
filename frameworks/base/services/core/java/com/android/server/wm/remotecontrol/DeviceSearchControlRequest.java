package com.android.server.wm.remotecontrol;

import java.nio.charset.Charset;

public class DeviceSearchControlRequest extends RemoteControlRequest {
	private String mDeviceAddress;
	private String mDeviceName;
	
	public DeviceSearchControlRequest(){
		setControlType(TypeConstants.TYPE_DEVICE_DISCOVERY);
	}
	
	public DeviceSearchControlRequest(UDPPacket packet){
		super(packet);
	}
	
	@Override
	protected byte[] encodeData() {
		// TODO Auto-generated method stub
		String msg = mDeviceAddress+"@#@"+mDeviceName;
		byte[] data = msg.getBytes(Charset.defaultCharset());
		return data;
	}
	
	@Override
	protected void decodeData(byte[] data) {
		// TODO Auto-generated method stub
		if (data == null){
			isBadMsg = true;
			return;
		}
		
		String msg = new String(data, Charset.defaultCharset());
		String[] split = msg.split("@#@");
		if (split != null && split.length == 2){
			mDeviceAddress = split[0];
			mDeviceName = split[1];
		}
	}

	public String getDeviceAddress() {
		return mDeviceAddress;
	}

	public void setDeviceAddress(String deviceAddress) {
		this.mDeviceAddress = deviceAddress;
	}

	public String getDeviceName() {
		return mDeviceName;
	}

	public void setDeviceName(String DeviceName) {
		this.mDeviceName = DeviceName;
	}
	
}
