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
package org.eclipse.equinox.jmx.internal.client.ui.actions;

import org.eclipse.osgi.util.NLS;

/**
 * @since 1.0
 */
public class ActionMessages extends NLS {

	public static String disconnect_action;
	public static String openconnection_action;
	public static String connection_closed;
	public static String server_closed_connection;
	public static String error_message;
	public static String info_message;
	public static String local_mbean_server;
	public static String host;
	public static String port;
	public static String invalid_host;
	public static String invalid_port;
	public static String no_url_selected;
	public static String add_jmx_service_url;
	public static String url_exists;
	public static String remove_jmx_service_url;
	public static String connect_host;
	public static String connection_selection_dialog_title;
	public static String no_transports_available;

	// disallow instantiations
	private ActionMessages() {
		super();
	}

	static {
		NLS.initializeMessages(ActionMessages.class.getName(), ActionMessages.class);
	}
}