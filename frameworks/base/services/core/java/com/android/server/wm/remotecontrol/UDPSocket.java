/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    UDPSocket.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-11-17 11:34:01  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-11-17      xwf         1.0         create
*******************************************************************/   
package com.android.server.wm.remotecontrol;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketException;

import android.util.Log;


public class UDPSocket {

	private DatagramSocket udpSock = null;
	private String localAddr = "";
	
	public UDPSocket() {
		open();
	}
	
	public UDPSocket(String bindAddr, int bindPort) {
		open(bindAddr, bindPort);
	}

	public UDPSocket(int bindPort) {
		open(bindPort);
	}

	protected void finalize() {
		close();
	}

	public void setLocalAddress(String addr) {
		localAddr = addr;
	}
	
	public DatagramSocket getDatagramSocket() {
		return udpSock;
	}

	public DatagramSocket getUDPSocket(){
		return udpSock;
	}	
	
	public String getLocalAddress() {
		if (localAddr!=null&&0 < localAddr.length())
			return localAddr;
		if(udpSock!=null&&udpSock.getLocalAddress()!=null){
			return udpSock.getLocalAddress().getHostAddress();
		}else{
			return "";
		}
	}

	public boolean open() {
		close();
		try {
			udpSock = new DatagramSocket();
		}
		catch (Exception e) {
			Log.e("UDPSocket","USocket Open Error:"+e);
			return false;
		}
		return true;
	}
	
	/**
	 * open sockect with specified addr and port 
	 * @param bindAddr
	 * @param bindPort
	 * @return
	 */
	public boolean open(String bindAddr, int bindPort) {
		close();
		try {
//			udpSock = new DatagramSocket(bindPort);
//			udpSock.setReuseAddress(true);
			InetSocketAddress bindSock = new InetSocketAddress(bindPort);
			udpSock = new DatagramSocket(null);
			udpSock.setReuseAddress(true);
			udpSock.bind(bindSock);
		}catch (Exception e) {
			Log.e("UDPSocket","USocket Open Error:"+e);
			return false;
		}
		setLocalAddress(bindAddr);
		return true;
	}
	
	/**
	 * open sockect with specified port
	 * @param bindPort
	 * @return
	 */
	public boolean open(int bindPort) {
		close();
		try {
			InetSocketAddress bindSock = new InetSocketAddress(bindPort);
			udpSock = new DatagramSocket(null);
			udpSock.setReuseAddress(true);
			udpSock.bind(bindSock);
		}catch (Exception e) {
			Log.e("UDPSocket","USocket Open Error:"+e);
			return false;
		}
		return true;
	}
		
	/**
	 * close sockect
	 * @return
	 */
	public boolean close() {
		if (udpSock == null)
			return true;
		try {
			udpSock.close();
			udpSock = null;
		}
		catch (Exception e) {
			Log.e("UDPSocket", "USocket Close Error:"+e);
			return false;
		}
		
		return true;
	}

	/**
	 * send data package
	 * @param addr
	 * @param port
	 * @param msg
	 * @return
	 */
	public boolean post(String addr, int port, String msg) {
		return post(addr, port, msg.getBytes());
	}
	public boolean post(String addr, int port, byte[] data) {
		 try {
			 
			InetAddress inetAddr = InetAddress.getByName(addr);
			DatagramPacket dgmPacket = new DatagramPacket(data, data.length, inetAddr, port);
			udpSock.send(dgmPacket);
		}
		catch (Exception e) {
			Log.e("UDPSocket", "USocket Post Data Error:"+e);
			return false;
		}
		return true;
	}

	/**
	 * receive data
	 * @return
	 */
	public UDPPacket receive() {
		byte ssdvRecvBuf[] = new byte[512];
		UDPPacket recvPacket = new UDPPacket(ssdvRecvBuf, ssdvRecvBuf.length);
		recvPacket.setLocalAddress(getLocalAddress());
		try {
	 		udpSock.receive(recvPacket.getDatagramPacket());
			recvPacket.setTimeStamp(System.currentTimeMillis());
		}catch(SocketException e){
			//ignore
			return null;
		}catch (Exception e) {
			Log.e("UDPSocket","USocket Receive Data Error:"+e);
			return null;
		}
 		return recvPacket;
	}
}

