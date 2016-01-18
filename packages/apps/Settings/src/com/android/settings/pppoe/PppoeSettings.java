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

package com.android.settings.pppoe;

import android.app.ActivityManagerNative;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Context;
import android.content.ContentUris;
import android.database.ContentObserver;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.view.IWindowManager;
import android.view.Surface;
import android.net.pppoe.PppoeManager;
import android.net.EthernetManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.widget.Toast;
import android.database.Cursor;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.R;

import java.util.ArrayList;
import java.util.Formatter;
import android.util.Log;

public class PppoeSettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener {
    private static final String TAG = "PppoeSettings";
    public static final boolean DEBUG = true;
    private static void LOG(String msg) {
        if ( DEBUG ) {
            Log.d(TAG, msg);
        }
    }
        
    private static final String REFRESH_PPPOE_CHECKBOX_ACTION = "android.settings.pppoe.REFRESH_PPPOE_CHECKBOX_ACTION";
    private static final String KEY_PPPOE_ENABLER = "pppoe_connect";
    private static final String KEY_PPPOE_PHY_IFACE = "physical_interface";
    private static final String KEY_PPPOE_ADD_ACCOUNT = "pppoe_add_account";
    private static final String DEFAULT_PHY_IFACE = "ethernet";

    private CheckBoxPreference mEnablePppoeCheckBox;
    private ListPreference mPppoePhyIface;
    private PppoeManager mPppoeMgr;
    private int mPppoeState;
    private String mIface = DEFAULT_PHY_IFACE;
    private EthernetManager mEthMgr;

    private static final int ID_INDEX = 0;
    private static final int NAME_INDEX = 1;
    private static final int USER_INDEX = 2;
    private static final int DNS1_INDEX = 3;
    private static final int DNS2_INDEX = 4;
    private static final int PASSWORD_INDEX = 5;
    private static final Uri PREFERRED_PPPOE_URI = Uri.parse("content://pppoe/accounts/preferaccount");
    private static final String DEFAULT_SORT_ORDER = "name ASC";

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if ( action.equals(PppoeManager.PPPOE_STATE_CHANGED_ACTION) ) {
                mPppoeState = intent.getIntExtra(PppoeManager.EXTRA_PPPOE_STATE, PppoeManager.PPPOE_STATE_DISCONNECTED);
                LOG("mPppoeState = " + mPppoeState);
            } else if (action.equals(REFRESH_PPPOE_CHECKBOX_ACTION)) {
                refreshEnablePppoeCheckBox();
            }
            refreshEnablePppoeCheckBox();
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.pppoe_prefs);

        mEnablePppoeCheckBox = (CheckBoxPreference)findPreference(KEY_PPPOE_ENABLER);
        mEnablePppoeCheckBox.setOnPreferenceChangeListener(this);
        mPppoePhyIface = (ListPreference) findPreference(KEY_PPPOE_PHY_IFACE);
        mPppoePhyIface.setOnPreferenceChangeListener(this);

        mPppoeMgr = (PppoeManager) getSystemService(Context.PPPOE_SERVICE);
        if (mPppoeMgr == null) {
            Log.e(TAG, "get pppoe manager failed");
            return;
        }
        mEthMgr = (EthernetManager) getSystemService(Context.ETHERNET_SERVICE);

        IntentFilter filter = new IntentFilter();
        filter.addAction(PppoeManager.PPPOE_STATE_CHANGED_ACTION);

//        filter.addAction(EthernetDataTracker.ETHERNET_STATE_CHANGED_ACTION);
        filter.addAction(REFRESH_PPPOE_CHECKBOX_ACTION);
        getActivity().registerReceiver(mReceiver, filter);
        mIface = mPppoePhyIface.getValue();
        if(mIface == null) {
            mIface = DEFAULT_PHY_IFACE;
            mPppoePhyIface.setValue(mIface);
        }
/*
        mPppoeState = mPppoeMgr.getPppoeState();
        mPppoePhyIface.setSummary(mIface);
*/
       // refreshEnablePppoeCheckBox();
    }

    @Override
    public void onResume() {
        mPppoeState = mPppoeMgr.getPppoeState();
        mPppoePhyIface.setSummary(mIface);
        refreshEnablePppoeCheckBox();
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    getActivity().unregisterReceiver(mReceiver);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    private void refreshEnablePppoeCheckBox() {
        String summary = null;

        LOG("mPppoeState:"+mPppoeState);

        if ( PppoeManager.PPPOE_STATE_DISCONNECTED == mPppoeState ) {
            mEnablePppoeCheckBox.setChecked(false);
            mEnablePppoeCheckBox.setEnabled(true);
            mEnablePppoeCheckBox.setSummary(R.string.pppoe_quick_toggle_summary);
        } else if ( PppoeManager.PPPOE_STATE_DISCONNECTING == mPppoeState ) {
            mEnablePppoeCheckBox.setEnabled(false);
            mEnablePppoeCheckBox.setSummary(R.string.pppoe_disconnecting);
        } else if ( PppoeManager.PPPOE_STATE_CONNECTING == mPppoeState ) {
            mEnablePppoeCheckBox.setEnabled(false);
            mEnablePppoeCheckBox.setSummary(R.string.pppoe_connecting);
        } else if ( PppoeManager.PPPOE_STATE_CONNECTED == mPppoeState ) {
            mEnablePppoeCheckBox.setChecked(true);
            mEnablePppoeCheckBox.setSummary(R.string.pppoe_connected);
            mEnablePppoeCheckBox.setEnabled(true);
        }
    }

    public boolean onPreferenceChange(Preference preference, Object newValue) {
        boolean result = true;
        String key = preference.getKey();

        if ( null == key ) {
            return true;
        } else if ( key.equals(KEY_PPPOE_ENABLER) ) {
            mEnablePppoeCheckBox.setEnabled(false);
            setPppoeEnabled((Boolean) newValue);
        } else if(key.equals(KEY_PPPOE_PHY_IFACE)) {
            mPppoePhyIface.setValue((String)newValue);
            mIface = (String)newValue;
            mPppoePhyIface.setSummary(mIface);
        }
        return result;
    }

    private void sendRefreshCheckboxBroadcast() {
        Intent intent = new Intent(REFRESH_PPPOE_CHECKBOX_ACTION);
        getActivity().sendBroadcast(intent);
    }

    private void setPppoeEnabled(final boolean enable) {
        LOG("setPppoeEnabled:"+enable);

        if (enable) {
            String[] projection = new String[] { "_id",
                                                 "name",
                                                 "user",
                                                 "dns1",
                                                 "dns2",
                                                 "password", };

            String user = null;
            String dns1 = null;
            String dns2 = null;
            String password = null;

            Cursor c = /*getActivity().managedQuery*/getContentResolver().query(PREFERRED_PPPOE_URI,
            projection, null, null,
            DEFAULT_SORT_ORDER);

            LOG("c.getCount="+c.getCount());

            if(c.getCount() == 0) {
                Toast.makeText(getActivity(),
                                R.string.pppoe_invalid_account,
                                Toast.LENGTH_SHORT).show();
                sendRefreshCheckboxBroadcast();
                return;
            }

            c.moveToFirst();
            user = c.getString(USER_INDEX);
            dns1= c.getString(DNS1_INDEX);
            dns2= c.getString(DNS2_INDEX);
            password = c.getString(PASSWORD_INDEX);
            c.close();

            LOG("user="+user);
            if(user.equals("") || password.equals("")) {
                Toast.makeText(getActivity(),
                                R.string.pppoe_invalid_account,
                                Toast.LENGTH_SHORT).show();
                sendRefreshCheckboxBroadcast();
                return;
            }

            if (mPppoeMgr == null) {
                LOG("mPppoeMgr == null");
                sendRefreshCheckboxBroadcast();
                return;
            }

            String iface;
            if (mIface.equals("wifi")) {
                iface = "wlan0";
                ConnectivityManager connectivity =
                         (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
                if (connectivity == null) {
                    LOG("connectivity manager is null");
                    sendRefreshCheckboxBroadcast();
                    return;
                }

                NetworkInfo info = connectivity.getActiveNetworkInfo();
                if (info == null || !info.isConnected() ||
                    info.getType() != ConnectivityManager.TYPE_WIFI) {
                    Toast.makeText(getActivity(),
                                    R.string.pppoe_connect_wifi_first,
                                    Toast.LENGTH_SHORT).show();
                    sendRefreshCheckboxBroadcast();
                    return;
                }
            } else if (mIface.equals("ethernet")){
                iface = "eth0";
/*
                if((mEthMgr.getEthernetIfaceState() != EthernetDataTracker.ETHER_IFACE_STATE_UP) ||
                (mEthMgr.getEthernetCarrierState() != 1)) {
                    Toast.makeText(getActivity(),
                                    R.string.pppoe_connect_eth_first,
                                    Toast.LENGTH_SHORT).show();
                    sendRefreshCheckboxBroadcast();
                    return;
                }*/
            } else {
                Log.e(TAG, "physical interface abnormal:"+mIface);
                sendRefreshCheckboxBroadcast();
                return;
            }

            mPppoeMgr.setupPppoe(user, iface, dns1, dns2, password);
            mPppoeMgr.startPppoe();
        } else {
            if(mPppoeState == PppoeManager.PPPOE_STATE_CONNECTED) {
                mPppoeMgr.stopPppoe();
            }
        }
    }
}

