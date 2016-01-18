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
package org.eclipse.equinox.internal.preferences.jmx;

import org.eclipse.osgi.util.NLS;

/**
 * @since 1.0
 */
public class Messages extends NLS {
	public static String preferenceContributionName;
	public static String kvpContributionName;

	public static String operation_addChild;
	public static String operation_put;
	public static String operation_removeNode;
	public static String operation_removeKVP;

	public static String parm_childName_desc;
	public static String parm_key_desc;
	public static String parm_value_desc;

	private Messages() {
		// prevent instantiation
	}

	// initialize resource bundle
	static {
		NLS.initializeMessages("org.eclipse.equinox.internal.preferences.jmx.messages", Messages.class); //$NON-NLS-1$
	}

}
