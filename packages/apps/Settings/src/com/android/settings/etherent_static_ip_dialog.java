/*
 * Copyright (C) 2010 The Android Open Source Project
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

import java.net.Inet4Address;
import java.net.InetAddress;

import android.net.NetworkUtils;

import com.android.settings.R;

import java.util.regex.Pattern;

import android.content.Context;
import android.preference.EditTextPreference;
import android.provider.Settings.System;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.provider.Settings.System;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Button;

class ethernet_static_ip_dialog extends AlertDialog implements TextWatcher {
	
	public getStaticIpInfo mGetStaticInfo; 
	private TextView mIpAddressView;
	private TextView mIPgateway;
	private TextView ipnetmask;
	private TextView mdns1;
	private TextView mdns2;

	public EditText ip_address;
	public EditText ip_gateway;
	public EditText gateway;
	public EditText dns1;
	public EditText dns2;

	static final int BUTTON_SUBMIT = DialogInterface.BUTTON_POSITIVE;
	static final int BUTTON_FORGET = DialogInterface.BUTTON_NEUTRAL;

	// private final boolean mEdit;
	private final DialogInterface.OnClickListener mListener;

	private View mView;
	Context mcontext;

	// private boolean mHideSubmitButton;

	public ethernet_static_ip_dialog(Context context, boolean cancelable,
			DialogInterface.OnClickListener listener,getStaticIpInfo GetgetStaticIpInfo) {
		super(context);
		mcontext = context;
		mListener = listener;
		mGetStaticInfo=GetgetStaticIpInfo;
		// TODO Auto-generated constructor stub
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		mView = getLayoutInflater().inflate(R.layout.static_ip_dialog, null);
		setView(mView);
		setInverseBackgroundForced(true);

		mIpAddressView = (TextView) mView.findViewById(R.id.ipaddress);
		ipnetmask = (TextView) mView.findViewById(R.id.network_prefix_length);
		mIPgateway = (TextView) mView.findViewById(R.id.gateway);
		mdns1 = (TextView) mView.findViewById(R.id.dns1);
		mdns2 = (TextView) mView.findViewById(R.id.dns2);

		mIpAddressView.addTextChangedListener(this);
		ipnetmask.addTextChangedListener(this);
		mIPgateway.addTextChangedListener(this);
		mdns1.addTextChangedListener(this);
		mdns2.addTextChangedListener(this);

		setButton(BUTTON_SUBMIT, mcontext.getString(R.string.ethernet_connect), mListener);
		setButton(BUTTON_NEGATIVE,mcontext.getString(R.string.ethernet_cancel), mListener);
		setTitle(mcontext.getString(R.string.ethernet_settings));

		super.onCreate(savedInstanceState);
	}

	@Override
	public void onStart() {
		super.onStart();
		updateIpSettingsInfo();
		checkIPValue();
	}
	private void updateIpSettingsInfo() {
		Log.d("blb", "Static IP status updateIpSettingsInfo");
		ContentResolver contentResolver = mcontext.getContentResolver();
		String staticip = System.getString(contentResolver,
				System.ETHERNET_STATIC_IP);
		if (!TextUtils.isEmpty(staticip))
			mIpAddressView.setText(staticip);

		String ipmask = System.getString(contentResolver,
				System.ETHERNET_STATIC_NETMASK);
		if (!TextUtils.isEmpty(ipmask))
			ipnetmask.setText(ipmask);

		String gateway = System.getString(contentResolver,
				System.ETHERNET_STATIC_GATEWAY);
		if (!TextUtils.isEmpty(gateway))
			mIPgateway.setText(gateway);

		String dns1 = System.getString(contentResolver,
				System.ETHERNET_STATIC_DNS1);
		if (!TextUtils.isEmpty(dns1))
			mdns1.setText(dns1);

		String dns2 = System.getString(contentResolver,
				System.ETHERNET_STATIC_DNS2);
		if (!TextUtils.isEmpty(dns2))
			mdns2.setText(dns2);
	}

	public void saveIpSettingInfo() {
		ContentResolver contentResolver = mcontext.getContentResolver();
		String ipAddr = mIpAddressView.getText().toString();
		String gateway = mIPgateway.getText().toString();
		String netMask =ipnetmask.getText().toString();
		String dns1 = mdns1.getText().toString();
		String dns2 = mdns2.getText().toString();
		int network_prefix_length = 24;// Integer.parseInt(ipnetmask.getText().toString());
		if (!TextUtils.isEmpty(ipAddr)) { // not empty
			System.putString(contentResolver, System.ETHERNET_STATIC_IP, ipAddr);
		} else {
			System.putString(contentResolver, System.ETHERNET_STATIC_IP, null);
		}
		if (!TextUtils.isEmpty(gateway)) { // not empty
			System.putString(contentResolver, System.ETHERNET_STATIC_GATEWAY,
					gateway);
		} else {
			System.putString(contentResolver, System.ETHERNET_STATIC_GATEWAY,
					null);
		}
		if (!TextUtils.isEmpty(netMask)) { // not empty
			System.putString(contentResolver, System.ETHERNET_STATIC_NETMASK,
					netMask);
		} else {
			System.putString(contentResolver, System.ETHERNET_STATIC_NETMASK,
					null);
		}
		if (!TextUtils.isEmpty(dns1)) { // not empty
			System.putString(contentResolver, System.ETHERNET_STATIC_DNS1, dns1);
		} else {
			System.putString(contentResolver, System.ETHERNET_STATIC_DNS1, null);
		}
		if (!TextUtils.isEmpty(dns2)) { // not empty
			System.putString(contentResolver, System.ETHERNET_STATIC_DNS2, dns2);
		} else {
			System.putString(contentResolver, System.ETHERNET_STATIC_DNS2, null);
		}
		/*
		 * 回调传给EthernetSetting
		 */
		mGetStaticInfo.getStaticIp(ipAddr);
		mGetStaticInfo.getStaticNetMask(netMask);
		mGetStaticInfo.getStaticGateway(gateway);
		mGetStaticInfo.getStaticDns1(dns1);
		mGetStaticInfo.getStaticDns2(dns2);
	}

	/*
	 * 返回 指定的 String 是否是 有效的 IP 地址.
	 */
	private boolean isValidIpAddress(String value) {
		int start = 0;
		int end = value.indexOf('.');
		int numBlocks = 0;

		while (start < value.length()) {

			if (-1 == end) {
				end = value.length();
			}

			try {
				int block = Integer.parseInt(value.substring(start, end));
				if ((block > 255) || (block < 0)) {
					Log.w("EthernetIP",
							"isValidIpAddress() : invalid 'block', block = "
									+ block);
					return false;
				}
			} catch (NumberFormatException e) {
				Log.w("EthernetIP", "isValidIpAddress() : e = " + e);
				return false;
			}

			numBlocks++;

			start = end + 1;
			end = value.indexOf('.', start);
		}
		return numBlocks == 4;
	}
    public void checkIPValue()
    {

		boolean enable = false;
		String ipAddr = mIpAddressView.getText().toString();
		String gateway = mIPgateway.getText().toString();
		String dns1 = mdns1.getText().toString();
		String dns2 = mdns2.getText().toString();
		String  netMask = ipnetmask.getText().toString();
     	Pattern pattern = Pattern.compile("(^((\\d|[01]?\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d|[01]?\\d\\d|2[0-4]\\d|25[0-5])$)|^(\\d|[1-2]\\d|3[0-2])$"); /*check subnet mask*/   	
		if (isValidIpAddress(ipAddr) && isValidIpAddress(gateway)
				&& isValidIpAddress(dns1) && (pattern.matcher(netMask).matches())) {
			if (TextUtils.isEmpty(dns2)) { // 为空可以不考虑
				enable = true;
			} else {
				if (isValidIpAddress(dns2)) {
					enable = true;
				} else {
					enable = false;
				}
			}
		} else {
			enable = false;
		}
		getButton(BUTTON_SUBMIT).setEnabled(enable);

    }
	@Override
	public void afterTextChanged(Editable s) {
		checkIPValue();
	//	Log.d("blb", "afterTextChanged");
	}

	@Override
	public void beforeTextChanged(CharSequence s, int start, int count,
			int after) {
		// work done in afterTextChanged
	}

	@Override
	public void onTextChanged(CharSequence s, int start, int before, int count) {
		// work done in afterTextChanged
	}

}
