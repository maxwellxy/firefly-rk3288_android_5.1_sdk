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
public class ServiceContributionMessages extends NLS {

	public static String service_contribution_name;
	public static String bundle_service_contribution_name;
	public static String bundleservice_contribution_name;
	public static String controlling_bundle_stopped;

	private ServiceContributionMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages(ServiceContributionMessages.class.getName(), ServiceContributionMessages.class);
	}
}
