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
package org.eclipse.equinox.jmx.internal.common;

import org.eclipse.osgi.util.NLS;

public class CommonMessages extends NLS {

	public static String invalid_server_type;
	public static String contribution_instance_not_found;
	public static String exception_occurred;

	private CommonMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages("org.eclipse.equinox.jmx.common.CommonMessages", CommonMessages.class); //$NON-NLS-1$
	}
}
