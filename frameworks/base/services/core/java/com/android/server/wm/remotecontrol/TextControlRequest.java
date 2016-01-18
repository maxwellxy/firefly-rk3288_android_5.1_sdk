package com.android.server.wm.remotecontrol;

import java.nio.charset.Charset;

public class TextControlRequest extends RemoteControlRequest {
	private String mText;
	
	public TextControlRequest() {
		// TODO Auto-generated constructor stub
		setControlType(TypeConstants.TYPE_TEXT);
	}
	
	public TextControlRequest(UDPPacket packet){
		super(packet);
	}
	
	@Override
	protected byte[] encodeData() {
		// TODO Auto-generated method stub
		byte[] data = mText != null ? mText.getBytes(Charset.defaultCharset()) : new byte[0];
		
		return data;
	}
	
	@Override
	protected void decodeData(byte[] data) {
		// TODO Auto-generated method stub
		mText = new String(data, Charset.defaultCharset());
	}
	
	public String getText(){
		return mText;
	}
	
	public void setText(String text){
		mText = text;
	}
}
