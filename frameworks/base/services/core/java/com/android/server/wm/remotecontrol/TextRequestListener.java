package com.android.server.wm.remotecontrol;

import android.content.Context;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import com.android.internal.view.IInputMethodManager;

public class TextRequestListener implements ControlSocket.RequestListener{
	private static final String TAG = "TextRequestListener";
	private static final boolean DEBUG = true;
	private void LOG(String msg){
		if (DEBUG) {
			Log.d(TAG,msg);
		}
	}

	private Context mContext;
	private IInputMethodManager IMM;
	
	public TextRequestListener(Context context) {
		mContext = context;
	}

	private IInputMethodManager getInputMethodService() {
        	if (IMM == null) {
           		 IMM = IInputMethodManager.Stub.asInterface(
                    	ServiceManager.getService(Context.INPUT_METHOD_SERVICE));
            		if (IMM == null) {
               		 LOG("warning: no STATUS_BAR_SERVICE");
            		}
        	}
        	return IMM;
    	}

	@Override
	public void requestRecieved(UDPPacket packet) {
		// TODO Auto-generated method stub
		
		TextControlRequest request = new TextControlRequest(packet);

		String text = request.getText();
		//LOG("recieved text = " + text);
		IInputMethodManager IIMM = getInputMethodService();
		if (IIMM != null)
			try {
				IIMM.commitText(text);
			} catch (RemoteException e) {
				//TODO: handle exception
				LOG("InputMethodService exception:"+e);
			}
	}
}