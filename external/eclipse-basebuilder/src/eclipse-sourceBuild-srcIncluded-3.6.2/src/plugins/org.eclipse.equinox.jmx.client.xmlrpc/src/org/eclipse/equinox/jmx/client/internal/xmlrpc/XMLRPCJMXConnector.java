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
package org.eclipse.equinox.jmx.client.internal.xmlrpc;

import java.io.IOException;
import java.net.URL;
import java.util.Map;
import javax.management.*;
import javax.management.remote.JMXConnector;
import javax.security.auth.Subject;

public class XMLRPCJMXConnector implements JMXConnector {

	private final String host;
	private final int port;
	private ClientTransportProvider clientProvider;
	private MBeanServerConnection mbeanServer;

	public XMLRPCJMXConnector(String host, int port) {
		this.host = host;
		this.port = port;
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#connect()
	 */
	public void connect() throws IOException {
		connect(null);
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#connect(java.util.Map)
	 */
	public void connect(Map arg0) throws IOException {
		clientProvider = new WebServerTransportProvider(new URL("http://" + host + ":" + port));
		mbeanServer = new XMLRPCMBeanServerConnection(clientProvider.getXmlRpcClient());
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#getMBeanServerConnection()
	 */
	public MBeanServerConnection getMBeanServerConnection() throws IOException {
		return mbeanServer;
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#getMBeanServerConnection(javax.security.auth.Subject)
	 */
	public MBeanServerConnection getMBeanServerConnection(Subject delegationSubject) throws IOException {
		return mbeanServer;
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#close()
	 */
	public void close() throws IOException {
		// the xmlrpc http transport creates a new connection for each remote operation
		// thus there is no global close ability associated with this provider
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#addConnectionNotificationListener(javax.management.NotificationListener, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void addConnectionNotificationListener(NotificationListener listener, NotificationFilter filter, Object handback) {
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#removeConnectionNotificationListener(javax.management.NotificationListener)
	 */
	public void removeConnectionNotificationListener(NotificationListener listener) throws ListenerNotFoundException {
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#removeConnectionNotificationListener(javax.management.NotificationListener, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void removeConnectionNotificationListener(NotificationListener l, NotificationFilter f, Object handback) throws ListenerNotFoundException {
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnector#getConnectionId()
	 */
	public String getConnectionId() throws IOException {
		return null;
	}
}
