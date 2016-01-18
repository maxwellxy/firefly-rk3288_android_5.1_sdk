/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.content.Intent;
import android.content.Context;
import android.content.DialogInterface;

import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.PhoneConstants;
import android.app.AlertDialog;
import java.util.List;
import java.util.ArrayList;

public class TestingSettings extends PreferenceActivity {
    private static final String PHONE_INFO = "phone_info";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        addPreferencesFromResource(R.xml.testing_settings);
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        final String key = preference.getKey();
        if (key != null && key.equals(PHONE_INFO)) {
            final Intent intent = new Intent();
            intent.setClassName("com.android.settings", "com.android.settings.RadioInfo");
            intent.putExtra(PhoneConstants.PHONE_KEY,SubscriptionManager.DEFAULT_PHONE_INDEX);
            int simCount = TelephonyManager.getDefault().getSimCount();
            if (simCount > 1) {
                final List<SubscriptionInfo> subInfoList = SubscriptionManager.from(this).getActiveSubscriptionInfoList();
                if (subInfoList != null && subInfoList.size() > 1) {
                    final List<Integer> slots = getPhoneList(subInfoList);
                    String title = preference.getTitle().toString();
                    final String[] itemsStr = new String[slots.size()];
                    for(int slot: slots){
                        itemsStr[slot] = title + " " + (slot+1);
                    }

                    AlertDialog.Builder builder= new AlertDialog.Builder(this);
                    builder.setTitle(preference.getTitle())
                            .setItems(itemsStr, new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    if(which >=0 && (which < slots.size())){
                                        intent.putExtra(PhoneConstants.PHONE_KEY,slots.get(which));
                                    }
                                    startActivity(intent);
                                }
                            })
                            .show();
                } else {
                    if((subInfoList != null) && (subInfoList.size() == 1)){
                        intent.putExtra(PhoneConstants.PHONE_KEY, subInfoList.get(0).getSimSlotIndex());
                    }
                    startActivity(intent);
                }
            } else {
                startActivity(intent);
            }
            return true;
        } else {
            return super.onPreferenceTreeClick(preferenceScreen, preference);
        }
    }

    private List<Integer> getPhoneList(List<SubscriptionInfo> subInfoList) {
        final TelephonyManager tm =
            (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        final int numSlots = tm.getSimCount();
        List<Integer> slotArray = new ArrayList<Integer>(numSlots);
        for (int i = 0; i < numSlots; ++i) {
            for (int j = 0; j < subInfoList.size(); ++j) {
                final int slotId = subInfoList.get(j).getSimSlotIndex();
                if (slotId == i) {
                    slotArray.add(i);
                    break;
                }
            }
        }
        return slotArray;
    }

}
