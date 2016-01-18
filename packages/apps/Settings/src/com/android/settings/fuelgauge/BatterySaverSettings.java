/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.settings.fuelgauge;

import static android.os.PowerManager.ACTION_POWER_SAVE_MODE_CHANGING;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.SwitchPreference;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.util.Log;
import android.widget.Switch;

import com.android.settings.R;
import com.android.settings.SettingsActivity;
import com.android.settings.SettingsPreferenceFragment;
import com.android.settings.Utils;
import com.android.settings.notification.SettingPref;
import com.android.settings.widget.SwitchBar;

public class BatterySaverSettings extends SettingsPreferenceFragment
        implements Preference.OnPreferenceChangeListener, SwitchBar.OnSwitchChangeListener {
    private static final String TAG = "BatterySaverSettings";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);
    private static final String KEY_SHOW_INDICATOR = "show_power_save_indicator";
    private static final String KEY_TURN_ON_AUTOMATICALLY = "turn_on_automatically";
    private static final String KEY_SUB_ITEMS = "sub_items";
    private static final String KEY_LIMIT_CPU = "limit_cpu";
    private static final String KEY_LIMIT_BRIGHTNESS = "limit_brightness";
    private static final String KEY_LIMIT_LOCATION = "limit_location";
    private static final String KEY_LIMIT_NETWORK = "limit_network";
    private static final String KEY_LIMIT_ANIMATION = "limit_animation";

    private static final String PROPERTY_THERMAL_ENABLED = "persist.service.thermal";

    private static final long WAIT_FOR_SWITCH_ANIM = 500;

    private final Handler mHandler = new Handler();
    private final SettingsObserver mSettingsObserver = new SettingsObserver(mHandler);
    private final Receiver mReceiver = new Receiver();

    private Context mContext;
    private boolean mCreated;
    private SwitchBar mSwitchBar;
    private Switch mSwitch;
    private boolean mValidListener;
    private PowerManager mPowerManager;

    private SwitchPreference mShowIndicatorPref;
    private SwitchPreference mEnableWhenLowBatteryPref;
    private SwitchPreference mLimitCPU;
    private SwitchPreference mLimitBrightness;
    private SwitchPreference mLimitLocation;
    private SwitchPreference mLimitNetwork;
    private SwitchPreference mLimitAnimation;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        if (mCreated) {
            mSwitchBar.show();
            return;
        }
        mCreated = true;
        addPreferencesFromResource(R.xml.battery_saver_settings);

        mContext = getActivity();
        mSwitchBar = ((SettingsActivity) mContext).getSwitchBar();
        mSwitch = mSwitchBar.getSwitch();
        mSwitchBar.show();

        mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);

        mShowIndicatorPref = (SwitchPreference) findPreference(KEY_SHOW_INDICATOR);
        mShowIndicatorPref.setOnPreferenceChangeListener(this);
        mEnableWhenLowBatteryPref = (SwitchPreference) findPreference(KEY_TURN_ON_AUTOMATICALLY);
        mEnableWhenLowBatteryPref.setOnPreferenceChangeListener(this);
        mLimitCPU = (SwitchPreference) findPreference(KEY_LIMIT_CPU);
        mLimitCPU.setOnPreferenceChangeListener(this);
        if (!SystemProperties.getBoolean(PROPERTY_THERMAL_ENABLED, false)) {
            final PreferenceCategory parent = (PreferenceCategory) findPreference(KEY_SUB_ITEMS);
            parent.removePreference(mLimitCPU);
            mLimitCPU = null;
        }
        mLimitBrightness = (SwitchPreference) findPreference(KEY_LIMIT_BRIGHTNESS);
        mLimitBrightness.setOnPreferenceChangeListener(this);
        mLimitLocation = (SwitchPreference) findPreference(KEY_LIMIT_LOCATION);
        mLimitLocation.setOnPreferenceChangeListener(this);
        mLimitNetwork = (SwitchPreference) findPreference(KEY_LIMIT_NETWORK);
        mLimitNetwork.setOnPreferenceChangeListener(this);
        mLimitAnimation = (SwitchPreference) findPreference(KEY_LIMIT_ANIMATION);
        mLimitAnimation.setOnPreferenceChangeListener(this);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mSwitchBar.hide();
    }

    @Override
    public void onResume() {
        super.onResume();
        mSettingsObserver.setListening(true);
        mReceiver.setListening(true);
        if (!mValidListener) {
            mSwitchBar.addOnSwitchChangeListener(this);
            mValidListener = true;
        }
        updateSwitch();
        updateBasicItems();
        updateSubItems();
    }

    private void updateBasicItems() {
        if (mShowIndicatorPref != null) {
            mShowIndicatorPref.setChecked(isShowPowerSaveModeIndicator());
        }
        if (mEnableWhenLowBatteryPref != null) {
            mEnableWhenLowBatteryPref.setChecked(getLowPowerModeTriggerLevel() > 0);
        }
    }

    private void updateSubItems() {
        if (mLimitCPU != null) {
            mLimitCPU.setChecked(getLimitOnPowerSaveMode(KEY_LIMIT_CPU));
        }
        if (mLimitBrightness != null) {
            mLimitBrightness.setChecked(getLimitOnPowerSaveMode(KEY_LIMIT_BRIGHTNESS));
        }
        if (mLimitLocation != null) {
            mLimitLocation.setChecked(getLimitOnPowerSaveMode(KEY_LIMIT_LOCATION));
        }
        if (mLimitNetwork != null) {
            mLimitNetwork.setChecked(getLimitOnPowerSaveMode(KEY_LIMIT_NETWORK));
        }
        if (mLimitAnimation != null) {
            mLimitAnimation.setChecked(getLimitOnPowerSaveMode(KEY_LIMIT_ANIMATION));
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        mSettingsObserver.setListening(false);
        mReceiver.setListening(false);
        if (mValidListener) {
            mSwitchBar.removeOnSwitchChangeListener(this);
            mValidListener = false;
        }
    }
    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();

        if (KEY_SHOW_INDICATOR.equals(key)) {
            boolean show = (Boolean) objValue;
            showPowerSaveModeIndicator(show);
        } else if (KEY_TURN_ON_AUTOMATICALLY.equals(key)) {
            boolean auto = (Boolean) objValue;
            setLowPowerModeTriggerLevel(auto ? 15 : 0);

            if (auto) {
                if (isAllSubItemDisabled()) {
                    enableAllSubItem();
                    mHandler.post(mUpdateSubItems);
                }
            }
        } else if (KEY_LIMIT_CPU.equals(key)
                || KEY_LIMIT_BRIGHTNESS.equals(key)
                || KEY_LIMIT_LOCATION.equals(key)
                || KEY_LIMIT_NETWORK.equals(key)
                || KEY_LIMIT_ANIMATION.equals(key)) {
            boolean limit = (Boolean) objValue;
            setLimitOnPowerSaveMode(key, limit);

            if (isAllSubItemDisabled()) {
                onSwitchChanged(mSwitch, false);
                setLowPowerModeTriggerLevel(0);
                if (mEnableWhenLowBatteryPref != null) {
                    mEnableWhenLowBatteryPref.setChecked(false);
                }
            }
        }

        return true;
    }

    @Override
    public void onSwitchChanged(Switch switchView, boolean isChecked) {
        mHandler.removeCallbacks(mStartMode);
        if (isChecked) {
            mHandler.postDelayed(mStartMode, WAIT_FOR_SWITCH_ANIM);
        } else {
            if (DEBUG) Log.d(TAG, "Stopping low power mode from settings");
            trySetPowerSaveMode(false);
        }
    }

    private void trySetPowerSaveMode(boolean mode) {
        if (!mPowerManager.setPowerSaveMode(mode)) {
            if (DEBUG) Log.d(TAG, "Setting mode failed, fallback to current value");
            mHandler.post(mUpdateSwitch);
        } else {
            if (mode) {
                if (isAllSubItemDisabled()) {
                    enableAllSubItem();
                    mHandler.post(mUpdateSubItems);
                }
            }
        }
    }

    private void updateSwitch() {
        final boolean mode = mPowerManager.isPowerSaveMode();
        if (DEBUG) Log.d(TAG, "updateSwitch: isChecked=" + mSwitch.isChecked() + " mode=" + mode);
        if (mode == mSwitch.isChecked()) return;

        // set listener to null so that that code below doesn't trigger onCheckedChanged()
        if (mValidListener) {
            mSwitchBar.removeOnSwitchChangeListener(this);
        }
        mSwitch.setChecked(mode);
        if (mValidListener) {
            mSwitchBar.addOnSwitchChangeListener(this);
        }
    }

    private final Runnable mUpdateSwitch = new Runnable() {
        @Override
        public void run() {
            updateSwitch();
        }
    };

    private final Runnable mStartMode = new Runnable() {
        @Override
        public void run() {
            AsyncTask.execute(new Runnable() {
                @Override
                public void run() {
                    if (DEBUG) Log.d(TAG, "Starting low power mode from settings");
                    trySetPowerSaveMode(true);
                }
            });
        }
    };

    private final class Receiver extends BroadcastReceiver {
        private boolean mRegistered;

        @Override
        public void onReceive(Context context, Intent intent) {
            if (DEBUG) Log.d(TAG, "Received " + intent.getAction());
            mHandler.post(mUpdateSwitch);
        }

        public void setListening(boolean listening) {
            if (listening && !mRegistered) {
                mContext.registerReceiver(this, new IntentFilter(ACTION_POWER_SAVE_MODE_CHANGING));
                mRegistered = true;
            } else if (!listening && mRegistered) {
                mContext.unregisterReceiver(this);
                mRegistered = false;
            }
        }
    }

    private final class SettingsObserver extends ContentObserver {
        private final Uri LOW_POWER_MODE_TRIGGER_LEVEL_URI
                = Global.getUriFor(Global.LOW_POWER_MODE_TRIGGER_LEVEL);

        public SettingsObserver(Handler handler) {
            super(handler);
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            if (LOW_POWER_MODE_TRIGGER_LEVEL_URI.equals(uri)) {
                int level = getLowPowerModeTriggerLevel();
                mEnableWhenLowBatteryPref.setChecked(level > 0);
            }
        }

        public void setListening(boolean listening) {
            final ContentResolver cr = getContentResolver();
            if (listening) {
                cr.registerContentObserver(LOW_POWER_MODE_TRIGGER_LEVEL_URI, false, this);
            } else {
                cr.unregisterContentObserver(this);
            }
        }
    }

    private boolean isShowPowerSaveModeIndicator() {
        final ContentResolver cr = getContentResolver();
        final boolean val = Global.getInt(cr, Global.SHOW_LOW_POWER_MODE_INDICATOR, 1) != 0;
        return val;
    }

    private void showPowerSaveModeIndicator(boolean show) {
        final ContentResolver cr = getContentResolver();
        Settings.Global.putInt(cr, Global.SHOW_LOW_POWER_MODE_INDICATOR, show ? 1 : 0);
    }

    private int getLowPowerModeTriggerLevel() {
        final ContentResolver cr = getContentResolver();
        final int val = Global.getInt(cr, Global.LOW_POWER_MODE_TRIGGER_LEVEL, 0);
        return val;
    }

    private void setLowPowerModeTriggerLevel(int val) {
        final ContentResolver cr = getContentResolver();
        Settings.Global.putInt(cr, Global.LOW_POWER_MODE_TRIGGER_LEVEL, val);
    }

    private boolean getLimitOnPowerSaveMode(String key) {
        final ContentResolver cr = getContentResolver();
        final boolean val;
        switch (key) {
            case KEY_LIMIT_CPU:
                val = Settings.Global.getInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_CPU, 1) != 0;
                break;
            case KEY_LIMIT_BRIGHTNESS:
                val = Settings.Global.getInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_BRIGHTNESS, 1) != 0;
                break;
            case KEY_LIMIT_LOCATION:
                val = Settings.Secure.getInt(cr, Settings.Secure.LOW_POWER_MODE_LIMIT_LOCATION, 1) != 0;
                break;
            case KEY_LIMIT_NETWORK:
                val = Settings.Global.getInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_NETWORK, 1) != 0;
                break;
            case KEY_LIMIT_ANIMATION:
                val = Settings.Global.getInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_ANIMATION, 1) != 0;
                break;
            default:
                Log.e(TAG, "Not found settings for " + key);
                val = false;
                break;
        }
        return val;
    }

    private void setLimitOnPowerSaveMode(String key, boolean limit) {
        final ContentResolver cr = getContentResolver();
        switch (key) {
            case KEY_LIMIT_CPU:
                Settings.Global.putInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_CPU, limit ? 1 : 0);
                break;
            case KEY_LIMIT_BRIGHTNESS:
                Settings.Global.putInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_BRIGHTNESS, limit ? 1 : 0);
                break;
            case KEY_LIMIT_LOCATION:
                Settings.Secure.putInt(cr, Settings.Secure.LOW_POWER_MODE_LIMIT_LOCATION, limit ? 1 : 0);
                break;
            case KEY_LIMIT_NETWORK:
                Settings.Global.putInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_NETWORK, limit ? 1 : 0);
                break;
            case KEY_LIMIT_ANIMATION:
                Settings.Global.putInt(cr, Settings.Global.LOW_POWER_MODE_LIMIT_ANIMATION, limit ? 1 : 0);
                break;
            default:
                Log.e(TAG, "Not found settings for " + key);
                break;
        }
    }

    private boolean isAllSubItemDisabled() {
        if ((mLimitCPU == null || !getLimitOnPowerSaveMode(KEY_LIMIT_CPU))
                && !getLimitOnPowerSaveMode(KEY_LIMIT_BRIGHTNESS)
                && !getLimitOnPowerSaveMode(KEY_LIMIT_LOCATION)
                && !getLimitOnPowerSaveMode(KEY_LIMIT_NETWORK)
                && !getLimitOnPowerSaveMode(KEY_LIMIT_ANIMATION)) {
            return true;
        }
        return false;
    }

    private void enableAllSubItem() {
        if (mLimitCPU != null) {
            setLimitOnPowerSaveMode(KEY_LIMIT_CPU, true);
        }
        if (mLimitBrightness != null) {
            setLimitOnPowerSaveMode(KEY_LIMIT_BRIGHTNESS, true);
        }
        if (mLimitLocation != null) {
            setLimitOnPowerSaveMode(KEY_LIMIT_LOCATION, true);
        }
        if (mLimitNetwork != null) {
            setLimitOnPowerSaveMode(KEY_LIMIT_NETWORK, true);
        }
        if (mLimitAnimation != null) {
            setLimitOnPowerSaveMode(KEY_LIMIT_ANIMATION, true);
        }
    }

   private final Runnable mUpdateSubItems = new Runnable() {
        @Override
        public void run() {
            updateSubItems();
        }
    };
}
