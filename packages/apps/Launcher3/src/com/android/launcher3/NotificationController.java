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

package com.android.launcher3;

import android.app.NotificationManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.widget.Toast;

public class NotificationController{
    private static final String TAG = "NotificationController";
    private static final boolean DEBUG = true;
    private static final int NOTIFICATION_ID = 1;
    private static Context mContext;
    public static boolean hasNotification = false;

    private static NotificationCompat.Builder mBuilder;
    private static NotificationManager mNotificationManager;

	public static void initNotify(Context mctx){
        if(DEBUG) Log.e(TAG, "initNotify");

        mContext = mctx;
		mBuilder = new NotificationCompat.Builder(mContext);
        mNotificationManager = (NotificationManager)mContext.getSystemService(Context.NOTIFICATION_SERVICE);
		mBuilder.setContentTitle("test title")
				.setContentText("test content")
				.setTicker("test notification coming")
				.setWhen(System.currentTimeMillis())
				.setPriority(Notification.PRIORITY_DEFAULT)
				.setOngoing(true)
				.setDefaults(Notification.DEFAULT_VIBRATE)
				.setSmallIcon(R.drawable.ic_launcher_info_normal_holo);
	}

	public static void showNotify(Context mctx){
        if(DEBUG) Log.e(TAG, "showNotify");

        mContext = mctx;
		mBuilder.setContentTitle("Scanning app...")
				.setContentText("Launcher is scanning app")
				.setTicker("Launcher notification");
		mNotificationManager.notify(NOTIFICATION_ID, mBuilder.build());

        Toast.makeText(mContext, "Scanning app...", Toast.LENGTH_SHORT).show();
        hasNotification = true;
	}

    public static void clearAllNotify(Context mctx) {
        if(DEBUG) Log.e(TAG, "clearAllNotify");

        mContext = mctx;
		mNotificationManager.cancelAll();
        Toast.makeText(mContext, "Scanning app finish", Toast.LENGTH_SHORT).show();
        hasNotification = false;
	}
}
