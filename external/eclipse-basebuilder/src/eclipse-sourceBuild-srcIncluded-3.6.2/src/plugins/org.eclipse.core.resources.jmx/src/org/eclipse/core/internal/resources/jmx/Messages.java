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
package org.eclipse.core.internal.resources.jmx;

import org.eclipse.osgi.util.NLS;

/**
 * Class which is used in the translation of messages.
 * 
 * @since 1.0
 */
public class Messages extends NLS {

	public static String contributionName;
	public static String rootName;
	public static String operation_delete;

	private Messages() {
		// prevent instantiation
	}

	// initialize resource bundle
	static {
		NLS.initializeMessages("org.eclipse.core.internal.resources.jmx.messages", Messages.class); //$NON-NLS-1$
	}
}
