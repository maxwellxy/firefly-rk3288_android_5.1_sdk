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

import org.apache.xmlrpc.client.*;

/**
 * Interface for implementors that wish to provider their own xml-rpc transport layer.
 */
public abstract class ClientTransportProvider {

	/**
	 * Implementor's are required to supply their specific transport layer
	 * factory which is used by the client to communicate with the server.
	 * 
	 * @return An instance of <code>XmlRpcTransportFactory<code>.
	 */
	public abstract XmlRpcTransportFactory getXmlRpcTransportFactory(XmlRpcClient client);

	public abstract XmlRpcClientConfigImpl getConfig();

	/**
	 * Return the xml-rpc client configured with the implementor's
	 * transport layer.
	 * 
	 * @return A new <code>XmlRpcClient</code>
	 */
	public XmlRpcClient getXmlRpcClient() {
		XmlRpcClient result = new XmlRpcClient();
		result.setConfig(getConfig());
		result.setTransportFactory(getXmlRpcTransportFactory(result));
		return result;
	}
}
