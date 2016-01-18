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
package org.eclipse.equinox.jmx.client.rmi;

import java.io.IOException;
import java.net.MalformedURLException;
import java.util.Map;
import javax.management.remote.*;
import org.eclipse.equinox.jmx.client.IJMXConnectorProvider;

public class RMIConnectorProvider implements IJMXConnectorProvider {

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.client.IJMXConnectorProvider#getJMXServiceURL(java.lang.String, int, java.lang.String, java.lang.String)
	 */
	public JMXServiceURL getJMXServiceURL(String host, int port, String protocol, String domain) throws MalformedURLException {
		return new JMXServiceURL("service:jmx:rmi:///jndi/rmi://" + host + ":" + port + "/" + domain);
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorProvider#newJMXConnector(javax.management.remote.JMXServiceURL, java.util.Map)
	 */
	public JMXConnector newJMXConnector(JMXServiceURL arg0, Map arg1) throws IOException {
		return JMXConnectorFactory.newJMXConnector(arg0, arg1);
	}
}
