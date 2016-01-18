package com.android.settings;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.telephony.CellInfo;
import android.telephony.CellLocation;
import android.telephony.DataConnectionRealTimeInfo;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.telephony.NeighboringCellInfo;
import android.telephony.cdma.CdmaCellLocation;
import android.telephony.gsm.GsmCellLocation;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Toast;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.PhoneStateIntentReceiver;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.DefaultPhoneNotifier;
import com.android.settings.R;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

public class SMSCSettingActivity extends Activity {
    private final String TAG = "SMSCSettingActivity";
	
	private TelephonyManager mTelephonyManager;
	private Phone phone = null;
	private int mPhoneId = SubscriptionManager.DEFAULT_PHONE_INDEX;
	
	private static final int EVENT_QUERY_SMSC_DONE = 1005;
	private static final int EVENT_UPDATE_SMSC_DONE = 1006;
	public static final String EXTRA_TITLE = "title";
	
	private EditText smsc;
    private Button updateSmscButton;
    private Button refreshSmscButton;
	
    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        setContentView(R.layout.smsc_setting_activity);
        
	    mTelephonyManager = (TelephonyManager)getSystemService(TELEPHONY_SERVICE);
	    Intent intent = getIntent();
	    if (intent != null) {
	        mPhoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY, SubscriptionManager.DEFAULT_PHONE_INDEX);
	        if(intent.hasExtra(EXTRA_TITLE)){
	        	setTitle(intent.getStringExtra(EXTRA_TITLE));
	        }
	    }
        log("received phone id: " + mPhoneId);
        phone = PhoneFactory.getPhone(mPhoneId);
        
        smsc = (EditText) findViewById(R.id.smsc);
        updateSmscButton = (Button) findViewById(R.id.update_smsc);
        updateSmscButton.setOnClickListener(mUpdateSmscButtonHandler);
        refreshSmscButton = (Button) findViewById(R.id.refresh_smsc);
        refreshSmscButton.setOnClickListener(mRefreshSmscButtonHandler);
    }
    
    OnClickListener mUpdateSmscButtonHandler = new OnClickListener() {
    	public void onClick(View v) {
    		if(smsc.getText().toString().trim().length() > 0){    			
    			updateSmscButton.setEnabled(false);
    			phone.setSmscAddress(smsc.getText().toString(),
    					mHandler.obtainMessage(EVENT_UPDATE_SMSC_DONE));
    		}else{
    			Toast.makeText(SMSCSettingActivity.this, R.string.smsc_input_first, 
    					Toast.LENGTH_SHORT).show();
    		}
    	}
    };

	OnClickListener mRefreshSmscButtonHandler = new OnClickListener() {
	    public void onClick(View v) {
	        refreshSmsc();
	    }
	};
    	
	private void refreshSmsc() {
		phone.getSmscAddress(mHandler.obtainMessage(EVENT_QUERY_SMSC_DONE));
	}
	
	private Handler mHandler = new Handler() {
		public void handleMessage(Message msg) {
	        AsyncResult ar;
	        switch (msg.what) {
	          	case EVENT_QUERY_SMSC_DONE:
	                ar= (AsyncResult) msg.obj;
	                if (ar.exception != null) {
	                    smsc.setText("refresh error");
	                } else {
	                    smsc.setText((String)ar.result);
	                }
	                break;
	            case EVENT_UPDATE_SMSC_DONE:
	                updateSmscButton.setEnabled(true);
	                ar= (AsyncResult) msg.obj;
	                if (ar.exception != null) {
	                    smsc.setText("update error");
	                }else{
	                	Toast.makeText(SMSCSettingActivity.this,
	                			R.string.smsc_update_success, 
	                			Toast.LENGTH_SHORT).show();
	                }
	                break;
	            default:
	                break;
	      }
	  }
	};
	
	private void log(String s) {
		Log.d(TAG, "[RadioInfo] " + s);
	}
}