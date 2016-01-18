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

import java.net.URL;
import org.apache.xmlrpc.client.*;

public class WebServerTransportProvider extends ClientTransportProvider {

	private URL serverUrl;

	public WebServerTransportProvider(URL serverUrl) {
		this.serverUrl = serverUrl;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.client.internal.xmlrpc.ClientTransportProvider#getXmlRpcTransportFactory(org.apache.xmlrpc.client.XmlRpcClient)
	 */
	public XmlRpcTransportFactory getXmlRpcTransportFactory(XmlRpcClient client) {
		return new XmlRpcCommonsTransportFactory(client);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.client.internal.xmlrpc.ClientTransportProvider#getConfig()
	 */
	public XmlRpcClientConfigImpl getConfig() {
		XmlRpcClientConfigImpl result = new XmlRpcClientConfigImpl();
		result.setContentLengthOptional(true);
		result.setEnabledForExtensions(true);
		result.setServerURL(serverUrl);
		return result;
	}
}
