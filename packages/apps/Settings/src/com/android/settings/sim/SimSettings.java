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

package com.android.settings.sim;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.OvalShape;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceScreen;
import android.provider.SearchIndexableResource;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.PhoneNumberUtils;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.TextView;
import android.view.ViewGroup;
import android.widget.AdapterView;
import com.android.settings.RestrictedSettingsFragment;
import com.android.settings.Utils;
import com.android.settings.search.BaseSearchIndexProvider;
import com.android.settings.search.Indexable;
import com.android.settings.R;

import com.android.internal.telephony.TelephonyIntents;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Iterator;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.preference.SwitchPreference;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

public class SimSettings extends RestrictedSettingsFragment implements Indexable {
    private static final String TAG = "SimSettings";
    private static final boolean DBG = Log.isLoggable(TAG, Log.DEBUG);

    private static final String DISALLOW_CONFIG_SIM = "no_config_sim";
    private static final String SIM_CARD_CATEGORY = "sim_cards";
    private static final String KEY_CELLULAR_DATA = "sim_cellular_data";
    private static final String KEY_CALLS = "sim_calls";
    private static final String KEY_SMS = "sim_sms";
    private static final String KEY_DATA_SWITCH = "sim_data_switch";
    private static final String KEY_ACTIVITIES = "activities";
    private static final int ID_INDEX = 0;
    private static final int NAME_INDEX = 1;
    private static final int APN_INDEX = 2;
    private static final int PROXY_INDEX = 3;
    private static final int PORT_INDEX = 4;
    private static final int USER_INDEX = 5;
    private static final int SERVER_INDEX = 6;
    private static final int PASSWORD_INDEX = 7;
    private static final int MMSC_INDEX = 8;
    private static final int MCC_INDEX = 9;
    private static final int MNC_INDEX = 10;
    private static final int NUMERIC_INDEX = 11;
    private static final int MMSPROXY_INDEX = 12;
    private static final int MMSPORT_INDEX = 13;
    private static final int AUTH_TYPE_INDEX = 14;
    private static final int TYPE_INDEX = 15;
    private static final int PROTOCOL_INDEX = 16;
    private static final int CARRIER_ENABLED_INDEX = 17;
    private static final int BEARER_INDEX = 18;
    private static final int ROAMING_PROTOCOL_INDEX = 19;
    private static final int MVNO_TYPE_INDEX = 20;
    private static final int MVNO_MATCH_DATA_INDEX = 21;
    private static final int DATA_PICK = 0;
    private static final int CALLS_PICK = 1;
    private static final int SMS_PICK = 2;

    private static final int REQUEST_SET_DEFAULT_DATA = 10;
    private static final int DIALOG_SWITCHING_DATA = 1001;

    private HashMap<Integer, MobilePhoneStateListener> mPhoneStateListeners = new HashMap<Integer, MobilePhoneStateListener>();

    /**
     * By UX design we use only one Subscription Information(SubInfo) record per SIM slot.
     * mAvalableSubInfos is the list of SubInfos we present to the user.
     * mSubInfoList is the list of all SubInfos.
     * mSelectableSubInfos is the list of SubInfos that a user can select for data, calls, and SMS.
     */
    private List<SubscriptionInfo> mAvailableSubInfos = null;
    private List<SubscriptionInfo> mSubInfoList = null;
    private List<SubscriptionInfo> mSelectableSubInfos = null;

    private SubscriptionInfo mCellularData = null;
    private SubscriptionInfo mCalls = null;
    private SubscriptionInfo mSMS = null;

    private PreferenceScreen mSimCards = null;

    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    private Utils mUtils;

    private boolean mShowWaitingDialogNeeded;
    //indicate whether data sub switch or data on/off switch
    private int mSwitchingType = -1;
    private int mPreDefaultDataSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    private int mCurDefaultDataSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;

    public SimSettings() {
        super(DISALLOW_CONFIG_SIM);
    }

    @Override
    public void onCreate(final Bundle bundle) {
        super.onCreate(bundle);

        mSubscriptionManager = SubscriptionManager.from(getActivity());

        mTelephonyManager =
            (TelephonyManager) getActivity().getSystemService(Context.TELEPHONY_SERVICE);
        if (mSubInfoList == null) {
            mSubInfoList = mSubscriptionManager.getActiveSubscriptionInfoList();
            // FIXME: b/18385348, needs to handle null from getActiveSubscriptionInfoList
        }
        if (DBG) log("[onCreate] mSubInfoList=" + mSubInfoList);

        createPreferences();

        SimBootReceiver.cancelNotification(getActivity());
    }

    private void createPreferences() {
        addPreferencesFromResource(R.xml.sim_settings);

        mSimCards = (PreferenceScreen)findPreference(SIM_CARD_CATEGORY);

        final Preference simPref = findPreference(KEY_DATA_SWITCH);
        simPref.setOnPreferenceChangeListener(mOnPreferenceChangeListener);

        final int numSlots = mTelephonyManager.getSimCount();
        mAvailableSubInfos = new ArrayList<SubscriptionInfo>(numSlots);
        mSelectableSubInfos = new ArrayList<SubscriptionInfo>();
        updatePreferences();
        updateActivitesCategory();
    }

    private void updatePreferences() {
        clearSimPreference();

        final int numSlots = mTelephonyManager.getSimCount();
        if (mAvailableSubInfos != null) {
            mAvailableSubInfos.clear();
            mAvailableSubInfos = null;
        }
        mAvailableSubInfos = new ArrayList<SubscriptionInfo>(numSlots);

        if (mSelectableSubInfos != null) {
            mSelectableSubInfos.clear();
            unregisterListeners();
        }

        for (int i = 0; i < numSlots; ++i) {
            final SubscriptionInfo sir = Utils.findRecordBySlotId(getActivity(), i);
            SimPreference simPreference = new SimPreference(getActivity(), sir, i);
            simPreference.setOrder(i-numSlots);
            mSimCards.addPreference(simPreference);
            mAvailableSubInfos.add(sir);
            if (sir != null) {
                startListenForPhoneState(sir.getSubscriptionId());
                mSelectableSubInfos.add(sir);
            }
        }
    }

    private void updateAvailableSubInfos(){
        final int numSlots = mTelephonyManager.getSimCount();

        mAvailableSubInfos = new ArrayList<SubscriptionInfo>(numSlots);
        for (int i = 0; i < numSlots; ++i) {
            final SubscriptionInfo sir = Utils.findRecordBySlotId(getActivity(), i);
            mAvailableSubInfos.add(sir);
            if (sir != null) {
            }
        }
    }

    private void updateAllOptions() {
        updateSimSlotValues();
        updateActivitesCategory();
    }

    private void updateSimSlotValues() {
        mSubscriptionManager.getAllSubscriptionInfoList();

        final int prefSize = mSimCards.getPreferenceCount();
        for (int i = 0; i < prefSize; ++i) {
            Preference pref = mSimCards.getPreference(i);
            if (pref instanceof SimPreference) {
                ((SimPreference)pref).update();
            }
        }
    }

    private void updateActivitesCategory() {
        updateCellularDataValues();
        updateDataSwitchValues();
        updateCallValues();
        updateSmsValues();
    }

    private void updateSmsValues() {
        final Preference simPref = findPreference(KEY_SMS);
        final SubscriptionInfo sir = Utils.findRecordBySubId(getActivity(),
                mSubscriptionManager.getDefaultSmsSubId());
        simPref.setTitle(R.string.sms_messages_title);
        if (DBG) log("[updateSmsValues] mSubInfoList=" + mSubInfoList);

        if (sir != null) {
            simPref.setSummary(sir.getDisplayName());
        } else if (sir == null) {
            simPref.setSummary(R.string.sim_calls_ask_first_prefs_title);
        }
        simPref.setEnabled(mSelectableSubInfos.size() >= 1);
    }

    private void updateDataSwitchValues(){
        final Preference simPref = findPreference(KEY_DATA_SWITCH);
        if(mSelectableSubInfos.size() >= 1){
            simPref.setEnabled(true);
            if(mTelephonyManager.getDataEnabled()){
                ((SwitchPreference) simPref).setChecked(true);
            } else {
                ((SwitchPreference) simPref).setChecked(false);
            }
        }
        else{
            simPref.setEnabled(false);
            ((SwitchPreference) simPref).setChecked(false);
        }
        //int dataSub = mSubscriptionManager.getDefaultDataSubId();
        //boolean defaultDataSelected = mSubscriptionManager.isValidSubscriptionId(dataSub);
        //if (DBG) log("[updateDataSwitchValues] dataSub=" + dataSub + ",mTelephonyManager.getDataEnabled()=" + mTelephonyManager.getDataEnabled());
        //simPref.setEnabled(defaultDataSelected);
        if (DBG) log("[updateDataSwitchValues] mSelectableSubInfos.size() >= 1=" + (mSelectableSubInfos.size() >= 1));
    }

    private void updateCellularDataValues() {
        final Preference simPref = findPreference(KEY_CELLULAR_DATA);
        final SubscriptionInfo sir = Utils.findRecordBySubId(getActivity(),
                mSubscriptionManager.getDefaultDataSubId());
        simPref.setTitle(R.string.cellular_data_title);
        if (DBG) log("[updateCellularDataValues] mSubInfoList=" + mSubInfoList);

        if (sir != null) {
            simPref.setSummary(sir.getDisplayName());
        } else if (sir == null) {
            simPref.setSummary(R.string.sim_selection_required_pref);
        }
        simPref.setEnabled(mSelectableSubInfos.size() >= 1);
    }

    private void updateCallValues() {
        final Preference simPref = findPreference(KEY_CALLS);
        final TelecomManager telecomManager = TelecomManager.from(getActivity());
        final PhoneAccountHandle phoneAccount =
            telecomManager.getUserSelectedOutgoingPhoneAccount();

        simPref.setTitle(R.string.calls_title);
        simPref.setSummary(phoneAccount == null
                ? getResources().getString(R.string.sim_calls_ask_first_prefs_title)
                : (String)telecomManager.getPhoneAccount(phoneAccount).getLabel());
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mShowWaitingDialogNeeded) {
            waitingForDataSwitch();
        }
        mShowWaitingDialogNeeded = false;

        mSubInfoList = mSubscriptionManager.getActiveSubscriptionInfoList();
        // FIXME: b/18385348, needs to handle null from getActiveSubscriptionInfoList
        if (DBG) log("[onResme] mSubInfoList=" + mSubInfoList);

        updateAvailableSubInfos();

        if (getActivity() != null) {
            final IntentFilter filter = new IntentFilter(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
            filter.addAction(TelephonyIntents.ACTION_SUBINFO_RECORD_UPDATED);
            filter.addAction(TelephonyIntents.ACTION_SUBINFO_CONTENT_CHANGE);
            filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
            filter.addAction(TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED);
            getActivity().registerReceiver(mReceiver, filter);
        }

        updateAllOptions();
    }

     @Override
    public void onPause() {
        super.onPause();
        if (getActivity() != null) {
            getActivity().unregisterReceiver(mReceiver);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        unregisterListeners();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (DBG) log("onActivityResult, requestCode = " + requestCode
                                   + ", resultCode = " + resultCode
                                   + ", data = " + data);
        if (requestCode == REQUEST_SET_DEFAULT_DATA &&
                 resultCode == Activity.RESULT_OK) {
            mShowWaitingDialogNeeded = true;
            mSwitchingType = TYPE_DATA_SUB_SWITCH;
            if (data != null) {
                mPreDefaultDataSubId = data.getIntExtra(SimDialogActivity.SUB_ID_EXTRA,
                        SubscriptionManager.INVALID_SUBSCRIPTION_ID);
            }
            mCurDefaultDataSubId = SubscriptionManager.getDefaultDataSubId();
            if (DBG) log("onActivityResult, mPreDefaultDataSubId = " + mPreDefaultDataSubId
                                   + ", mCurDefaultDataSubId = " + mCurDefaultDataSubId);
        }
    }

    private OnPreferenceChangeListener mOnPreferenceChangeListener = new OnPreferenceChangeListener() {

        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue) {
            if (DBG) log("onPreferenceChange, newValue=" + (boolean)newValue + ",mSwitchingType=" + mSwitchingType);
            if (mSwitchingType == TYPE_DATA_ONOFF_SWITCH) {
                log("onPreferenceChange, data on/off is changing, ignore this! ");
                return false;
            }
            mSwitchingType = TYPE_DATA_ONOFF_SWITCH;
            waitingForDataSwitch();
            boolean dataEnable = (boolean)newValue;
            mTelephonyManager.setDataEnabled(dataEnable);
            //data state is already as what we want
            //Need a little delay to wait framework finished
            if (isDataStateExpected(dataEnable)) {
                mHandler.sendEmptyMessageDelayed(MESSAGE_DATA_ONOFF_COMPLETED, DISMISS_DIALOG_DELAYED);
            }
            return true;
        }

    };

    private boolean isDataStateExpected(boolean enabled) {
        //If wifi is connected, mobile data is disconnected always.
        ConnectivityManager connMgr = (ConnectivityManager) getActivity()
                        .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo wifiNetworkInfo = connMgr
                    .getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (wifiNetworkInfo != null && wifiNetworkInfo.isConnected()) {
            log("isDataStateExpected, wifi is connected!");
            return true;
        }
        final int dataState = mTelephonyManager.getDataState();
        if (DBG) log("isDataStateExpected, enabled = " + enabled + ", dataState = " + dataState);
        if (enabled) {
            return dataState == TelephonyManager.DATA_CONNECTED;
        } else {
            return dataState == TelephonyManager.DATA_DISCONNECTED ||
                   dataState == TelephonyManager.DATA_UNKNOWN;
        }
    }

    @Override
    public boolean onPreferenceTreeClick(final PreferenceScreen preferenceScreen,
            final Preference preference) {
        final Context context = getActivity();
        Intent intent = new Intent(context, SimDialogActivity.class);
        //remove this flag because SimDialogActivity is in other task, which will
        //cause callback onResultActivity not work as excepted
        //intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        if (preference instanceof SimPreference) {
            ((SimPreference)preference).createEditDialog((SimPreference)preference);
        } else if (findPreference(KEY_CELLULAR_DATA) == preference) {
            intent.putExtra(SimDialogActivity.DIALOG_TYPE_KEY, SimDialogActivity.DATA_PICK);
            startActivityForResult(intent, REQUEST_SET_DEFAULT_DATA);
        } else if (findPreference(KEY_CALLS) == preference) {
            intent.putExtra(SimDialogActivity.DIALOG_TYPE_KEY, SimDialogActivity.CALLS_PICK);
            context.startActivity(intent);
        } else if (findPreference(KEY_SMS) == preference) {
            intent.putExtra(SimDialogActivity.DIALOG_TYPE_KEY, SimDialogActivity.SMS_PICK);
            intent.putExtra(SimDialogActivity.ITEM_ASK_SUPPORTED, true);
            context.startActivity(intent);
        }

        return true;
    }

    private class SimPreference extends Preference{
        private SubscriptionInfo mSubInfoRecord;
        private int mSlotId;
        private int[] mTintArr;
        Context mContext;
        private String[] mColorStrings;
        private int mTintSelectorPos;

        public SimPreference(Context context, SubscriptionInfo subInfoRecord, int slotId) {
            super(context);

            mContext = context;
            mSubInfoRecord = subInfoRecord;
            mSlotId = slotId;
            setKey("sim" + mSlotId);
            update();
            mTintArr = context.getResources().getIntArray(com.android.internal.R.array.sim_colors);
            mColorStrings = context.getResources().getStringArray(R.array.color_picker);
            mTintSelectorPos = 0;
        }

        public void update() {
            final Resources res = getResources();

            setTitle(String.format(getResources()
                    .getString(R.string.sim_editor_title), (mSlotId + 1)));
            if (mSubInfoRecord != null) {
                if (TextUtils.isEmpty(getPhoneNumber(mSubInfoRecord))) {
                   setSummary(mSubInfoRecord.getDisplayName());
                } else {
                    setSummary(mSubInfoRecord.getDisplayName() + " - " +
                            getPhoneNumber(mSubInfoRecord));
                    setEnabled(true);
                }
                setIcon(new BitmapDrawable(res, (mSubInfoRecord.createIconBitmap(mContext))));
            } else {
                setSummary(R.string.sim_slot_empty);
                setFragment(null);
                setEnabled(false);
            }
        }

        public SubscriptionInfo getSubInfoRecord() {
            return mSubInfoRecord;
        }

        public void createEditDialog(SimPreference simPref) {
            final Resources res = getResources();

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

            final View dialogLayout = getActivity().getLayoutInflater().inflate(
                    R.layout.multi_sim_dialog, null);
            builder.setView(dialogLayout);

            EditText nameText = (EditText)dialogLayout.findViewById(R.id.sim_name);
            nameText.setText(mSubInfoRecord.getDisplayName());

            final Spinner tintSpinner = (Spinner) dialogLayout.findViewById(R.id.spinner);
            SelectColorAdapter adapter = new SelectColorAdapter(getContext(),
                     R.layout.settings_color_picker_item, mColorStrings);
            adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            tintSpinner.setAdapter(adapter);

            for (int i = 0; i < mTintArr.length; i++) {
                if (mTintArr[i] == mSubInfoRecord.getIconTint()) {
                    tintSpinner.setSelection(i);
                    mTintSelectorPos = i;
                    break;
                }
            }

            tintSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view,
                    int pos, long id){
                    tintSpinner.setSelection(pos);
                    mTintSelectorPos = pos;
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                }
            });

            TextView numberView = (TextView)dialogLayout.findViewById(R.id.number);
            final String rawNumber = getPhoneNumber(mSubInfoRecord);
            if (TextUtils.isEmpty(rawNumber)) {
                numberView.setText(res.getString(com.android.internal.R.string.unknownName));
            } else {
                numberView.setText(PhoneNumberUtils.formatNumber(rawNumber));
            }

            String simCarrierName = mTelephonyManager.getSimOperatorNameForSubscription(mSubInfoRecord
                        .getSubscriptionId());
            TextView carrierView = (TextView)dialogLayout.findViewById(R.id.carrier);
            carrierView.setText(!TextUtils.isEmpty(simCarrierName) ? simCarrierName :
                    getContext().getString(com.android.internal.R.string.unknownName));

            builder.setTitle(String.format(res.getString(R.string.sim_editor_title),
                    (mSubInfoRecord.getSimSlotIndex() + 1)));

            builder.setPositiveButton(R.string.okay, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int whichButton) {
                    final EditText nameText = (EditText)dialogLayout.findViewById(R.id.sim_name);

                    String displayName = nameText.getText().toString();
                    int subId = mSubInfoRecord.getSubscriptionId();
                    mSubInfoRecord.setDisplayName(displayName);
                    mSubscriptionManager.setDisplayName(displayName, subId,
                            SubscriptionManager.NAME_SOURCE_USER_INPUT);
                    Utils.findRecordBySubId(getActivity(), subId).setDisplayName(displayName);

                    final int tintSelected = tintSpinner.getSelectedItemPosition();
                    int subscriptionId = mSubInfoRecord.getSubscriptionId();
                    int tint = mTintArr[tintSelected];
                    mSubInfoRecord.setIconTint(tint);
                    mSubscriptionManager.setIconTint(tint, subscriptionId);
                    Utils.findRecordBySubId(getActivity(), subscriptionId).setIconTint(tint);

                    updateAllOptions();
                    update();
                }
            });

            builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int whichButton) {
                    dialog.dismiss();
                }
            });

            builder.create().show();
        }

        private class SelectColorAdapter extends ArrayAdapter<CharSequence> {
            private Context mContext;
            private int mResId;

            public SelectColorAdapter(
                Context context, int resource, String[] arr) {
                super(context, resource, arr);
                mContext = context;
                mResId = resource;
            }

            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                LayoutInflater inflater = (LayoutInflater)
                    mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

                View rowView;
                final ViewHolder holder;
                Resources res = getResources();
                int iconSize = res.getDimensionPixelSize(R.dimen.color_swatch_size);
                int strokeWidth = res.getDimensionPixelSize(R.dimen.color_swatch_stroke_width);

                if (convertView == null) {
                    // Cache views for faster scrolling
                    rowView = inflater.inflate(mResId, null);
                    holder = new ViewHolder();
                    ShapeDrawable drawable = new ShapeDrawable(new OvalShape());
                    drawable.setIntrinsicHeight(iconSize);
                    drawable.setIntrinsicWidth(iconSize);
                    drawable.getPaint().setStrokeWidth(strokeWidth);
                    holder.label = (TextView) rowView.findViewById(R.id.color_text);
                    holder.icon = (ImageView) rowView.findViewById(R.id.color_icon);
                    holder.swatch = drawable;
                    rowView.setTag(holder);
                } else {
                    rowView = convertView;
                    holder = (ViewHolder) rowView.getTag();
                }

                holder.label.setText(getItem(position));
                holder.swatch.getPaint().setColor(mTintArr[position]);
                holder.swatch.getPaint().setStyle(Paint.Style.FILL_AND_STROKE);
                holder.icon.setVisibility(View.VISIBLE);
                holder.icon.setImageDrawable(holder.swatch);
                return rowView;
            }

            @Override
            public View getDropDownView(int position, View convertView, ViewGroup parent) {
                View rowView = getView(position, convertView, parent);
                final ViewHolder holder = (ViewHolder) rowView.getTag();

                if (mTintSelectorPos == position) {
                    holder.swatch.getPaint().setStyle(Paint.Style.FILL_AND_STROKE);
                } else {
                    holder.swatch.getPaint().setStyle(Paint.Style.STROKE);
                }
                holder.icon.setVisibility(View.VISIBLE);
                return rowView;
            }

            private class ViewHolder {
                TextView label;
                ImageView icon;
                ShapeDrawable swatch;
            }
        }


    }

    // Returns the line1Number. Line1number should always be read from TelephonyManager since it can
    // be overridden for display purposes.
    private String getPhoneNumber(SubscriptionInfo info) {
        return mTelephonyManager.getLine1NumberForSubscriber(info.getSubscriptionId());
    }

    private static void log(String s) {
        Log.d(TAG, s);
    }

    /**
     * For search
     */
    public static final SearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider() {
                @Override
                public List<SearchIndexableResource> getXmlResourcesToIndex(Context context,
                        boolean enabled) {
                    ArrayList<SearchIndexableResource> result =
                            new ArrayList<SearchIndexableResource>();

                    if (Utils.showSimCardTile(context)) {
                        SearchIndexableResource sir = new SearchIndexableResource(context);
                        sir.xmlResId = R.xml.sim_settings;
                        result.add(sir);
                    }

                    return result;
                }
            };

    private void clearSimPreference() {
        int prefSize = mSimCards.getPreferenceCount();
        for (int i = 0; i < prefSize;) {
            Preference pref = mSimCards.getPreference(i);
            if (pref instanceof SimPreference) {
                mSimCards.removePreference(pref);
                prefSize--;
            } else {
                i++;
            }
        }
    }

    private static final int MESSAGE_UPDATE_VIEW = 102;
    private static final int MESSAGE_UPDATE_DEFAULT_DATA = 103;
    private static final int MESSAGE_DISMISS_DIALOG = 104;
    private static final int MESSAGE_CHECK_SWITCH_COMPLETED = 105;
    private static final int MESSAGE_CHECK_DATA_ONOFF_COMPLETED = 106;
    private static final int MESSAGE_DATA_ONOFF_COMPLETED = 107;

    private static final int MAX_DIALOG_SHOWING_DELAYED = 30 * 1000;
    private static final int DISMISS_DIALOG_DELAYED = 2 * 1000;

    private static final int TYPE_DATA_SUB_SWITCH = 1001;
    private static final int TYPE_DATA_ONOFF_SWITCH = 1002;

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (getActivity() == null) return;
            if (DBG) log("handle message: " + msg.what);
            switch (msg.what) {
                case MESSAGE_UPDATE_VIEW:
                    mSubInfoList = mSubscriptionManager.getActiveSubscriptionInfoList();
                    updatePreferences();
                    updateAllOptions();
                    break;
                case MESSAGE_UPDATE_DEFAULT_DATA:
                    updateDataSwitchValues();
                    break;
                case MESSAGE_DISMISS_DIALOG:
                    onSwitchDataTimeout();
                    break;
                case MESSAGE_CHECK_SWITCH_COMPLETED:
                    checkIfSwitchCompleted();
                    break;
                case MESSAGE_CHECK_DATA_ONOFF_COMPLETED:
                    checkIfDataOnOffCompleted();
                    break;
                case MESSAGE_DATA_ONOFF_COMPLETED:
                    onSwitchDataCompleted();
                    break;
                default:
                    log("unsupported message!");
                    break;
            }
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (DBG) log("receive action=" + action);
            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)
                    || TelephonyIntents.ACTION_SUBINFO_RECORD_UPDATED.equals(action)
                    || TelephonyIntents.ACTION_SUBINFO_CONTENT_CHANGE.equals(action)) {
                mHandler.removeMessages(MESSAGE_UPDATE_VIEW);
                mHandler.sendEmptyMessage(MESSAGE_UPDATE_VIEW);
            } else if (TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED.equals(action) ||
                    ConnectivityManager.CONNECTIVITY_ACTION.equals(action)){
                mHandler.removeMessages(MESSAGE_UPDATE_DEFAULT_DATA);
                mHandler.sendEmptyMessage(MESSAGE_UPDATE_DEFAULT_DATA);
            }
        }
    };

    private void startListenForPhoneState(int subId) {
        if (SubscriptionManager.isValidSubscriptionId(subId)) {
            if (mPhoneStateListeners.get(subId) != null) return;
            if (DBG) log("startListenForPhone sub " + subId);
            MobilePhoneStateListener phoneStateListener = new MobilePhoneStateListener(subId);
            mTelephonyManager.listen(phoneStateListener,
                        PhoneStateListener.LISTEN_SERVICE_STATE
                        | PhoneStateListener.LISTEN_DATA_CONNECTION_STATE);
            mPhoneStateListeners.put(subId, phoneStateListener);
        }
    }

    private void unregisterListeners() {
        Iterator<Integer> iterator = mPhoneStateListeners.keySet().iterator();
        while (iterator.hasNext()) {
            int id = iterator.next();
            PhoneStateListener phoneStateListener = mPhoneStateListeners.get(id);
            mTelephonyManager.listen(phoneStateListener, 0);
            iterator.remove();
            if (DBG) log("unregisterListener for sub " + id);
        }
    }

    class MobilePhoneStateListener extends PhoneStateListener {

        private int mDataAttachState = ServiceState.STATE_OUT_OF_SERVICE;
        private int mDataConnectState = TelephonyManager.DATA_DISCONNECTED;

        public MobilePhoneStateListener(int subId) {
            super(subId);
        }

        @Override
        public void onServiceStateChanged(ServiceState state) {
            if (DBG) log("onServiceStateChanged, sub=" + mSubId + ", state=" + state);
            mDataAttachState = null != state ? state.getDataRegState() : ServiceState.STATE_OUT_OF_SERVICE;
            notifyDataStateChanged();
        }

        @Override
        public void onDataConnectionStateChanged(int state) {
            if (DBG) log("onDataConnectionStateChanged, sub=" + mSubId + ", state=" + state);
            mDataConnectState = state;
            notifyDataStateChanged();
        }

        public int getDataState() {
            return mDataAttachState;
        }

        public boolean isDataConnected() {
            return mDataConnectState == TelephonyManager.DATA_CONNECTED;
        }
    }

    private void notifyDataStateChanged() {
        int message = 0;
        switch (mSwitchingType) {
            case TYPE_DATA_SUB_SWITCH:
                message = MESSAGE_CHECK_SWITCH_COMPLETED;
                break;
            case TYPE_DATA_ONOFF_SWITCH:
                message = MESSAGE_CHECK_DATA_ONOFF_COMPLETED;
                break;
            default:
                log("onServiceStateChanged(), no switching type!!!");
                return;
        }
        mHandler.removeMessages(message);
        mHandler.sendEmptyMessage(message);
    }

    private void checkIfSwitchCompleted() {
        int preDataState = ServiceState.STATE_OUT_OF_SERVICE;
        int curDataState = ServiceState.STATE_OUT_OF_SERVICE;
        if (mPhoneStateListeners.get(mPreDefaultDataSubId) != null) {
            preDataState = mPhoneStateListeners.get(mPreDefaultDataSubId).getDataState();
        }
        if (mPhoneStateListeners.get(mCurDefaultDataSubId) != null) {
            curDataState = mPhoneStateListeners.get(mCurDefaultDataSubId).getDataState();
        }
        if (DBG) log("checkIfSwitchCompleted, preDataState=" + preDataState + ", curDataState=" + curDataState);
        if (preDataState == ServiceState.STATE_OUT_OF_SERVICE &&
                curDataState == ServiceState.STATE_IN_SERVICE) {
            onSwitchDataCompleted();
        }
    }

    private void checkIfDataOnOffCompleted() {
        boolean isDataConnected = false;
        mCurDefaultDataSubId = SubscriptionManager.getDefaultDataSubId();
        if (mPhoneStateListeners.get(mCurDefaultDataSubId) != null) {
            isDataConnected = mPhoneStateListeners.get(mCurDefaultDataSubId).isDataConnected();
        }
        final boolean isDataEnable = mTelephonyManager.getDataEnabled();
        if (DBG) log("checkIfDataOnOffCompleted, isDataConnected=" + isDataConnected
                                    + ", mCurDefaultDataSubId=" + mCurDefaultDataSubId
                                    + ", isDataEnable=" + isDataEnable);
        if ((isDataEnable && isDataConnected) ||
                (!isDataEnable && !isDataConnected)) {
            onSwitchDataCompleted();
        }
    }

    private void waitingForDataSwitch() {
        if (DBG) log("waitingForDataSwitch()");
        showDialog(DIALOG_SWITCHING_DATA);
    }

    private void onSwitchDataCompleted() {
        if (DBG) log("onSwitchDataCompleted");
        resetSwitchState();
        mHandler.removeMessages(MESSAGE_DISMISS_DIALOG);
        removeDialog(DIALOG_SWITCHING_DATA);
    }

    private void onSwitchDataTimeout() {
        if (DBG) log("onSwitchDataTimeout");
        resetSwitchState();
        removeDialog(DIALOG_SWITCHING_DATA);
    }

    private void resetSwitchState() {
        mSwitchingType = -1;
        mCurDefaultDataSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        mPreDefaultDataSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    }

    @Override
    public void onDialogShowing() {
        super.onDialogShowing();
        if (DBG) log("onDialogShowing");
        setCancelable(false);
        if (!mHandler.hasMessages(MESSAGE_DISMISS_DIALOG)) {
            mHandler.sendEmptyMessageDelayed(MESSAGE_DISMISS_DIALOG, MAX_DIALOG_SHOWING_DELAYED);
        }
    }

    @Override
    public Dialog onCreateDialog(int id) {
        if (id == DIALOG_SWITCHING_DATA) {
            ProgressDialog dialog = new ProgressDialog(getActivity());
            dialog.setMessage(getResources().getString(R.string.switching_default_data_sub));
            dialog.setCancelable(false);
            return dialog;
        }
        return null;
    }
}
