package com.android.server.pppoe;

import android.content.Context;
import android.content.pm.PackageManager;
import com.android.internal.util.IndentingPrintWriter;
import android.net.ConnectivityManager;
import android.net.IEthernetManager;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.LinkAddress;
import android.net.NetworkAgent;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.RouteInfo;
import android.net.StaticIpConfiguration;
import android.os.Binder;
import android.os.IBinder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.INetworkManagementService;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.PrintWriterPrinter;

import android.content.Intent;
import android.content.IntentFilter;
import android.net.pppoe.PppoeManager;
import android.net.pppoe.IPppoeManager;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.concurrent.atomic.AtomicBoolean;
import android.provider.Settings;
/**
 * @hide
 */
public class PppoeServiceImpl extends IPppoeManager.Stub {
    private static final String TAG = "PppoeServiceImpl";
    public static final boolean DEBUG = true;
    public Context mContext;
    public String mIface="";
    public Handler mHandler;
    private final INetworkManagementService mNMService;
    private final AtomicBoolean mStarted = new AtomicBoolean(false);
    private final PppoeNetworkFactory mTracker;
    private ConnectivityManager mCM;

    private static void LOG(String msg) {
        if ( DEBUG ) {
            Log.d(TAG, msg);
        }
    }

    private void enforceAccessPermission() {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.ACCESS_NETWORK_STATE,
                "PppoeService");
    }

    private void enforceChangePermission() {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.CHANGE_NETWORK_STATE,
                "PppoeService");
    }

    private void enforceConnectivityInternalPermission() {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.CONNECTIVITY_INTERNAL,
                "ConnectivityService");
    }


    public PppoeServiceImpl(Context context) {
         Log.i(TAG, "Creating PppoeServiceImpl");
         mContext=context;

         IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
         mNMService = INetworkManagementService.Stub.asInterface(b);

         mTracker=new PppoeNetworkFactory();
    }

    public int getPppoeState() {
       
        return mTracker.mPppoeState;
    }
    public boolean setupPppoe(String user, String iface, String dns1, String dns2, String password) {

        return mTracker.setupPppoe(user,iface,dns1,dns2,password);
    }
    public boolean startPppoe() {

        return mTracker.startPppoe();

    }
    public boolean stopPppoe() {

         return mTracker.stopPppoe();
    }
    public String  getPppoePhyIface() {

        return mTracker.getPppoePhyIface();

    }

    public void start() {
        Log.i(TAG, "Starting PPPOE service");
        mCM = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);

        HandlerThread handlerThread = new HandlerThread("PppoeServiceThread");
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper());
        mTracker.start(mContext, mHandler);
        mStarted.set(true);
        
    }
}
