/*******************************************************************************
 * Copyright (c) 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.equinox.jmx.internal.vm;

import org.eclipse.osgi.util.NLS;

public class VMStatsMessages extends NLS {

	public static String title;

	// memory mbean data
	public static String mem_title;
	public static String mem_heapusage;
	public static String mem_noheapusage;
	public static String mem_commited;
	public static String mem_initreq;
	public static String mem_max;
	public static String mem_used;

	// classloading mbean data
	public static String cl_title;
	public static String cl_loadedclscnt;
	public static String cl_totloadedclscnt;
	public static String cl_totunloadedclscnt;

	// operating system mbean data
	public static String os_title;
	public static String os_arch;
	public static String os_ncpus;
	public static String os_name;
	public static String os_version;

	// compilation mbean data
	public static String cmp_title;
	public static String cmp_tottime;
	public static String cmp_jitname;

	// runtime mbean data
	public static String rt_title;
	public static String rt_bootclasspath;
	public static String rt_systemclasspath;
	public static String rt_inputargs;
	public static String rt_ldpath;
	public static String rt_mgmtspecver;
	public static String rt_vmname;
	public static String rt_vmvendor;
	public static String rt_vmversion;
	public static String rt_starttime;
	public static String rt_uptime;

	private VMStatsMessages() {
		// prevent instantiation
	}

	static {
		NLS.initializeMessages(VMStatsMessages.class.getName(), VMStatsMessages.class);
	}

}
