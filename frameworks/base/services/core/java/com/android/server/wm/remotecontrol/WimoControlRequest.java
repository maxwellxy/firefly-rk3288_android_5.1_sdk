package com.android.server.wm.remotecontrol;

import android.util.Log;

public class WimoControlRequest extends RemoteControlRequest {
	private boolean DEBUG = false;
	private void LOG(String msg){
		if (DEBUG)
		Log.d("WimoControlRequest",msg);
	}
	public static final String URL_SCHEME = "udpwimo";
	public static final int UNKNOWSTATE = -1;
	public static final int CONNECTING = 0;
	public static final int CONNECTED = 1;
	public static final int DISCONNECT = 2;
	public static final int HEARTALIVE = 3;
	public static final int HEARTSTOP = 4;
	
	private int State;
	private int RemotePort;
	private int LocalPort;
	private int versionCode;
	
	public WimoControlRequest(){
		setControlType(TypeConstants.TYPE_WIMO);
	}
	
	public WimoControlRequest(UDPPacket packet){
		super(packet);
		setRequestHost(packet.getRemoteAddress());
	}
	
	@Override
	protected byte[] encodeData() {
		// TODO Auto-generated method stub
		byte[] data;
		switch (State) {
		case CONNECTING:		
		case DISCONNECT:
		case HEARTALIVE:
		case HEARTSTOP:
			data = new byte[1];
			data[0] = (byte)State;
			return data;
		
		case CONNECTED:
			data = new byte[10];
			data[0] = (byte)State;
			byte[] tmp = DataTypesConvert.changeIntToByte(LocalPort, 4);
			fillData(data, tmp, 1, 4);
			tmp = DataTypesConvert.changeIntToByte(RemotePort, 4);
			fillData(data, tmp, 5, 8);
			data[9] = (byte)versionCode;
			return data;
			
		default:
			break;
		}
		
		return super.encodeData();
	}
	
	@Override
	protected void decodeData(byte[] data) {
		// TODO Auto-generated method stub
		State = data[0];
		
		switch (State) {
		case CONNECTED:
			byte[] tmp = fetchData(data, 1, 4);
			LocalPort = DataTypesConvert.changeByteToInt(tmp);
			tmp = fetchData(data, 5, 8);
			RemotePort = DataTypesConvert.changeByteToInt(tmp);
			versionCode = data[9]>>4;
			break;
			
		case CONNECTING:
		case DISCONNECT:
		case HEARTALIVE:
		case HEARTSTOP:
			break;

		default:
			break;
		}
	}

	public int getState() {
		return State;
	}

	public void setState(int state) {
		State = state;
	}

	public int getRemotePort() {
		return RemotePort;
	}

	public void setRemotePort(int remotePort) {
		RemotePort = remotePort;
	}

	public int getLocalPort() {
		return LocalPort;
	}

	public void setLocalPort(int localPort) {
		LocalPort = localPort;
	}

	public int getVersionCode() {
		return versionCode;
	}

	public void setVersionCode(int versionCode) {
		this.versionCode = versionCode;
	}
	
	
}
