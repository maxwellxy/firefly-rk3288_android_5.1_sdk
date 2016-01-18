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
public final class ServerMessages extends NLS {

	public static String server_started;
	public static String server_stopped;
	public static String invalid_port;
	public static String invalid_jmx_server;
	public static String expected_non_null_jmxconnector;
	public static String duplicate_protocol_provider;
	public static String exception_occurred;
	public static String protocol_not_available;

	private ServerMessages() {
		// disallow instantiations
	}

	static {
		NLS.initializeMessages(ServerMessages.class.getName(), ServerMessages.class);
	}
}
