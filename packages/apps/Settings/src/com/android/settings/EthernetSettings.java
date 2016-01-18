/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.settings;

import com.android.settings.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.admin.DevicePolicyManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.provider.SearchIndexableResource;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.content.Intent;

import java.util.regex.Pattern;
import java.lang.Integer;
import java.net.InetAddress;
import java.net.Inet4Address;
import java.util.Iterator;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import android.preference.Preference.OnPreferenceChangeListener;
import com.android.settings.SettingsPreferenceFragment.SettingsDialogFragment;

/*for 5.0*/
import android.net.EthernetManager;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.StaticIpConfiguration;
import android.net.NetworkUtils;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.preference.ListPreference;

import com.android.settings.ethernet_static_ip_dialog;

public class EthernetSettings extends SettingsPreferenceFragment 
			implements DialogInterface.OnClickListener ,OnPreferenceChangeListener {
    private static final String TAG = "EthernetSettings";
    
    private static final String KEY_ETH_IP_ADDRESS = "ethernet_ip_addr";
    private static final String KEY_ETH_HW_ADDRESS = "ethernet_hw_addr";
    private static final String KEY_ETH_NET_MASK = "ethernet_netmask";
    private static final String KEY_ETH_GATEWAY = "ethernet_gateway";
    private static final String KEY_ETH_DNS1 = "ethernet_dns1";
    private static final String KEY_ETH_DNS2 = "ethernet_dns2";
    private static final String KEY_ETH_MODE= "ethernet_mode_select";

    
    private  static String mEthHwAddress = null;
    private  static String mEthIpAddress = null;
    private  static String mEthNetmask = null;
    private  static String mEthGateway = null;
    private  static String mEthdns1 = null;
    private  static String mEthdns2 = null;
    private final static String nullIpInfo = "0.0.0.0";

    private ListPreference mkeyEthMode;
    private SwitchPreference mEthCheckBox;
    private CheckBoxPreference staticEthernet;
	
    private final IntentFilter mIntentFilter;
    IpConfiguration mIpConfiguration;
    EthernetManager mEthManager;
    StaticIpConfiguration mStaticIpConfiguration;
    Context mContext;
    private ethernet_static_ip_dialog mDialog;
    private static final int SHOW_RENAME_DIALOG = 0;  
    
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
	        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
                log("Action "+action);
                if(EthernetManager.ETHERNET_STATE_CHANGED_ACTION.equals(action)) {
	            	/*接收到以太网状态改变的广播*/
	            	int EtherState=intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE, -1);
	            	handleEtherStateChange(EtherState);
	        }
	   }
    };
    public EthernetSettings() {
	mIntentFilter = new IntentFilter(EthernetManager.ETHERNET_STATE_CHANGED_ACTION);
    }
	/*
	 * 
	*/
    private void handleEtherStateChange(int EtherState ) {
	log("curEtherState"+ EtherState);
		
	switch(EtherState) {
	case EthernetManager.ETHER_STATE_DISCONNECTED:
		mEthHwAddress = nullIpInfo;
		mEthIpAddress = nullIpInfo;
		mEthNetmask = nullIpInfo;
		mEthGateway = nullIpInfo;
		mEthdns1 = nullIpInfo;
		mEthdns2 = nullIpInfo;	
		break;
	case EthernetManager.ETHER_STATE_CONNECTING:
                String mStatusString = this.getResources().getString(R.string.ethernet_info_getting);
                mEthHwAddress = mStatusString;
                mEthIpAddress = mStatusString;
                mEthNetmask = mStatusString;
                mEthGateway = mStatusString;
                mEthdns1 = mStatusString;
                mEthdns2 = mStatusString;
		break;
	case EthernetManager.ETHER_STATE_CONNECTED:
		getEthInfo();
		break;
	} 
		
	refreshUI();
    }
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.ethernet_settings);
        
        mEthManager = (EthernetManager) getSystemService(Context.ETHERNET_SERVICE);
        
        if (mEthManager == null) {
		Log.e(TAG, "get ethernet manager failed");
		return;
	}
	mContext=this.getActivity().getApplicationContext();
    }

    private Inet4Address getIPv4Address(String text) {
        try {
            return (Inet4Address) NetworkUtils.numericToInetAddress(text);
        } catch (IllegalArgumentException|ClassCastException e) {
            return null;
        }
    }
 
    @Override
    public void onResume() {
        super.onResume();
        if(mkeyEthMode==null) {
            mkeyEthMode=(ListPreference)findPreference(KEY_ETH_MODE);
            mkeyEthMode.setOnPreferenceChangeListener(this);
        }
        
        if (mEthCheckBox== null) {
            mEthCheckBox =  (SwitchPreference) findPreference("ethernet");
        }
        handleEtherStateChange(mEthManager.getEthernetConnectState());
        refreshUI();
        log("resume");
        mContext.registerReceiver(mReceiver, mIntentFilter);
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        log("destory");
    }
    @Override
    public void onStop(){
    	super.onStop();
    	log("stop");
    	
        mContext.unregisterReceiver(mReceiver);
    }

    private void setStringSummary(String preference, String value) {
        try {
            findPreference(preference).setSummary(value);
        } catch (RuntimeException e) {
            findPreference(preference).setSummary("");
            log("can't find "+preference);
        }
    }
    private String getStringFromPref(String preference) {
        try {
            return findPreference(preference).getSummary().toString();
        } catch (RuntimeException e) {
            return null;
        }
    }
    private void refreshUI ( ) {
  
    //    setStringSummary(KEY_ETH_HW_ADDRESS,mEthHwAddress);
  
        setStringSummary(KEY_ETH_IP_ADDRESS, mEthIpAddress);
        setStringSummary(KEY_ETH_NET_MASK, mEthNetmask);
        setStringSummary(KEY_ETH_GATEWAY, mEthGateway);
        setStringSummary(KEY_ETH_DNS1, mEthdns1);
        setStringSummary(KEY_ETH_DNS2, mEthdns2);
	updateCheckbox();
    }
    
    private void updateCheckbox(){  //add by ljh for adding a checkbox switch
    /*	
        if(mEthManager==null){
        	((CheckBoxPreference)findPreference("static_ethernet")).setChecked(false);
        	((CheckBoxPreference)findPreference("dhcp_ethernet")).setChecked(false);
        }else{
        	boolean useDhcp=(mEthManager.getConfiguration().ipAssignment == IpAssignment.DHCP) ? true : false;
        	((CheckBoxPreference)findPreference("static_ethernet")).setChecked(!useDhcp);
        	((CheckBoxPreference)findPreference("dhcp_ethernet")).setChecked(useDhcp);
        }
      */  
         
        if(mEthManager==null){   	
        	mkeyEthMode.setSummary("null");
        } else {
        	boolean useDhcp=(mEthManager.getConfiguration().ipAssignment == IpAssignment.DHCP) ? true : false;
        	if(useDhcp){
        		mkeyEthMode.setValue("DHCP");
        		mkeyEthMode.setSummary(R.string.usedhcp);
        	}else {
        		mkeyEthMode.setValue("StaticIP");
        		mkeyEthMode.setSummary(R.string.usestatic);
        	}
                int isEnable = mEthManager.getEthernetIfaceState();
                if(isEnable == EthernetManager.ETHER_IFACE_STATE_UP) {
                   mEthCheckBox.setChecked(true);   
                }else mEthCheckBox.setChecked(false);
        }    
    }
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
   // 	 log("onPreferenceTreeclick");
    	if(preference==mkeyEthMode) {
    	     String value=(String)newValue;
    	     if(value.equals("DHCP")){
    	        	mEthManager.setConfiguration(new IpConfiguration(IpAssignment.DHCP, ProxySettings.NONE,null,null));
    	        	log("switch to dhcp");
    	     }else if(value.equals("StaticIP")){
    	        	log("static editor");       	
    	        	this.showDialog(SHOW_RENAME_DIALOG);
    	     }
    		
    	}
    	return true;
    }
    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen screen, Preference preference) {
    	
        if (preference == mEthCheckBox) {
            boolean newState = mEthCheckBox.isChecked();
            if(newState) {
                log("turn on Ethernet");
                mEthManager.setEthernetEnabled(true);   
            } else {
                log("turn off Ethernet");
                mEthManager.setEthernetEnabled(false);
            }
            //log("IpAssignment: "+mEthManager.getConfiguration().toString());
        }
       /* else if(preference ==  (CheckBoxPreference) findPreference("dhcp_ethernet")) {//config dhcp
        	mEthManager.setConfiguration(new IpConfiguration(IpAssignment.DHCP, ProxySettings.NONE,null,null));
        	log("switch to dhcp");
       } else if (preference ==  (CheckBoxPreference) findPreference("static_ethernet")) { //config static ip
        	log("static editor");       	
        	this.showDialog(SHOW_RENAME_DIALOG);
       }*/
        return super.onPreferenceTreeClick(screen, preference);
    } 
    
  //将子网掩码转换成ip子网掩码形式，比如输入32输出为255.255.255.255  
    public  String interMask2String(int prefixLength) {
        String netMask = null;
		int inetMask = prefixLength;
		
		int part = inetMask / 8;
		int remainder = inetMask % 8;
		int sum = 0;
		
		for (int i = 8; i > 8 - remainder; i--) {
			sum = sum + (int) Math.pow(2, i - 1);
		}
		
		if (part == 0) {
			netMask = sum + ".0.0.0";
		} else if (part == 1) {
			netMask = "255." + sum + ".0.0";
		} else if (part == 2) {
			netMask = "255.255." + sum + ".0";
		} else if (part == 3) {
			netMask = "255.255.255." + sum;
		} else if (part == 4) {
			netMask = "255.255.255.255";
		}

		return netMask;
	}

    /*
     * convert subMask string to prefix length
     */
    private int maskStr2InetMask(String maskStr) {
    	StringBuffer sb ;
    	String str;
    	int inetmask = 0; 
    	int count = 0;
    	/*
    	 * check the subMask format
    	 */
      	Pattern pattern = Pattern.compile("(^((\\d|[01]?\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d|[01]?\\d\\d|2[0-4]\\d|25[0-5])$)|^(\\d|[1-2]\\d|3[0-2])$");
    	if (pattern.matcher(maskStr).matches() == false) {
    		Log.e(TAG,"subMask is error");
    		return 0;
    	}
    	
    	String[] ipSegment = maskStr.split("\\.");
    	for(int n =0; n<ipSegment.length;n++) {
    		sb = new StringBuffer(Integer.toBinaryString(Integer.parseInt(ipSegment[n])));
    		str = sb.reverse().toString();
    		count=0;
    		for(int i=0; i<str.length();i++) {
    			i=str.indexOf("1",i);
    			if(i==-1)  
    				break;
    			count++;
    		}
    		inetmask+=count;
    	}
    	return inetmask;
    }

    private boolean setStaticIpConfiguration() {
    	
        mStaticIpConfiguration =new StaticIpConfiguration();
		 /*
		  * get ip address, netmask,dns ,gw etc.
		  */	 
        Inet4Address inetAddr = getIPv4Address(this.mEthIpAddress);
        int prefixLength = maskStr2InetMask(this.mEthNetmask); 
        InetAddress gatewayAddr =getIPv4Address(this.mEthGateway); 
        InetAddress dnsAddr = getIPv4Address(this.mEthdns1);
		 
        if (inetAddr.getAddress().toString().isEmpty() || prefixLength ==0 || gatewayAddr.toString().isEmpty()
		  || dnsAddr.toString().isEmpty()) {
              log("ip,mask or dnsAddr is wrong");
			  return false;
	}
		  
        String dnsStr2=this.mEthdns2;  
        mStaticIpConfiguration.ipAddress = new LinkAddress(inetAddr, prefixLength);
        mStaticIpConfiguration.gateway=gatewayAddr;
        mStaticIpConfiguration.dnsServers.add(dnsAddr);
  
        if (!dnsStr2.isEmpty()) {
            mStaticIpConfiguration.dnsServers.add(getIPv4Address(dnsStr2));
	} 
	mIpConfiguration=new IpConfiguration(IpAssignment.STATIC, ProxySettings.NONE,mStaticIpConfiguration,null);  
        return true;
    }

    public void getEthInfoFromDhcp(){	
	String tempIpInfo;
	String iface = "eth0";
		
	tempIpInfo = SystemProperties.get("dhcp."+ iface +".ipaddress");

	if ((tempIpInfo != null) && (!tempIpInfo.equals("")) ){ 
		mEthIpAddress = tempIpInfo;
    	} else {  
    		mEthIpAddress = nullIpInfo;
    	}
				
	tempIpInfo = SystemProperties.get("dhcp."+ iface +".mask");	
	if ((tempIpInfo != null) && (!tempIpInfo.equals("")) ){
            mEthNetmask = tempIpInfo;
    	} else {           		
    		mEthNetmask = nullIpInfo;
    	}
					
	tempIpInfo = SystemProperties.get("dhcp."+ iface +".gateway");	
	if ((tempIpInfo != null) && (!tempIpInfo.equals(""))){
        	mEthGateway = tempIpInfo;
    	} else {
    		mEthGateway = nullIpInfo;        		
    	}

	tempIpInfo = SystemProperties.get("dhcp."+ iface +".dns1");
	if ((tempIpInfo != null) && (!tempIpInfo.equals(""))){
       		mEthdns1 = tempIpInfo;
    	} else {
    		mEthdns1 = nullIpInfo;      		
    	}

	tempIpInfo = SystemProperties.get("dhcp."+ iface +".dns2");
	if ((tempIpInfo != null) && (!tempIpInfo.equals(""))){
       		mEthdns2 = tempIpInfo;
    	} else {
    		mEthdns2 = nullIpInfo;       		
    	}
    }

    public void getEthInfoFromStaticIp() {
	StaticIpConfiguration staticIpConfiguration=mEthManager.getConfiguration().getStaticIpConfiguration();
		
	if(staticIpConfiguration == null) {
		return ;
	}
	LinkAddress ipAddress = staticIpConfiguration.ipAddress;
	InetAddress gateway   = staticIpConfiguration.gateway;
	ArrayList<InetAddress> dnsServers=staticIpConfiguration.dnsServers;
	
	if( ipAddress !=null) {
		mEthIpAddress=ipAddress.getAddress().getHostAddress();
		mEthNetmask=interMask2String(ipAddress.getPrefixLength());
	}
	if(gateway !=null) {
		mEthGateway=gateway.getHostAddress();
	}
		mEthdns1=dnsServers.get(0).getHostAddress();
	
	if(dnsServers.size() > 1) { /* 只保留两个*/
		mEthdns2=dnsServers.get(1).getHostAddress();
	}		
   }
	/*
	 * TODO:
	 */
    public void getEthInfo(){
		/*
		mEthHwAddress = mEthManager.getEthernetHwaddr(mEthManager.getEthernetIfaceName());
		if (mEthHwAddress == null) mEthHwAddress = nullIpInfo;
		*/
        IpAssignment mode =mEthManager.getConfiguration().getIpAssignment();
        
	   
         if ( mode== IpAssignment.DHCP) {
	   /*
	    * getEth from dhcp
	   */
            getEthInfoFromDhcp();
	} else if(mode == IpAssignment.STATIC) {
	   /*
	    * TODO: get static IP
	   */
            getEthInfoFromStaticIp();
	}     
    }

    /*
     * tools
    */
    private void log(String s) {
        Log.d(TAG, s);
    }
    
    @Override
    public void onClick(DialogInterface dialogInterface, int button) {
    	if(button==ethernet_static_ip_dialog.BUTTON_SUBMIT) {
    	mDialog.saveIpSettingInfo(); //从Dialog获取静态数据  	
    	if(setStaticIpConfiguration()) {
            mEthManager.setConfiguration(mIpConfiguration); 	
    	} else {
            Log.e(TAG, mIpConfiguration.toString());
    	}
    	}
    	updateCheckbox();
    }

    @Override
    public Dialog onCreateDialog(int dialogId) {
    	log("onCreateDialog "+dialogId);
    	switch(dialogId) {
    	case SHOW_RENAME_DIALOG:	
  
        	mDialog = new ethernet_static_ip_dialog(getActivity(), false, this,mGetStaticIpInfo);
        	return mDialog;
    	}
    	return super.onCreateDialog(dialogId);
    }
    /*interface*/
    
    public getStaticIpInfo mGetStaticIpInfo =new getStaticIpInfo() {
    	
    	public boolean getStaticIp(String ipAddr) {    		
    		mEthIpAddress = ipAddr;
    		
    		log("ipAddr: "+ipAddr);
    		return true;
    	}
    	public boolean getStaticNetMask(String netMask) {
    		mEthNetmask =netMask ;
    		
    		log("netMask: "+netMask);
    		return true;
    	}
    	public boolean getStaticGateway(String gateway) {
    		mEthGateway=gateway;
    		
    		log("gateway: "+gateway);
    		return true;
    	}
    	public boolean getStaticDns1(String dns1) {
    		mEthdns1=dns1;
    		
    		log("dns1: "+dns1);
    		return true;
    	}
    	public boolean getStaticDns2(String dns2) {
    		mEthdns2=dns2;
    		
    		log("dns2: "+dns2);
    		return true;
    	}
    };
   
}
