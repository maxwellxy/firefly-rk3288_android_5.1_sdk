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
package org.eclipse.equinox.jmx.internal.client.ui.contributionsview;

import org.eclipse.osgi.util.NLS;

/**
 * @since 1.0
 */
public class ContributionViewMessages extends NLS {

	public static String operations;
	public static String error_message;

	// disallow instantiations
	private ContributionViewMessages() {
		super();
	}

	static {
		NLS.initializeMessages(ContributionViewMessages.class.getName(), ContributionViewMessages.class);
	}
}
