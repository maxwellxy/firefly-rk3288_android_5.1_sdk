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
package org.eclipse.equinox.jmx.client;

import java.net.MalformedURLException;
import javax.management.remote.JMXConnectorProvider;
import javax.management.remote.JMXServiceURL;

public interface IJMXConnectorProvider extends JMXConnectorProvider {
	public JMXServiceURL getJMXServiceURL(String host, int port, String protocol, String domain) throws MalformedURLException;
}
