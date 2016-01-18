/*
 ** Copyright (C) 2015 Intel Corporation, All rights reserved
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

package com.android.systemui.statusbar.phone;

import java.util.List;
import java.util.ArrayList;
import android.os.Handler;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionManager.OnSubscriptionsChangedListener;
import android.telephony.TelephonyManager;

import com.android.systemui.R;

/* We can try two methods to return system SIM cards informations:
 * 1. Use OnSubscriptionsChangedListener(), this will cost more performance effort, but good.
 * 2. Dynamic parse SIM cards info everytime while UI invoke show() it. also good, less effort.
 * */
public class PhoneCards {
    private static PhoneCards mSelf = null;
    private final Context mContext;
    private final TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    private final TelecomManager mTelecomManager;
    private final Handler mHandler = new Handler();

    // importance list to store current SIM card basic info.
    // Need to be locked.
    private List<CardInfo> mSimInfo = new ArrayList<CardInfo>(2);
    // A virtual ASK card with fixed info, so cache it.
    private CardInfo mAskInfo = null;

    private PhoneCards(Context context) {
        mContext = context;
        mTelephonyManager = TelephonyManager.getDefault();
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mTelecomManager = TelecomManager.from(mContext);
        // Method 1 begin
        // mSubscriptionManager.addOnSubscriptionsChangedListener(mSubscriptionListener);
        // Method 1 end
    }

    public static PhoneCards getInstance(Context context) {
        if (mSelf == null) {
            mSelf = new PhoneCards(context);
        }
        return mSelf;
    }

    public class CardInfo {
        private int slot;
        private int subId;
        private String name;
        private int color;
        private Bitmap icon;

        CardInfo(int slot, int id, String name, int color, Bitmap icon) {
            this.slot = slot;
            this.subId = id;
            this.name = name;
            this.color = color;
            this.icon = icon;
        }

        public int getSlot() {
            return slot;
        }

        public int getId() {
            return subId;
        }

        public String getName() {
            return name;
        }

        public int getColor() {
            return color;
        }

        public Bitmap getIcon() {
            return icon;
        }
    }

    // Listener will infect system when first boot.
    private final OnSubscriptionsChangedListener mSubscriptionListener =
            new OnSubscriptionsChangedListener() {
                @Override
                public void onSubscriptionsChanged() {
                    // Use Handler will reduce the frequently SIM update.
                    mHandler.removeCallbacks(mDetectRunnable);
                    mHandler.postDelayed(mDetectRunnable, 1000);
                }
            };

    private final Runnable mDetectRunnable = new Runnable() {
        public void run() {
            detectedSimChange();
        }
    };

    synchronized private int detectedSimChange() {
        mSimInfo.clear();
        final List<SubscriptionInfo> silist = mSubscriptionManager.getActiveSubscriptionInfoList();
        if (silist == null) {
            return 0;
        }
        for (SubscriptionInfo sir : silist) {
            if (sir != null) {
                CardInfo tmp = new CardInfo(
                        sir.getSimSlotIndex(),
                        sir.getSubscriptionId(),
                        sir.getDisplayName().toString(),
                        sir.getIconTint(),
                        sir.createIconBitmap(mContext));
                mSimInfo.add(tmp);
            }
        }
        return mSimInfo.size();
    }

    public int getCardsNum() {
        // Method 2 begin
        return detectedSimChange();
        // Method 2 end
    }

    // Concept: Virtual slot should be 0(sim1),1(sim2),2(ask)
    synchronized public CardInfo getVirtualCard(int slot) {
        if (slot < 0 || slot > 2)
            return null;
        if (slot == 2) {
            return getAsk();
        }
        // Process with 0 and 1
        int count = mSimInfo.size();
        if (count == 0) {
            return null;
        } else if (count == 1) {
            CardInfo tmp = mSimInfo.get(0);
            if (tmp.getSlot() == slot) {
                return tmp;
            }
        } else if (count == 2) {
            return mSimInfo.get(slot);
        }
        return null;
    }

    // An import function to return all kind of default cards we set before.
    public CardInfo getDefault(int type) {
        CardInfo ret = null;
        if (type == 0/* Call */) {
            ret = getCall();
        } else if (type == 1/* Sms */) {
            ret = getSms();
        }
        if (ret == null) {
            ret = getAsk();
        }
        return ret;
    }

    // Concept: virtural ASK card, use the cache mechanism.
    public CardInfo getAsk() {
        if (mAskInfo == null) {
            Drawable d = mContext.getDrawable(R.drawable.ic_live_help);
            BitmapDrawable bd = (BitmapDrawable) d;

            Bitmap bm = bd.getBitmap();
            String str = mContext.getString(R.string.sim_switch_ask);
            mAskInfo = new CardInfo(-1, -1, str, -1, bm);
        }
        return mAskInfo;
    }

    // Transform PhoneAcount info to Subscription info
    private SubscriptionInfo findRecordBySubId(final int subId) {
        final List<SubscriptionInfo> subInfoList =
                mSubscriptionManager.getActiveSubscriptionInfoList();
        if (subInfoList != null) {
            final int subInfoLength = subInfoList.size();

            for (int i = 0; i < subInfoLength; ++i) {
                final SubscriptionInfo sir = subInfoList.get(i);
                if (sir != null && sir.getSubscriptionId() == subId) {
                    return sir;
                }
            }
        }
        return null;
    }

    public CardInfo getCall() {
        CardInfo ret = null;
        // default outgoing call is just included on PhoneAccount
        final PhoneAccountHandle phoneAccount = mTelecomManager
                .getUserSelectedOutgoingPhoneAccount();
        if (phoneAccount != null) {
            final String strId = phoneAccount.getId();
            final SubscriptionInfo sir = findRecordBySubId(Integer.parseInt(strId));
            if (sir != null) {
                int slot = sir.getSimSlotIndex();
                int id = sir.getSubscriptionId();
                String name = sir.getDisplayName().toString();
                ret = new CardInfo(
                        sir.getSimSlotIndex(),
                        sir.getSubscriptionId(),
                        sir.getDisplayName().toString(),
                        sir.getIconTint(),
                        sir.createIconBitmap(mContext));
            }
        }
        return ret;
    }

    // Set default card that voice call uses
    public int setCall(int num) {
        if (num != 0 && num != 1) {
            mTelecomManager.setUserSelectedOutgoingPhoneAccount(null);
            return 0;
        }

        final List<PhoneAccountHandle> phoneAccountsList =
                mTelecomManager.getCallCapablePhoneAccounts();
        int availableCard = phoneAccountsList.size();
        if (availableCard == 1) {
            mTelecomManager.setUserSelectedOutgoingPhoneAccount(phoneAccountsList.get(0));
        } else {
            mTelecomManager.setUserSelectedOutgoingPhoneAccount(phoneAccountsList.get(num));
        }
        return 0;
    }

    public CardInfo getSms() {
        CardInfo ret = null;
        final SubscriptionInfo sir = mSubscriptionManager.getDefaultSmsSubscriptionInfo();
        if (sir != null) {
            int slot = sir.getSimSlotIndex();
            int id = sir.getSubscriptionId();
            String name = sir.getDisplayName().toString();
            ret = new CardInfo(
                    sir.getSimSlotIndex(),
                    sir.getSubscriptionId(),
                    sir.getDisplayName().toString(),
                    sir.getIconTint(),
                    sir.createIconBitmap(mContext));
        }
        return ret;
    }

    // set default card that message uses
    synchronized public int setSms(int slot) {
        if (slot != 0 && slot != 1) {
            mSubscriptionManager.setDefaultSmsSubId(SubscriptionManager.INVALID_SUBSCRIPTION_ID);
            return 0;
        }
        int availableCard = mSimInfo.size();
        CardInfo tmp = null;
        if (availableCard == 1) {
            tmp = mSimInfo.get(0);
        } else {
            tmp = mSimInfo.get(slot);
        }
        int subId = tmp.getId();
        mSubscriptionManager.setDefaultSmsSubId(subId);
        return 0;
    }

    // An utility tool to decorate the icon with selected effect*/
    public static Bitmap selectedIcon(Bitmap bitmap, boolean selected) {
        final int width = 80, height = 80;
        final int stroke = 8;
        final int padding = 2;
        Bitmap newBitmap = null;
        newBitmap = Bitmap.createScaledBitmap(bitmap, width, height, true);
        if (!selected) {
            // return a scaled bitmap
            return newBitmap;
        }
        Canvas canvas = new Canvas(newBitmap);
        Paint paint = new Paint();
        paint.setColor(0xFF80CBC4);/*The same as Display SeekBar color*/
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(stroke);
        canvas.drawRect(padding, 0, newBitmap.getWidth()-padding, newBitmap.getWidth(), paint);
        canvas.save(Canvas.ALL_SAVE_FLAG);
        canvas.restore();
        // return a selected scaled bitmap
        return newBitmap;
    }

}
