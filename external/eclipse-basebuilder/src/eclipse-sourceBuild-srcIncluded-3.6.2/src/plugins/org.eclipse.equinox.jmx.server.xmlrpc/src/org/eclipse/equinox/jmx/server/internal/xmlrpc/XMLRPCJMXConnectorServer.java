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
package org.eclipse.equinox.jmx.server.internal.xmlrpc;

import java.io.IOException;
import java.util.Map;
import javax.management.MBeanServer;
import javax.management.remote.JMXConnectorServer;
import javax.management.remote.JMXServiceURL;

public class XMLRPCJMXConnectorServer extends JMXConnectorServer {

	private JMXServiceURL url;
	private XMLRPCMBeanServerAdapter xmlrpcServer;

	public XMLRPCJMXConnectorServer(JMXServiceURL url, MBeanServer mbeanServer) {
		this.url = url;
		xmlrpcServer = new XMLRPCMBeanServerAdapter(url.getPort(), mbeanServer);
		super.setMBeanServerForwarder(xmlrpcServer);
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerMBean#start()
	 */
	public synchronized void start() throws IOException {
		if (!xmlrpcServer.isActive()) {
			xmlrpcServer.start();
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerMBean#stop()
	 */
	public synchronized void stop() throws IOException {
		xmlrpcServer.stop();
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerMBean#isActive()
	 */
	public synchronized boolean isActive() {
		return xmlrpcServer.isActive();
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerMBean#getAddress()
	 */
	public JMXServiceURL getAddress() {
		return url;
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerMBean#getAttributes()
	 */
	public Map getAttributes() {
		return null;
	}
}
