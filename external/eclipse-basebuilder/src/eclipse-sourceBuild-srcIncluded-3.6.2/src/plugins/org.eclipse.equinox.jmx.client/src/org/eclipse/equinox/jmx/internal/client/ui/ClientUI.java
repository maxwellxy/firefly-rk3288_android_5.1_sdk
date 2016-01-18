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
package org.eclipse.equinox.jmx.internal.client.ui;

/**
 * <p>
 * This class provides static methods and fields only; it is not intended to be
 * instantiated or subclassed by clients.
 * </p>
 * 
 * @since 1.0
 */
public final class ClientUI {

	// disallow instantiations
	private ClientUI() {
		super();
	}

	//view ids
	public static final String VIEWID_CONTRIBUTIONS = "org.eclipse.equinox.jmx.client.ui.contributionsview"; //$NON-NLS-1$
	public static final String VIEWID_MBEANINFO = "org.eclipse.equinox.jmx.client.ui.mbeaninfoview"; //$NON-NLS-1$
	public static final String VIEWID_INVOCATION = "org.eclipse.equinox.jmx.client.ui.invocationView"; //$NON-NLS-1$
}
