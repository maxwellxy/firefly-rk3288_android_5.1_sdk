package com.android.settings.pppoe;

import android.net.Uri;
import android.provider.BaseColumns;



public final class PppoeAccountsContentMeta {	
	
	public static final class PppoeAccountsColumns implements BaseColumns {
		
		public static final Uri CONTENT_URI = Uri.parse("content://pppoe/accounts");
		public static final Uri RESTORE_URI	= Uri.parse("content://pppoe/restore");
		
		public static final String DEFAULT_SORT_ORDER = "name ASC";
		public static final String ID 	= "_id";
		public static final String NAME	= "name";
		public static final String USER	= "user";
		public static final String DNS1 = "dns1";
		public static final String DNS2 = "dns2";
		public static final String PASSWORD = "password";
		
		public static final String PREFERRED_PPPOE_ID = "preferred_pppoe_id";	
	}
}
