package com.android.launcher3;

import com.android.launcher3.NotificationController;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.SystemProperties;

public class StartupReceiver extends BroadcastReceiver {

    static final String SYSTEM_READY = "com.android.launcher3.SYSTEM_READY";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!SystemProperties.getBoolean("ro.boot_boost.enable", false)) {
            context.sendStickyBroadcast(new Intent(SYSTEM_READY));
        }

        CharSequence isPrescan = intent.getExtras().getCharSequence("com.android.pms.PRESCAN");
        if (isPrescan != null && isPrescan.equals("true")) {
            if(NotificationController.hasNotification == true){
                NotificationController.clearAllNotify(context);
            }
            context.sendStickyBroadcast(new Intent(SYSTEM_READY));
        }
    }
}
