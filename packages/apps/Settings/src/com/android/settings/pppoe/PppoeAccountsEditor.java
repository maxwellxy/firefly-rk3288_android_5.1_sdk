//$_FOR_ROCKCHIP_RBOX_$by_hhq_20120616
package com.android.settings.pppoe;

import android.app.AlertDialog;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.res.Resources;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;

import com.android.settings.R;

public class PppoeAccountsEditor extends PreferenceActivity implements
		OnSharedPreferenceChangeListener, OnPreferenceChangeListener {

	private static final String TAG = "pppoeAccountsEditor";

	private EditTextPreference mName;
	private EditTextPreference mUser;
	private EditTextPreference mDns1;
	private EditTextPreference mDns2;
	private EditTextPreference mPassword;

	private Uri mUri;
	private Cursor mCursor;
	private boolean mFirstTime;
	private boolean mNewAccount;
	private Resources mRes;

	private static final String[] sProjection = new String[] {
			PppoeAccountsContentMeta.PppoeAccountsColumns._ID,
			PppoeAccountsContentMeta.PppoeAccountsColumns.NAME,
			PppoeAccountsContentMeta.PppoeAccountsColumns.USER,
			PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1,
			PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2,
			PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD, };

	private static final int ID_INDEX = 0;
	private static final int NAME_INDEX = 1;
	private static final int USER_INDEX = 2;
	private static final int DNS1_INDEX = 3;
	private static final int DNS2_INDEX = 4;
	private static final int PASSWORD_INDEX = 5;

	private static final int MENU_DELETE = Menu.FIRST;
	private static final int MENU_SAVE = Menu.FIRST + 1;
	private static final int MENU_CANCEL = Menu.FIRST + 2;

	private static final String SAVED_POS = "pos";

	private static String sNotSet;

	@Override
	protected void onCreate(Bundle icicle) {
		super.onCreate(icicle);
		addPreferencesFromResource(R.xml.pppoe_accounts_editor);

		sNotSet = getResources().getString(R.string.pppoe_accounts_not_set);
		mRes = getResources();
		mName = (EditTextPreference) findPreference("pppoe_accounts_name");
		mUser = (EditTextPreference) findPreference("pppoe_accounts_user");
		mDns1 = (EditTextPreference) findPreference("pppoe_accounts_dns1");
		mDns2 = (EditTextPreference) findPreference("pppoe_accounts_dns2");
		mPassword = (EditTextPreference) findPreference("pppoe_accounts_password");

		final Intent intent = getIntent();
		final String action = intent.getAction();

		mFirstTime = icicle == null;
		if (action.equals(Intent.ACTION_EDIT)) {
			mUri = intent.getData();
		} else if (action.equals(Intent.ACTION_INSERT)) {
			if (mFirstTime || icicle.getInt(SAVED_POS) == 0) {
				mUri = getContentResolver().insert(intent.getData(),
						new ContentValues());
			} else {
				mUri = ContentUris
						.withAppendedId(
								PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI,
								icicle.getInt(SAVED_POS));
			}
			mNewAccount = true;

			// If we were unable to create a new note, then just finish
			// this activity. A RESULT_CANCELED will be sent back to the
			// original activity if they requested a result.
			if (mUri == null) {
				Log.w(TAG, "Failed to insert new account provider into "
						+ getIntent().getData());
				finish();
				return;
			}

			// The new entry was created, so assume all will end well and
			// set the result to be returned.
			setResult(RESULT_OK, (new Intent()).setAction(mUri.toString()));

		} else {
			finish();
			return;
		}

		mCursor = managedQuery(
				mUri,
				sProjection,
				null,
				null,
				PppoeAccountsContentMeta.PppoeAccountsColumns.DEFAULT_SORT_ORDER);
		mCursor.moveToFirst();

		fillUi();
	}

	@Override
	public void onResume() {
		super.onResume();
		getPreferenceScreen().getSharedPreferences()
				.registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	public void onPause() {
		getPreferenceScreen().getSharedPreferences()
				.unregisterOnSharedPreferenceChangeListener(this);
		super.onPause();
	}

	private void fillUi() {
		if (mFirstTime) {
			mFirstTime = false;
			mName.setText(mCursor.getString(NAME_INDEX));
			mUser.setText(mCursor.getString(USER_INDEX));
			mDns1.setText(mCursor.getString(DNS1_INDEX));
			mDns2.setText(mCursor.getString(DNS2_INDEX));
			mPassword.setText(mCursor.getString(PASSWORD_INDEX));
		}

		mName.setSummary(checkNull(mName.getText()));
		mUser.setSummary(checkNull(mUser.getText()));
		mDns1.setSummary(checkNull(mDns1.getText()));
		mDns2.setSummary(checkNull(mDns2.getText()));
		mPassword.setSummary(starify(mPassword.getText()));
	}

	@Override
	protected void onSaveInstanceState(Bundle icicle) {
		super.onSaveInstanceState(icicle);
		if (validateAndSave(true)) {
			icicle.putInt(SAVED_POS, mCursor.getInt(ID_INDEX));
		}
	}

	private boolean validateAndSave(boolean force) {
		String name = checkNotSet(mName.getText());
		String user = checkNotSet(mUser.getText());
		String dns1 = checkNotSet(mDns1.getText());
		String dns2 = checkNotSet(mDns2.getText());
		String password = checkNotSet(mPassword.getText());

		String errMsg = null;
		if (name.length() < 1) {
			errMsg = mRes.getString(R.string.pppoe_accounts_error_name_empty);
		} else if (user.length() < 1) {
			errMsg = mRes.getString(R.string.pppoe_accounts_error_user_empty);
		} else if (password.length() < 1) {
			errMsg = mRes
					.getString(R.string.pppoe_accounts_error_password_empty);
		}

		if (null != errMsg && !force) {
			showErrorMessage(errMsg);
			return false;
		}

		if (!mCursor.moveToFirst()) {
			Log.w(TAG,
					"Could not go to the first row in the Cursor when saving data.");
			return false;
		}

		// If it's a new account and a name or user haven't been entered, then
		// erase the entry
		if (force && mNewAccount && name.length() < 1 && user.length() < 1) {
			getContentResolver().delete(mUri, null, null);
			return false;
		}

		ContentValues values = new ContentValues();
		// Add a dummy name "Untitled", if the user exits the screen without
		// adding a name but
		// entered other information worth keeping.
		values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.NAME, name
				.length() < 1 ? mRes.getString(R.string.untitled_pppoe_account)
				: name);
		values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.USER, user);
		values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1, dns1);
		values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2, dns2);
		values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD,
				password);

		getContentResolver().update(mUri, values, null, null);

		return true;
	}

	private void showErrorMessage(String errMsg) {
		new AlertDialog.Builder(this)
				.setTitle(R.string.pppoe_accounts_error_title)
				.setMessage(errMsg)
				.setPositiveButton(android.R.string.ok, null).show();
	}

	private String checkNotSet(String value) {
		if (value == null || value.equals(sNotSet)) {
			return "";
		} else {
			return value;
		}
	}

	//$_rbox_$_modify_$ add by hhq 20120616 for exit editor
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		switch (keyCode) {
		case KeyEvent.KEYCODE_BACK: {      
        	new AlertDialog.Builder(PppoeAccountsEditor.this)
        		.setTitle(R.string.str_about)
        		.setMessage(R.string.str_mesg)
        		.setPositiveButton(R.string.str_ok, 
        		 new DialogInterface.OnClickListener()
        		 {
        		    public void onClick(DialogInterface dialoginterfacd,int i)
        		    {
        				if (validateAndSave(false)) {
        					finish();
        				}
        		    }
        		 } 
        		)
        		.setNegativeButton(R.string.str_exit,
        		 new DialogInterface.OnClickListener()
       		 	{
        			public void onClick(DialogInterface dialoginterfacd,int i)
        			{
        				
        				final Intent intent = getIntent();
        				final String action = intent.getAction();
        				if (action.equals(Intent.ACTION_INSERT)) {
        				String name = checkNotSet(mName.getText());
        				String user = checkNotSet(mUser.getText());
        				String dns1 = checkNotSet(mDns1.getText());
        				String dns2 = checkNotSet(mDns2.getText());
        				String password = checkNotSet(mPassword.getText());
        				
        				ContentValues values = new ContentValues();
        				// Add a dummy name "Untitled", if the user exits the screen without
        				// adding a name but
        				// entered other information worth keeping.
        				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.NAME, name
        						.length() < 1 ? mRes.getString(R.string.untitled_pppoe_account)
        						: name);
        				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.USER, user);
        				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1, dns1);
        				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2, dns2);
        				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD,
        						password);

        				getContentResolver().update(mUri, values, null, null);
        				}
        				finish();
        			 }
       		 	 }  		
        		)
        		.show();
        	return true;
		}
		}
		return super.onKeyDown(keyCode, event);
	}
	//$_rbox_$_modify_$_end_$_hhq_$_20120616_$


	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
			String key) {
		Preference pref = findPreference(key);
		if (pref != null) {
			if (pref.equals(mPassword)) {
				pref.setSummary(starify(sharedPreferences.getString(key, "")));
			} else {
				pref.setSummary(checkNull(sharedPreferences.getString(key, "")));
			}
		}
	}

	private String starify(String value) {
		if (value == null || value.length() == 0) {
			return sNotSet;
		} else {
			char[] password = new char[value.length()];
			for (int i = 0; i < password.length; i++) {
				password[i] = '*';
			}
			return new String(password);
		}
	}

	private String checkNull(String value) {
		if (value == null || value.length() == 0) {
			return sNotSet;
		} else {
			return value;
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		// If it's a new pppoe account, then cancel will delete the new entry in onPause
		if (!mNewAccount) {
			menu.add(
					0,
					MENU_DELETE,
					0,
					getResources().getString(
							R.string.pppoe_accounts_menu_delete)).setIcon(
					android.R.drawable.ic_menu_delete);
		}
		menu.add(0, MENU_SAVE, 0,
				getResources().getString(R.string.pppoe_accounts_menu_save))
				.setIcon(android.R.drawable.ic_menu_save);
		menu.add(0, MENU_CANCEL, 0,
				getResources().getString(R.string.pppoe_accounts_menu_cancel))
				.setIcon(android.R.drawable.ic_menu_close_clear_cancel);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case MENU_DELETE:
			deleteAccount();
			return true;
		case MENU_SAVE:
			if (validateAndSave(false)) {
				finish();
			}
			return true;
		case MENU_CANCEL:
			if (mNewAccount) {
				getContentResolver().delete(mUri, null, null);
			}
			finish();
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	private void deleteAccount() {
		getContentResolver().delete(mUri, null, null);
		finish();
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue) {
		// TODO Auto-generated method stub
		return false;
	}
}
