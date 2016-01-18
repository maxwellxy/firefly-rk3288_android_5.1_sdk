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
import java.net.MalformedURLException;
import java.util.Map;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXServiceURL;
import org.eclipse.equinox.jmx.client.IJMXConnectorProvider;

public class XMLRPCJMXConnectorProvider implements IJMXConnectorProvider {

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorProvider#newJMXConnector(javax.management.remote.JMXServiceURL, java.util.Map)
	 */
	public JMXConnector newJMXConnector(JMXServiceURL arg0, Map arg1) throws IOException {
		if (!arg0.getProtocol().equals("xmlrpc")) {
			throw new IOException("Invalid protocol"); // FIXME
		}
		return new XMLRPCJMXConnector(arg0.getHost(), arg0.getPort());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.client.IJMXConnectorProvider#getJMXServiceURL(java.lang.String, int, java.lang.String, java.lang.String)
	 */
	public JMXServiceURL getJMXServiceURL(String host, int port, String protocol, String domain) throws MalformedURLException {
		return new JMXServiceURL("service:jmx:xmlrpc://" + host + ":" + port);
	}
}
