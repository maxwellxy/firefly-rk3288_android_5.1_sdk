/* $_FOR_ROCKCHIP_RBOX_$ */
package com.android.systemui.usb;

import com.android.systemui.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.storage.IMountService;
import android.os.ServiceManager;
import android.util.Log;
import android.widget.Toast;

public class UsbStorageUnmountDialog extends Activity {
	private final String TAG = "UsbStorageUnmountDialog";
	
    private static final int DLG_CONFIRM_UNMOUNT = 1;
    private static final int DLG_ERROR_UNMOUNT = 2;
    
 // Access using getMountService()
    private IMountService mMountService;
    
    private String mMountPoint;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	// TODO Auto-generated method stub
    	super.onCreate(savedInstanceState);
    	mMountPoint = getIntent().getStringExtra("mount-point");
    	Log.d(TAG,"ready to unmount mount-point = " + mMountPoint);
    	if (mMountPoint != null) {
    		unmount(mMountPoint);
    	} else 
    		finish();
    }
    
    @Override
    @Deprecated
    protected Dialog onCreateDialog(int id) {
    	// TODO Auto-generated method stub
    	switch (id) {
        case DLG_CONFIRM_UNMOUNT:
                return new AlertDialog.Builder(UsbStorageUnmountDialog.this)
                    .setTitle(R.string.dlg_confirm_unmount_title)
                    .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            doUnmount(mMountPoint);
                        }})
                    .setNegativeButton(android.R.string.cancel, null)
					.setOnDismissListener(new DialogInterface.OnDismissListener() {
						
						@Override
						public void onDismiss(DialogInterface dialog) {
							// TODO Auto-generated method stub
							UsbStorageUnmountDialog.this.finish();
						}
					})
                    .setMessage(R.string.dlg_confirm_unmount_text)
                    .create();
        case DLG_ERROR_UNMOUNT:
                return new AlertDialog.Builder(UsbStorageUnmountDialog.this)
            .setTitle(R.string.dlg_error_unmount_title)
            .setNeutralButton(android.R.string.cancel, null)
            .setOnDismissListener(new DialogInterface.OnDismissListener() {
				
				@Override
				public void onDismiss(DialogInterface dialog) {
					// TODO Auto-generated method stub
					UsbStorageUnmountDialog.this.finish();
				}
			})
            .setMessage(R.string.dlg_error_unmount_text)
            .create();
        }
        return null;
    }
    
    private synchronized IMountService getMountService() {
        if (mMountService == null) {
            IBinder service = ServiceManager.getService("mount");
            if (service != null) {
                mMountService = IMountService.Stub.asInterface(service);
            } else {
                Log.e(TAG, "Can't get mount service");
            }
        }
        return mMountService;
    }
    
    private void doUnmount(String mountpoint) {
        // Present a toast here
        Toast.makeText(UsbStorageUnmountDialog.this, R.string.unmount_inform_text, Toast.LENGTH_SHORT).show();
        IMountService mountService = getMountService();
        try {
            mountService.unmountVolume(mountpoint, true, false);
            finish();
        } catch (RemoteException e) {
            // Informative dialog to user that unmount failed.
            showDialogInner(DLG_ERROR_UNMOUNT);
        }
    }

    private void showDialogInner(int id) {
        removeDialog(id);
        showDialog(id);
    }

    private boolean hasAppsAccessingStorage(String mountpoint) throws RemoteException {
        IMountService mountService = getMountService();
        int stUsers[] = mountService.getStorageUsers(mountpoint);
        if (stUsers != null && stUsers.length > 0) {
            return true;
        }
        // TODO FIXME Parameterize with mountPoint and uncomment.
        // On HC-MR2, no apps can be installed on sd and the emulated internal storage is not
        // removable: application cannot interfere with unmount
        /*
        ActivityManager am = (ActivityManager)getSystemService(Context.ACTIVITY_SERVICE);
        List<ApplicationInfo> list = am.getRunningExternalApplications();
        if (list != null && list.size() > 0) {
            return true;
        }
        */
        // Better safe than sorry. Assume the storage is used to ask for confirmation.
        return true;
    }

    private void unmount(String mountpoint) {
        // Check if external media is in use.
        try {
           if (hasAppsAccessingStorage(mountpoint)) {
               // Present dialog to user
               showDialogInner(DLG_CONFIRM_UNMOUNT);
           } else {
               doUnmount(mountpoint);
           }
        } catch (RemoteException e) {
            // Very unlikely. But present an error dialog anyway
            Log.e(TAG, "Is MountService running?");
            showDialogInner(DLG_ERROR_UNMOUNT);
        }
    }
}
