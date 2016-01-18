package com.android.server.pppoe;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.DhcpResults;
import android.net.InterfaceConfiguration;
import android.net.NetworkUtils;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkAgent;
import android.net.NetworkCapabilities;
import android.net.NetworkFactory;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.NetworkRequest;
import android.net.EthernetManager;
import android.net.StaticIpConfiguration;
import android.os.Handler;
import android.os.IBinder;
import android.os.INetworkManagementService;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.text.TextUtils;
import android.util.Log;
import android.content.Intent;
import android.os.UserHandle;
import android.os.SystemProperties;
import com.android.internal.util.IndentingPrintWriter;
import com.android.server.net.BaseNetworkObserver;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.FileNotFoundException;

import java.util.ArrayList;
import java.net.InetAddress;
import java.util.List;
import android.net.RouteInfo;
import android.net.pppoe.PppoeManager;
import android.net.pppoe.IPppoeManager;
import java.net.Inet4Address;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import android.provider.Settings;
import android.text.TextUtils;

/**
 *  track pppoe state
 *  pppoe connect & setup & disconnect
 *  @hide
**/

class PppoeNetworkFactory {
    private static final String NETWORK_TYPE = "PPPOE";
    private static final String TAG = "PppoeNetworkFactory";
    private static final int NETWORK_SCORE = 160;
    private static final boolean DBG = true;
    private static final boolean VDBG= false;

    private static void LOG(String msg) {
        if (DBG) {
            Log.d(TAG, msg);
        }
    }
    private static void LOGD(String msg) {
        if (VDBG) {
            Log.d(TAG,msg);
        }
    }

    /** Tracks interface changes. Called from NetworkManagementService. */
    private InterfaceObserver mInterfaceObserver;

    /** For static IP configuration */
    private PppoeManager mPppoeManager;

    /** To set link state and configure IP addresses. */
    private INetworkManagementService mNMService;

    /* To communicate with ConnectivityManager */
    private NetworkCapabilities mNetworkCapabilities;
    private NetworkAgent mNetworkAgent;
    private LocalNetworkFactory mFactory;
    private Context mContext;

    /** Product-dependent regular expression of interface names we track. */
    private static String mIfaceMatch = "ppp\\d";

    /** Data members. All accesses to these must be synchronized(this). */
    private static String mIface = "";
    private String mPhyIface = "";
    private String mHwAddr;
    private static boolean mLinkUp;
    private NetworkInfo mNetworkInfo;
    private LinkProperties mLinkProperties;
    private int mPppoeCurrentState;

    static {
        /* Native functions are defined in libpppoe-jni.so */
        System.loadLibrary("pppoe-jni");
        registerNatives();
    }

    /*=============================================================================================================*/

    public int mPppoeState = PppoeManager.PPPOE_STATE_DISCONNECTED;

    private void setPppoeStateAndSendBroadcast(int newState) {
        int preState = mPppoeState;
        mPppoeState = newState;
        
        final Intent intent = new Intent(PppoeManager.PPPOE_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);     
        intent.putExtra(PppoeManager.EXTRA_PPPOE_STATE, newState);
        intent.putExtra(PppoeManager.EXTRA_PREVIOUS_PPPOE_STATE, preState);
        LOG("setPppoeStateAndSendBroadcast() : preState = " + preState +", curState = " + newState);
        mContext.sendStickyBroadcast(intent);
    }


    PppoeNetworkFactory() {
        mNetworkInfo = new NetworkInfo(ConnectivityManager.TYPE_PPPOE, 0, NETWORK_TYPE, "");
        mLinkProperties = new LinkProperties();
        initNetworkCapabilities();
    }

    private void initNetworkCapabilities() {

        mNetworkCapabilities = new NetworkCapabilities();
        mNetworkCapabilities.addTransportType(NetworkCapabilities.TRANSPORT_PPPOE);
        mNetworkCapabilities.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        mNetworkCapabilities.addCapability(NetworkCapabilities.NET_CAPABILITY_NOT_RESTRICTED);
        // We have no useful data on bandwidth. Say 100M up and 100M down. :-(
        mNetworkCapabilities.setLinkUpstreamBandwidthKbps(100 * 1000);
        mNetworkCapabilities.setLinkDownstreamBandwidthKbps(100 * 1000);
    }

    private class LocalNetworkFactory extends NetworkFactory {
        LocalNetworkFactory(String name, Context context, Looper looper) {
            super(looper, context, name, new NetworkCapabilities());
        }

        protected void startNetwork() {
            onRequestNetwork();
        }
        protected void stopNetwork() {
        }
    }

    private class InterfaceObserver extends BaseNetworkObserver {
        @Override
        public void interfaceLinkStateChanged(String iface, boolean up) {
            updateInterfaceState(iface, up);
        }

        @Override
        public void interfaceAdded(String iface) {
            maybeTrackInterface(iface);
        }

        @Override
        public void interfaceRemoved(String iface) {
            stopTrackingInterface(iface);
        }
    }
    public void updateInterfaceState(String iface, boolean up) {

        LOGD("updateInterface: mIface:" + mIface + " mPhyIface:"+ mPhyIface + " iface:" + iface + " link:" + (up ? "up" : "down"));

        if (!mIface.equals(iface) && !mPhyIface.equals(iface)) {
            LOGD("not tracker interface");
            return;
        }

        if(mPhyIface.equals(iface)) { //需要监所连接物理端口
           if(!up) {
               machineStopPppoe(); //当物理网口断开时,断开pppoe链接
               stopTrackingInterface(iface);
           } else {
/*add for auto connect by blb*/
               int autoPppOn=Settings.Secure.getInt(mContext.getContentResolver(), Settings.Secure.PPPOE_ON, 0);
               Log.d(TAG, "PPPOE_ON:"+autoPppOn);
               if(autoPppOn == 1) autoconnect();
/*end*/
           }
 	   return;
        }
        LOG("updateInterface: " + iface + " link " + (up ? "up" : "down"));

        synchronized(this) {
            mLinkUp = up;
            mNetworkInfo.setIsAvailable(up);
            if (!up) {
                // Tell the agent we're disconnected. It will call disconnect().
                mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, mHwAddr);
                // sendEthernetStateChangedBroadcast(EthernetManager.ETHER_STATE_DISCONNECTED);
            }
             updateAgent();
            // set our score lower than any network could go
            // so we get dropped.  TODO - just unregister the factory
            // when link goes down.
            mFactory.setScoreFilter(up ? NETWORK_SCORE : -1);
        }
    }

    public boolean maybeTrackInterface(String iface) {

        // If we don't already have an interface, and if this interface matches
        // our regex, start tracking it.
        if (!iface.matches(mIfaceMatch) || !mIface.isEmpty())
            return false;

        LOG("Started tracking interface " + iface);
        setInterfaceUp(iface);
        return true;   
    }

    public void updateAgent() {
       LOG("updateAgent");

        synchronized (PppoeNetworkFactory.this) {
            if (mNetworkAgent == null) {
                LOG("mNetworkAgent is null");
                 return;
            }

            if (DBG) {
                Log.i(TAG, "Updating mNetworkAgent with: " +
                      mNetworkCapabilities + ", " +
                      mNetworkInfo + ", " +
                      mLinkProperties);
            }

            mNetworkAgent.sendNetworkCapabilities(mNetworkCapabilities);
            mNetworkAgent.sendNetworkInfo(mNetworkInfo);
            mNetworkAgent.sendLinkProperties(mLinkProperties);
            // never set the network score below 0.
            mNetworkAgent.sendNetworkScore(mLinkUp? NETWORK_SCORE : 0);
        }
    }

   private void setInterfaceUp(String iface) {
        // Bring up the interface so we get link status indications.
        try {
            mNMService.setInterfaceUp(iface);
            String hwAddr = null;
            InterfaceConfiguration config = mNMService.getInterfaceConfig(iface);

            if (config == null) {
                Log.e(TAG, "Null iterface config for " + iface + ". Bailing out.");
                return;
            }

            synchronized (this) {
                if (mIface.isEmpty()) {
                    mIface = iface;
                    mHwAddr = config.getHardwareAddress();
                    mNetworkInfo.setIsAvailable(true);
                    mNetworkInfo.setExtraInfo(mHwAddr);
                } else {
                    Log.e(TAG, "Interface unexpectedly changed from " + iface + " to " + mIface);
                    mNMService.setInterfaceDown(iface);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error upping interface " + mIface + ": " + e);
        }
    }


    public void stopTrackingInterface(String iface) {
        LOG("stopTrackingInterface");

        if (!iface.equals(mIface))
            return;

        Log.d(TAG, "Stopped tracking interface " + iface);
        // TODO: Unify this codepath with stop().
        synchronized (this) {

            mIface = "";
            mHwAddr = null;
            mNetworkInfo.setExtraInfo(null);
            mLinkUp = false;
            mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null, mHwAddr);
            updateAgent();
            mNetworkAgent = null;
            mNetworkInfo = new NetworkInfo(ConnectivityManager.TYPE_PPPOE, 0, NETWORK_TYPE, "");
            mLinkProperties = new LinkProperties();
        }
    }
    public void onRequestNetwork() {
           LOG("onRequestNetwork");
    }


    public void connected() {
        LOG("connected");
 
       LinkProperties linkProperties;
     
       synchronized(PppoeNetworkFactory.this) {
            if (mNetworkAgent != null) {
                Log.e(TAG, "Already have a NetworkAgent - aborting new request");
                return;
            }

            linkProperties = new LinkProperties();
            linkProperties.setInterfaceName(this.mIface);

            String[] dnses = new String[2];
	    String route;
            String ipaddr;

	    pppoeSetDns(dnses);

            List<InetAddress>dnsServers = new ArrayList<InetAddress>();
            dnsServers.add(NetworkUtils.numericToInetAddress(dnses[0]));
            dnsServers.add(NetworkUtils.numericToInetAddress(dnses[1]));
	    linkProperties.setDnsServers(dnsServers);

	    route = SystemProperties.get("net."+mIface+".gw");
	    Log.e(TAG, "route:"+route);
	    linkProperties.addRoute(new RouteInfo(NetworkUtils.numericToInetAddress(route)));
            ipaddr = SystemProperties.get("net."+mIface+".local-ip");

            linkProperties.addLinkAddress(new LinkAddress(NetworkUtils.numericToInetAddress(ipaddr),28));



            mLinkProperties = linkProperties;
            
            mNetworkInfo.setIsAvailable(true);
            mNetworkInfo.setDetailedState(DetailedState.CONNECTED, null, mHwAddr);

            // Create our NetworkAgent.
            mNetworkAgent = new NetworkAgent(mFactory.getLooper(), mContext,
                            NETWORK_TYPE, mNetworkInfo, mNetworkCapabilities, mLinkProperties,
                            NETWORK_SCORE) {
            public void unwanted() {
                synchronized(PppoeNetworkFactory.this) {
                if (this == mNetworkAgent) {
                     LOG("unWanted");
                     mLinkProperties.clear();
                     mNetworkInfo.setDetailedState(DetailedState.DISCONNECTED, null,mHwAddr);
                     updateAgent();
                     mNetworkAgent = null;

                  } else {
                     Log.d(TAG, "Ignoring unwanted as we have a more modern " +
                                            "instance");
                  }
                }
              };
            };
        }
    }

    private void pppoeSetDns(String[] dnses) {
        try {
            File file = new File ("/data/misc/ppp/resolv.conf");
            FileInputStream fis = new FileInputStream(file);
            InputStreamReader input = new InputStreamReader(fis);
            BufferedReader br =  new BufferedReader(input, 128);
            String str;
            int i = 0;

            Log.d(TAG, "pppoeSetDns");

            while((str=br.readLine()) != null) {
                String dns = str.substring(11);
                Log.d(TAG, "dns"+i+":="+dns);
	 //	mLinkProperties.addDns(NetworkUtils.numericToInetAddress(dns));
		dnses[i] = dns;
		i++;
            }
            br.close();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "resolv.conf not found");
        } catch (IOException e) {
            Log.e(TAG, "handle resolv.conf failed");
        }
    }
   
    /**
     * Begin monitoring connectivity
     */
    public synchronized void start(Context context, Handler target) {

        // The services we use.
        IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
        mNMService = INetworkManagementService.Stub.asInterface(b);
        mPppoeManager = (PppoeManager) context.getSystemService(Context.PPPOE_SERVICE);

        // Create and register our NetworkFactory.
        mFactory = new LocalNetworkFactory(NETWORK_TYPE, context, target.getLooper());
        mFactory.setCapabilityFilter(mNetworkCapabilities);
        mFactory.setScoreFilter(-1); // this set high when we have an iface
        mFactory.register();
        mContext = context;

        // Start tracking interface change events.
        mInterfaceObserver = new InterfaceObserver();
        try {
            mNMService.registerObserver(mInterfaceObserver);
        } catch (RemoteException e) {
            Log.e(TAG, "Could not register InterfaceObserver " + e);
        }

/*add for auto connect by blb*/
        mHandler.postDelayed(autoRunnable,8000);//
/*end*/
    }

/*============================================================================================================*/

    public String getPppoePhyIface() {

       return mPhyIface; 
    }


    public int getPppoeState() {
       
        return mPppoeState;
    }
    public boolean setupPppoe(String user, String iface, String dns1, String dns2, String password) {
     
        LOG("setupPppoe: ");
        LOG("user="+user);
        LOG("iface="+iface);
        LOG("dns1="+dns1);
        LOG("dns2="+dns2);
        LOG("password="+password);

        mPhyIface = iface;

        if (user==null || iface==null || password==null) return false;
        if (dns1==null) dns1="";
        if (dns2==null) dns2="";
        Settings.Secure.putString(mContext.getContentResolver(),Settings.Secure.PPPOE_USERNAME,user);
        Settings.Secure.putString(mContext.getContentResolver(),Settings.Secure.PPPOE_PSWD,password);
        if (0 == setupPppoeNative(user, iface, dns1, dns2, password)) {
            return true;
        } else {
            return false;
        }
    }
    public boolean machineStartPppoe() {
        LOG("startPppoe");
        setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_CONNECTING);
        if ( 0 != startPppoeNative() ) {
            Log.e(TAG,"machineStartPppoe() : fail to start pppoe!");
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTED);
            Settings.Secure.putInt(mContext.getContentResolver(),Settings.Secure.PPPOE_ON,0);

            return false;
        } else {
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_CONNECTED);
            connected();
            return true;
        }
    }

    public boolean machineStopPppoe() {
        setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTING);

        if ( 0 != stopPppoeNative() ) {
            Log.e(TAG,"stopPppoe() : fail to stop pppoe!");
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTED);
            return false;
        } else {
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTED);
            return true;
        }
    }

    public boolean startPppoe() {

        LOG("startPppoe");
        setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_CONNECTING);
        if ( 0 != startPppoeNative() ) {
            Log.e(TAG,"startPppoe() : fail to start pppoe!");    
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTED);
            
            return false;
        } else {
            Settings.Secure.putInt(mContext.getContentResolver(),Settings.Secure.PPPOE_ON,1);
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_CONNECTED);
            connected();
            return true;
        }
    }
    public boolean stopPppoe() {
        setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTING);

        if ( 0 != stopPppoeNative() ) {
            Log.e(TAG,"stopPppoe() : fail to stop pppoe!");
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTED);
            return false;
        } else {
            Settings.Secure.putInt(mContext.getContentResolver(),Settings.Secure.PPPOE_ON,0);
            setPppoeStateAndSendBroadcast(PppoeManager.PPPOE_STATE_DISCONNECTED);
            return true;
        }
    }
/*add for auto connect by blb*/
    Handler mHandler = new Handler();
    Runnable autoRunnable = new Runnable() {
        @Override
        public void run() {
          // TODO Auto-generated method stub^M
          int autoPppOn=Settings.Secure.getInt(mContext.getContentResolver(), Settings.Secure.PPPOE_ON, 0);
          if(autoPppOn == 1)  autoconnect();
        }
    };

    public boolean autoconnect() {
        String user;
        String password;
        String iface = "eth0";
        user = Settings.Secure.getString(mContext.getContentResolver(),Settings.Secure.PPPOE_USERNAME);
        password = Settings.Secure.getString(mContext.getContentResolver(),Settings.Secure.PPPOE_PSWD);

        if(!TextUtils.isEmpty(user) && !TextUtils.isEmpty(password)) {
            setupPppoe(user ,iface , "","" , password);
            machineStartPppoe();
        } else {
            Log.e(TAG,"user empty or password empty");
            return false;
        }

        return true;
    }
    public native static int setupPppoeNative(String user, String iface,String dns1, String dns2, String password);
    public native static int startPppoeNative();
    public native static int stopPppoeNative();
    public native static int isPppoeUpNative();
    public native static int registerNatives();
}
