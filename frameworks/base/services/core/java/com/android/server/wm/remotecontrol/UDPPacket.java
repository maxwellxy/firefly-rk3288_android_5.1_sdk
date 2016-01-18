/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    UDPPacket.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-19 11:34:01  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-19      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import java.net.DatagramPacket;
import java.net.InetAddress;

public class UDPPacket {

	private DatagramPacket dgmPacket = null;
	private String localAddr = "";
	private long timeStamp;
	private byte[] packetBytes = null;
	
	protected UDPPacket(){
	}

	public UDPPacket(byte[] buf, int length) {
		dgmPacket = new DatagramPacket(buf, length);
	}
	
	public void setPacket(UDPPacket packet){
		this.dgmPacket = packet.getDatagramPacket();
		this.localAddr = packet.getLocalAddress();
		this.timeStamp = packet.getTimeStamp();
		this.packetBytes = packet.getData();
	}
	
	public DatagramPacket getDatagramPacket() {
		return dgmPacket;
	}
	
	public byte[] getData() {
		if (packetBytes != null)
			return packetBytes;
		DatagramPacket packet = getDatagramPacket();
		int packetLen = packet.getLength();
		packetBytes = new byte[packetLen];
		System.arraycopy(packet.getData(), 0, packetBytes, 0, packetLen);
		return packetBytes;
	}
	
	
	public void setLocalAddress(String addr) {
		localAddr = addr;
	}
	
	public String getLocalAddress() {
		return localAddr;
	}
	
	public void setTimeStamp(long value) {
		timeStamp = value;
	}
		
	public long getTimeStamp() {
		return timeStamp;
	}
	
	public InetAddress getRemoteInetAddress() {
		return getDatagramPacket().getAddress();
	}
	
	public String getRemoteAddress() {
		return getDatagramPacket().getAddress().getHostAddress();
	}

	public int getRemotePort() {
		return getDatagramPacket().getPort();
	}
	
	public String toString() {
		return new String(getData());
	}
}
