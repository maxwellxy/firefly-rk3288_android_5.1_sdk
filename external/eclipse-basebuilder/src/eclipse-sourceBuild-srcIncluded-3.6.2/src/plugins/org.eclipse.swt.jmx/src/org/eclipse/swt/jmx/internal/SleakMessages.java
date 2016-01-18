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
package org.eclipse.swt.jmx.internal;

import org.eclipse.osgi.util.NLS;

public class SleakMessages extends NLS {

	public static String name;
	public static String description;
	public static String start_monitoring;
	public static String stop_monitoring;
	public static String colors;
	public static String cursors;
	public static String fonts;
	public static String images;
	public static String regions;
	public static String gcs;
	public static String poll_interval_desc;

	private SleakMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages(SleakMessages.class.getName(), SleakMessages.class);
	}

}
