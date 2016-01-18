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

import javax.management.remote.JMXConnectorServer;

/**
 * Required interface for supported JMX server types.
 * 
 * @since 1.0
 */
public interface IJMXServer {

	/**
	 * Initialize the JMX server prior to startup.
	 * 
	 * @param host The hostname or ip address of the server.
	 * @param port The port to listen on.
	 * @param domain The jmx service url domain.
	 * @throws Exception If an exception occurs during initialization.
	 */
	public void initialize(String host, int port, String domain) throws Exception;

	/**
	 * Start the JMX Server.
	 * 
	 * @throws Exception If an exception occurs when attempting to start the server.
	 */
	public void start() throws Exception;

	/**
	 * Stop the JMX Server.
	 * 
	 * @throws Exception If an exception occurs when attempting to stop the server.
	 */
	public void stop() throws Exception;

	/**
	 * Get the <code>JMXConnectorServer</code> associated with the JMX server.
	 * 
	 * @return The <code>JMXConnectorServer</code>
	 */
	public JMXConnectorServer getJMXConnectorServer();
}
