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
import java.net.MalformedURLException;
import java.util.Map;
import javax.management.MBeanServer;
import javax.management.remote.JMXConnectorServer;
import javax.management.remote.JMXServiceURL;
import org.eclipse.equinox.jmx.server.IJMXConnectorServerProvider;

public class XMLRPCServer implements IJMXConnectorServerProvider {

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.IJMXConnectorServerProvider#getJMXServiceURL(java.lang.String, int, java.lang.String, java.lang.String)
	 */
	public JMXServiceURL getJMXServiceURL(String host, int port, String protocol, String domain) throws MalformedURLException {
		return new JMXServiceURL("service:jmx:xmlrpc://" + host + ":" + port + "/" + (domain == null ? "" : domain));
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerProvider#newJMXConnectorServer(javax.management.remote.JMXServiceURL, java.util.Map, javax.management.MBeanServer)
	 */
	public JMXConnectorServer newJMXConnectorServer(JMXServiceURL arg0, Map arg1, MBeanServer arg2) throws IOException {
		return new XMLRPCJMXConnectorServer(arg0, arg2);
	}
}
