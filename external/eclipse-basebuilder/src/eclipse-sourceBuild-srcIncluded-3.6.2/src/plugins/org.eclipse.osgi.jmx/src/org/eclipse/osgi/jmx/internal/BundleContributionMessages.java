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
package org.eclipse.osgi.jmx.internal;

import org.eclipse.osgi.util.NLS;

/**
 * @since 1.0
 */
public class BundleContributionMessages extends NLS {

	public static String bundle_contribution_name;
	public static String service_bundle_contribution_name;
	public static String bundle_description;
	public static String start_operation_desc;
	public static String stop_operation_desc;
	public static String install_operation_desc;
	public static String uninstall_operation_desc;
	public static String bundle_url_desc;
	public static String excep_null_install_url;

	private BundleContributionMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages(BundleContributionMessages.class.getName(), BundleContributionMessages.class);
	}
}
