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
package org.eclipse.equinox.jmx.internal.server;

import org.eclipse.osgi.util.NLS;

/**
 * @since 1.0
 */
public class ContributionMessages extends NLS {

	public static String desc_getcontribs;

	// exceptions
	public static String excep_typeprovider_exists;
	public static String excep_null_mbeanserver;
	public static String excep_null_server_on_createself;
	public static String excep_contrib_reg_with_diff_server;
	public static String excep_contrib_delegate_exists;

	// disallow instantiations
	private ContributionMessages() {
		super();
	}

	static {
		NLS.initializeMessages(ContributionMessages.class.getName(), ContributionMessages.class);
	}
}
