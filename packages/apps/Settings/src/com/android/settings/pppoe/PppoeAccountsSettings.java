package com.android.settings.pppoe;

import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

import com.android.settings.R;

public class PppoeAccountsSettings extends PreferenceActivity implements
		OnPreferenceChangeListener {

	private static final String TAG = "pppoeAccountsSettings";

	private static final int ID_INDEX = 0;
	private static final int NAME_INDEX = 1;
	private static final int USER_INDEX = 2;
	private static final int DNS1_INDEX = 3;
	private static final int DNS2_INDEX = 4;
	private static final int PASSWORD_INDEX = 5;

	private static final int MENU_NEW = Menu.FIRST;
	private static final int MENU_RESTORE = Menu.FIRST + 1;

	private static final int DIALOG_RESTORE_DEFAULTACCOUNT = 1001;
	private static boolean mRestoreDefaultAccountsMode;

	private static final int EVENT_RESTORE_DEFAULTACCOUNTS_START = 1;
	private static final int EVENT_RESTORE_DEFAULTACCOUNTS_COMPLETE = 2;

	private RestoreAccountsUiHandler mRestoreAccountsUiHandler;
	private RestoreAccountsProcessHandler mRestoreAccountsProcessHandler;

	private String mSelectedKey;
	private PreferenceScreen mNewPppoePreferenceScreen;
	private PreferenceScreen mDelPppoePreferenceScreen;

	public static final String PREFERRED_PPPOE_ACCOUNTS_URI_STR = "content://pppoe/accounts/preferaccount";
	private static final Uri PREFERPPPOE_ACCOUNT_URI = Uri
			.parse(PREFERRED_PPPOE_ACCOUNTS_URI_STR);

	public boolean onPreferenceChange(Preference preference, Object newValue) {
		Log.d(TAG,
				"onPreferenceChange(): Preference - " + preference
						+ ", newValue - " + newValue + ", newValue type - "
						+ newValue.getClass());
		if (newValue instanceof String) {
			setSelectedPppoeAccountsKey((String) newValue);
		}
		return true;
	}

	private void setSelectedPppoeAccountsKey(String key) {
		mSelectedKey = key;
		ContentResolver resolver = getContentResolver();
		ContentValues values = new ContentValues();
		values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID, mSelectedKey);
		resolver.update(PREFERPPPOE_ACCOUNT_URI, values, null, null);
	}

	private String getSelectedPppoeAccountsKey() {
		String key = null;
		Cursor cursor = /*managedQuery*/getContentResolver().query(
				PREFERPPPOE_ACCOUNT_URI,
				new String[] { "_id" },
				null,
				null,
				PppoeAccountsContentMeta.PppoeAccountsColumns.DEFAULT_SORT_ORDER);
		if (cursor.getCount() > 0) {
			cursor.moveToFirst();
			key = cursor.getString(ID_INDEX);
		}
		cursor.close();
		return key;
	}

	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
			Preference preference) {
		Log.d(TAG, "onPreferenceTreeClick");
		int pos = Integer.parseInt(preference.getKey());
		Uri url = ContentUris.withAppendedId(
				PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI, pos);
		startActivity(new Intent(Intent.ACTION_EDIT, url));
		return true;
	}

	@Override
	protected void onCreate(Bundle icicle) {
		super.onCreate(icicle);

		addPreferencesFromResource(R.xml.pppoe_accounts_setting);
		getListView().setItemsCanFocus(true);

		mNewPppoePreferenceScreen = (PreferenceScreen) findPreference("add_new_pppoe");
		mNewPppoePreferenceScreen
				.setOnPreferenceClickListener(new OnPreferenceClickListener() {

					@Override
					public boolean onPreferenceClick(Preference preference) {
						// TODO Auto-generated method stub
						addNewPppoeAccounts();
						return true;
					}
				});

		mDelPppoePreferenceScreen = (PreferenceScreen) findPreference("del_selected_pppoe");
		mDelPppoePreferenceScreen
				.setOnPreferenceClickListener(new OnPreferenceClickListener() {
		
					@Override
					public boolean onPreferenceClick(Preference preference) {
						// TODO Auto-generated method stub
						DelPppoeAccounts();
						return true;
					}
				});

	}

	@Override
	protected void onResume() {
		super.onResume();

		fillList();
	}

	private void fillList() {
		String[] projection = new String[] {
				PppoeAccountsContentMeta.PppoeAccountsColumns.ID,
				PppoeAccountsContentMeta.PppoeAccountsColumns.NAME,
				PppoeAccountsContentMeta.PppoeAccountsColumns.USER,
				PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1,
				PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2,
				PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD, };

		Cursor c = /*managedQuery*/getContentResolver().query(
				PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI,
				projection,
				null,
				null,
				PppoeAccountsContentMeta.PppoeAccountsColumns.DEFAULT_SORT_ORDER);

		PreferenceGroup pppoe_accounts_list = (PreferenceGroup) findPreference("pppoe_accounts_list");
		pppoe_accounts_list.removeAll();
		mSelectedKey = getSelectedPppoeAccountsKey();
		c.moveToFirst();
		while (!c.isAfterLast()) {
			String key = c.getString(ID_INDEX);
			String name = c.getString(NAME_INDEX);
			String user = c.getString(USER_INDEX);
			String dns1 = c.getString(DNS1_INDEX);
			String dns2 = c.getString(DNS2_INDEX);
			String password = c.getString(PASSWORD_INDEX);

			PppoeAccountsPreference pref = new PppoeAccountsPreference(this);

			pref.setKey(key);
			pref.setTitle(name);
			pref.setSummary(user);
			pref.setPersistent(false);
			pref.setOnPreferenceChangeListener(this);

			if ((mSelectedKey != null) && mSelectedKey.equals(key)) {
				pref.setChecked();
			}
			pppoe_accounts_list.addPreference(pref);

			c.moveToNext();
		}
		c.close();
		if (pppoe_accounts_list.getPreferenceCount() == 1) {
			PppoeAccountsPreference pref = (PppoeAccountsPreference) pppoe_accounts_list
					.getPreference(0);
			if (!pref.isChecked()) {
				pref.setChecked();
				String key = pref.getKey();
				setSelectedPppoeAccountsKey(key);
			}
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		menu.add(0, MENU_NEW, 0,
				getResources().getString(R.string.pppoe_accounts_menu_new))
				.setIcon(android.R.drawable.ic_menu_add);
		menu.add(0, MENU_RESTORE, 0,
				getResources().getString(R.string.pppoe_accounts_menu_restore))
				.setIcon(android.R.drawable.ic_menu_upload);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {

		switch (item.getItemId()) {
		case MENU_NEW:
			addNewPppoeAccounts();
			break;

		case MENU_RESTORE:
			restoreDefaultPppoeAccounts();
			break;
		}
		return super.onOptionsItemSelected(item);
	}

	private boolean restoreDefaultPppoeAccounts() {
		//showDialog(DIALOG_RESTORE_DEFAULTACCOUNT);
		mRestoreDefaultAccountsMode = true;

		if (mRestoreAccountsUiHandler == null) {
			mRestoreAccountsUiHandler = new RestoreAccountsUiHandler();
		}

		if (mRestoreAccountsProcessHandler == null) {
			HandlerThread restoreDefaultApnThread = new HandlerThread(
					"Restore default Account Handler: Process Thread");
			restoreDefaultApnThread.start();
			mRestoreAccountsProcessHandler = new RestoreAccountsProcessHandler(
					restoreDefaultApnThread.getLooper(),
					mRestoreAccountsUiHandler);
		}

		mRestoreAccountsProcessHandler
				.sendEmptyMessage(EVENT_RESTORE_DEFAULTACCOUNTS_START);
		return true;
	}

	private class RestoreAccountsUiHandler extends Handler {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case EVENT_RESTORE_DEFAULTACCOUNTS_COMPLETE:
				fillList();
				getPreferenceScreen().setEnabled(true);
				mRestoreDefaultAccountsMode = false;
				//dismissDialog(DIALOG_RESTORE_DEFAULTACCOUNT);
				/*
				Toast.makeText(
						PppoeAccountsSettings.this,
						getResources()
								.getString(
										R.string.restore_default_pppoe_accounts_completed),
						Toast.LENGTH_LONG).show();
				*/		
				break;
			}
		}
	}

	private class RestoreAccountsProcessHandler extends Handler {
		private Handler mRestorePppoeAccountsUiHandler;

		public RestoreAccountsProcessHandler(Looper looper,
				Handler restoreAccountUiHandler) {
			super(looper);
			this.mRestorePppoeAccountsUiHandler = restoreAccountUiHandler;
		}

		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case EVENT_RESTORE_DEFAULTACCOUNTS_START:
				ContentResolver resolver = getContentResolver();
				resolver.delete(
						PppoeAccountsContentMeta.PppoeAccountsColumns.RESTORE_URI,
						null, null);
				PppoeAccountsPreference.clearSelectedKey();
				mRestorePppoeAccountsUiHandler
						.sendEmptyMessage(EVENT_RESTORE_DEFAULTACCOUNTS_COMPLETE);
				break;
			}
		}
	}

	private void addNewPppoeAccounts() {
		startActivity(new Intent(Intent.ACTION_INSERT,
				PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI));

	}

	private void DelPppoeAccounts() {
		//restoreDefaultPppoeAccounts();
		if(null != mSelectedKey){
			Uri url = ContentUris.withAppendedId(
					PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI, Integer.parseInt(mSelectedKey));
			ContentResolver resolver = getContentResolver();
			resolver.delete(
					url,
					null, null);
			PppoeAccountsPreference.clearSelectedKey();	
			fillList();
			getPreferenceScreen().setEnabled(true);
			
		}
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == DIALOG_RESTORE_DEFAULTACCOUNT) {
			ProgressDialog dialog = new ProgressDialog(this);
			dialog.setMessage(getResources().getString(
					R.string.restore_default_pppoe_accounts));
			dialog.setCancelable(false);
			return dialog;
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		if (id == DIALOG_RESTORE_DEFAULTACCOUNT) {
			getPreferenceScreen().setEnabled(false);
		}
	}

}
