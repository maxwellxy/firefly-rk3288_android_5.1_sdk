/*******************************************************************************
 * Copyright (c) 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     Red Hat, Inc. and IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.equinox.internal.initializer;

import org.eclipse.osgi.util.NLS;

public class Messages {
	private static final String BUNDLE_NAME = "org.eclipse.equinox.internal.initializer.messages"; //$NON-NLS-1$

	// initializer
	public static String initializer_error;

	// file initializer
	public static String fileInitializer_fileNotFound;
	public static String fileInitializer_IOError;
	public static String fileInitializer_missingFileName;

	static {
		// load message values from bundle file
		reloadMessages();
	}

	public static void reloadMessages() {
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}
}
