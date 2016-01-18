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
package org.eclipse.equinox.jmx.internal.client.ui.mbeaninfoview;

import org.eclipse.osgi.util.NLS;

/**
 * @since 1.0
 */
public class MBeanInfoViewMessages extends NLS {

	public static String invoke_result;
	public static String contribution_name;
	public static String description;
	public static String mbean_description;
	public static String error;
	public static String button_invoke;
	public static String MBeanOpTable_returnType;
	public static String MBeanOpTable_name;
	public static String MBeanOpTable_params;
	public static String InvocationView_0;
	public static String MBeanInfoViewPart_0;
	public static String MBeanInfoViewPart_1;
	public static String MBeanInfoViewPart_6;
	public static String MBeanInfoViewPart_7;
	public static String MBeanInfoViewPart_8;
	public static String MBeanInfoViewPart_11;

	private MBeanInfoViewMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages(MBeanInfoViewMessages.class.getName(), MBeanInfoViewMessages.class);
	}

}
