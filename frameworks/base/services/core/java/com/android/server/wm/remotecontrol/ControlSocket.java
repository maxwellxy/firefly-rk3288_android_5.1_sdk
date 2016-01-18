/*******************************************************************
* Company:     Fuzhou Rockchip Electronics Co., Ltd
* Filename:    ControlSocket.java  
* Description:   
* @author:     fxw@rock-chips.com
* Create at:   2011-12-19 11:01:51  
* 
* Modification History:  
* Date         Author      Version     Description  
* ------------------------------------------------------------------  
* 2011-12-19      xwf         1.0         create
*******************************************************************/   


package com.android.server.wm.remotecontrol;

import java.net.DatagramSocket;
import java.net.InetAddress;
import android.os.Process;
import android.util.Log;


public class ControlSocket extends UDPSocket implements Runnable {

	public interface RequestListener {
		public void requestRecieved(UDPPacket packet);
	}
	
	private Thread responseThread = null;
	private RequestListener mRequestListener;
	
	private String ThreadNameHead;
	
	public ControlSocket(String NameHead) {
		ThreadNameHead = NameHead;
	}
	
	public ControlSocket(int port,String NameHead){
		super(port);
		ThreadNameHead = NameHead;
	}

	public ControlSocket(String bindAddr, int port, String NameHead){
		super(bindAddr, port);
		ThreadNameHead = NameHead;
	}
	
	public void setRequestListener(RequestListener requestListener){
		mRequestListener = requestListener;
	}
	
	/** 
	 * <p>Title: run</p> 
	 * <p>Description: </p>  
	 * @see java.lang.Runnable#run() 
	 */
	@Override
	public void run() {
//		Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_DISPLAY);
		Process.setThreadPriority(Process.THREAD_PRIORITY_DISPLAY); //change by cx
		Process.setCanSelfBackground(false);
		Thread thisThread = Thread.currentThread();
		while (responseThread == thisThread) {
			Thread.yield();
			final UDPPacket packet = receive();
			if(packet==null) return;
			if (mRequestListener != null){
				mRequestListener.requestRecieved(packet);
			}
		}
	}
	
	public void start()	{
		Log.d(ThreadNameHead,"responseThread start");
		StringBuffer name = new StringBuffer(ThreadNameHead+"/");
		DatagramSocket s = getDatagramSocket();
		InetAddress localAddr = s.getLocalAddress();
		if (localAddr != null) {
			name.append(s.getLocalAddress()).append(':');
			name.append(s.getLocalPort());
		}
		Log.d(ThreadNameHead,"thread name:"+name.toString());
		responseThread = new Thread(this,name.toString());
		responseThread.start();
	}
	
	public void stop(){
		close();
		responseThread = null;
	}
	
	public boolean post(String addr, int port, RemoteControlRequest request){
		return post(addr, port, request.encodeMessage());
	}
	
}
