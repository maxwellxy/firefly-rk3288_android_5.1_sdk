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
package org.eclipse.equinox.jmx.internal.rmi;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.rmi.RMISecurityManager;
import java.rmi.registry.LocateRegistry;
import java.util.Map;
import javax.management.MBeanServer;
import javax.management.remote.*;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.server.IJMXConnectorServerProvider;

public class RMIServer implements IJMXConnectorServerProvider {

	private int port;

	public JMXServiceURL getJMXServiceURL(String host, int port, String protocol, String domain) throws MalformedURLException {
		this.port = port;
		return new JMXServiceURL("service:jmx:rmi:///jndi/rmi://" + host + ":" + port + "/" + domain);
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.JMXConnectorServerProvider#newJMXConnectorServer(javax.management.remote.JMXServiceURL, java.util.Map, javax.management.MBeanServer)
	 */
	public JMXConnectorServer newJMXConnectorServer(JMXServiceURL arg0, Map arg1, MBeanServer arg2) throws IOException {
		initialize();
		return JMXConnectorServerFactory.newJMXConnectorServer(arg0, arg1, arg2);
	}

	private void initialize() throws IOException {
		// load the security policy from the bundle install location
		IPath serverPolicyFileName = new Path("server.policy"); //$NON-NLS-1$
		URL serverPolicyBundleURL = FileLocator.find(Activator.getBundleContext().getBundle(), serverPolicyFileName, null);
		if (serverPolicyBundleURL == null)
			throw new FileNotFoundException("Unable to find server policy file.");
		serverPolicyBundleURL = FileLocator.toFileURL(serverPolicyBundleURL);
		System.setProperty("java.security.policy", serverPolicyBundleURL.getFile()); //$NON-NLS-1$
		// must register port and rmi security manager
		LocateRegistry.createRegistry(port);
		System.setSecurityManager(new RMISecurityManager());
	}
}
