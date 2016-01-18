package com.android.settings.pppoe;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.util.Config;
import android.util.Log;

public class PppoeAccountsContentProvider extends ContentProvider {

	private static final String TAG = "pppoeAccountsContentProvider";

	public static final String DATABASE_NAME = "pppoe.db";

	public static final String TABLE_NAME = "accounts";

	public static final int DATABASE_VERSION = 1;

	private static final String PREF_FILE = "preferred-pppoeaccount";

	private static final int ACCOUNTS = 1;
	private static final int ACCOUNTS_ID = 2;
	private static final int ACCOUNTS_RESTORE = 3;
	private static final int ACCOUNTS_PREFERPPPOE = 4;

	// uri matcher
	private static final UriMatcher sUriMatcher;

	static {
		sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
		sUriMatcher.addURI("pppoe", "accounts", ACCOUNTS);
		sUriMatcher.addURI("pppoe", "accounts/#", ACCOUNTS_ID);
		sUriMatcher.addURI("pppoe", "restore", ACCOUNTS_RESTORE);
		sUriMatcher.addURI("pppoe", "accounts/preferaccount",
				ACCOUNTS_PREFERPPPOE);
	}

	private static class DataBaseHelper extends SQLiteOpenHelper {

		public DataBaseHelper(Context context) {
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			db.execSQL("CREATE TABLE " + TABLE_NAME
					+ "(_id INTEGER PRIMARY KEY," + "name TEXT," + "user TEXT,"
					+ "dns1 TEXT," + "dns2 TEXT," + "password TEXT);");

			initDatabase(db);
		}

		private void initDatabase(SQLiteDatabase db) {
			Log.d(TAG, "pppoe content provider init");
			// TODO Auto-generated method stub
			ContentValues values = new ContentValues();
/*
			values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.NAME,
					"armer");
			values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.USER,
					"heha");
			values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1,
					"dns1");
			values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2,
					"dns2");
			values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD,
					"123412");
			db.insert(TABLE_NAME, null, values);
*/
		}

		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			db.execSQL("DROP TABLE IF EXISTS" + TABLE_NAME);
			onCreate(db);
		}
	}

	private SQLiteOpenHelper mOpenHelper = null;

	@Override
	public boolean onCreate() {
		mOpenHelper = new DataBaseHelper(getContext());
		return true;
	}

	private void setPreferredPppoeId(Long id) {
		Log.d(TAG, "setPreferredPppoeId");
		SharedPreferences sp = getContext().getSharedPreferences(PREF_FILE,
				Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = sp.edit();
		editor.putLong(
				PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID,
				id != null ? id.longValue() : -1);
		editor.apply();
	}

	private long getPreferredPppoeId() {
		SharedPreferences sp = getContext().getSharedPreferences(PREF_FILE,
				Context.MODE_PRIVATE);
		return sp
				.getLong(
						PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID,
						-1);
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection,
			String[] selectionArgs, String sortOrder) {
		SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
		qb.setTables(TABLE_NAME);

		int match = sUriMatcher.match(uri);

		switch (match) {
		case ACCOUNTS: {
			break;
		}
		case ACCOUNTS_ID: {
			qb.appendWhere("_id = " + uri.getPathSegments().get(1));
			break;
		}

		case ACCOUNTS_PREFERPPPOE: {
			qb.appendWhere("_id = " + getPreferredPppoeId());
			break;
		}
		default:
			return null;
		}

		SQLiteDatabase db = mOpenHelper.getReadableDatabase();
		Cursor ret = qb.query(db, projection, selection, selectionArgs, null,
				null, sortOrder);
		ret.setNotificationUri(getContext().getContentResolver(), uri);
		return ret;
	}

	@Override
	public String getType(Uri uri) {
		switch (sUriMatcher.match(uri)) {
		case ACCOUNTS:
			return "vnd.android.cursor.dir/pppoe-accounts";

		case ACCOUNTS_ID:
			return "vnd.android.cursor.item/pppoe-accounts";

		case ACCOUNTS_PREFERPPPOE:
			return "vnd.android.cursor.item/pppoe-accounts";

		default:
			throw new IllegalArgumentException("Unknown URL " + uri);
		}
	}

	@Override
	public Uri insert(Uri uri, ContentValues initialValues) {
		Uri result = null;

		// checkPermission();
		SQLiteDatabase db = mOpenHelper.getWritableDatabase();
		int match = sUriMatcher.match(uri);
		boolean notify = false;
		switch (match) {
		case ACCOUNTS: {
			ContentValues values;
			if (initialValues != null) {
				values = new ContentValues(initialValues);
			} else {
				values = new ContentValues();
			}

			if (values
					.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.NAME) == false)
				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.NAME,
						"");
			if (values
					.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.USER) == false)
				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.USER,
						"");
			if (values
					.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1) == false)
				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS1,
						"");
			if (values
					.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2) == false)
				values.put(PppoeAccountsContentMeta.PppoeAccountsColumns.DNS2,
						"");
			if (values
					.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD) == false)
				values.put(
						PppoeAccountsContentMeta.PppoeAccountsColumns.PASSWORD,
						"");

			long rowId = db.insert(TABLE_NAME, null, values);
			if (rowId > 0) {
				result = ContentUris
						.withAppendedId(
								PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI,
								rowId);
				notify = true;
			}
			if (Config.LOGD)
				Log.d(TAG, "inserted " + values.toString() + " rowID = "
						+ rowId);
			break;
		}

		case ACCOUNTS_PREFERPPPOE: {
			if (initialValues != null) {
				if (initialValues
						.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID)) {
					setPreferredPppoeId(initialValues
							.getAsLong(PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID));
				}
			}
			break;
		}

		default:
			break;
		}
		if (notify) {
			getContext().getContentResolver().notifyChange(
					PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI,
					null);
		}

		return result;
	}

	@Override
	public int delete(Uri uri, String selection, String[] selectionArgs) {
		int count = 0;
		// checkPermission();

		SQLiteDatabase db = mOpenHelper.getWritableDatabase();
		int match = sUriMatcher.match(uri);
		switch (match) {
		case ACCOUNTS: {
			count = db.delete(TABLE_NAME, selection, selectionArgs);
			break;
		}

		case ACCOUNTS_ID: {
			count = db.delete(TABLE_NAME,
					PppoeAccountsContentMeta.PppoeAccountsColumns._ID + "=?",
					new String[] { uri.getLastPathSegment() });
			break;
		}

		case ACCOUNTS_RESTORE: {
			count = 1;
			restoreDefaultPppoeAccounts();
			break;
		}

		case ACCOUNTS_PREFERPPPOE: {
			setPreferredPppoeId((long) -1);
			count = 1;
			break;
		}

		default:
			throw new UnsupportedOperationException("Cannot delete that URL: "
					+ uri);
		}

		if (count > 0) {
			getContext().getContentResolver().notifyChange(
					PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI,
					null);
		}

		return count;

	}

	private void restoreDefaultPppoeAccounts() {
		SQLiteDatabase db = mOpenHelper.getWritableDatabase();

		db.delete(TABLE_NAME, null, null);
		setPreferredPppoeId((long) -1);
		((DataBaseHelper) mOpenHelper).initDatabase(db);
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection,
			String[] selectionArgs) {
		int count = 0;

		// checkPermission();
		SQLiteDatabase db = mOpenHelper.getWritableDatabase();
		int match = sUriMatcher.match(uri);
		switch (match) {
		case ACCOUNTS: {
			count = db.update(TABLE_NAME, values, selection, selectionArgs);
			break;
		}

		case ACCOUNTS_ID: {
			if (null != selection || null != selectionArgs) {
				throw new UnsupportedOperationException("Cannot update URL "
						+ uri + " with a where clause");
			}
			count = db.update(TABLE_NAME, values,
					PppoeAccountsContentMeta.PppoeAccountsColumns._ID + "=?",
					new String[] { uri.getLastPathSegment() });
			break;
		}

		case ACCOUNTS_PREFERPPPOE: {
			// debug
			Log.d(TAG, values.toString());
			if (values != null) {
				if (values
						.containsKey(PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID)) {
					setPreferredPppoeId(values
							.getAsLong(PppoeAccountsContentMeta.PppoeAccountsColumns.PREFERRED_PPPOE_ID));
					count = 1;
				}
			}
			break;
		}

		default:
			throw new UnsupportedOperationException("Cannot update that URL: "
					+ uri);
		}

		if (count > 0) {
			getContext().getContentResolver().notifyChange(
					PppoeAccountsContentMeta.PppoeAccountsColumns.CONTENT_URI,
					null);
		}

		return count;

	}

}
