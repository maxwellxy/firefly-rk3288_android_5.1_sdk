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

public class BundleMessages extends NLS {

	public static String requires_name;
	public static String packages_name;

	private BundleMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages(BundleMessages.class.getName(), BundleMessages.class);
	}
}