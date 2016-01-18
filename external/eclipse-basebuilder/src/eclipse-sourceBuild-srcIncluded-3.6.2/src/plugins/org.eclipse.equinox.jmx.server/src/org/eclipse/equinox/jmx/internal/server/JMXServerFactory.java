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

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import javax.management.MBeanServer;
import javax.management.MBeanServerFactory;
import javax.management.remote.JMXConnectorServer;
import javax.management.remote.JMXServiceURL;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.server.IJMXConnectorServerProvider;
import org.eclipse.osgi.util.NLS;

public class JMXServerFactory {

	private static final String ATTRIBUTE_CLASS = "class"; //$NON-NLS-1$
	private static final String ATTRIBUTE_INITIALIZER_CLASS = "initializerClass"; //$NON-NLS-1$
	private static final String ATTRIBUTE_PROTOCOL = "protocol"; //$NON-NLS-1$

	private static Map jmxProviderCache;

	private JMXServerFactory() {
		super();
	}

	/**
	 * Create and return a new JMXConnectorServer instance with the given parameters.
	 * Throw an exception if there is an error or if the given protocol doesn't have a valid 
	 * registered transport extension.
	 */
	public static JMXConnectorServer createJMXServer(String host, int port, String protocol, String domain, Map environment) throws IOException {
		IJMXConnectorServerProvider provider = getProvider(protocol);
		if (provider == null)
			throw new IOException(NLS.bind(ServerMessages.protocol_not_available, protocol));
		MBeanServer mbeanServer = MBeanServerFactory.createMBeanServer(domain);
		JMXServiceURL providerURL = provider.getJMXServiceURL(host, port, protocol, domain);
		if (providerURL == null)
			providerURL = new JMXServiceURL(protocol, host, port);
		return provider.newJMXConnectorServer(providerURL, environment, mbeanServer);
	}

	private static IJMXConnectorServerProvider getProvider(String protocol) {
		final Class providerClass = (Class) getJMXProviderCache().get(protocol);
		if (providerClass != null) {
			try {
				return (IJMXConnectorServerProvider) providerClass.newInstance();
			} catch (Exception e) {
				Activator.log(e);
			}
		}
		return null;
	}

	private static void loadProviderExtensions() {
		IExtensionPoint point = RegistryFactory.getRegistry().getExtensionPoint(Activator.PI_NAMESPACE, Activator.PT_PROVIDER);
		IExtension[] types = point.getExtensions();
		for (int i = 0; i < types.length; i++) {
			loadProviderConfigurationElements(types[i].getConfigurationElements());
		}
	}

	private static void loadProviderConfigurationElements(IConfigurationElement[] configElems) {
		for (int j = 0; j < configElems.length; j++) {
			IConfigurationElement element = configElems[j];
			final String elementName = element.getName();
			String className, protocol;
			if (elementName.equals(Activator.PT_PROVIDER) && null != (className = element.getAttribute(ATTRIBUTE_CLASS)) && null != (protocol = element.getAttribute(ATTRIBUTE_PROTOCOL))) {
				try {
					// attempt to load initializer before instantiating provider class
					String initializer = element.getAttribute(ATTRIBUTE_INITIALIZER_CLASS);
					if (initializer != null)
						element.createExecutableExtension(ATTRIBUTE_INITIALIZER_CLASS);
					Object obj = element.createExecutableExtension(ATTRIBUTE_CLASS);
					// cache provider class name for protocol
					if (jmxProviderCache.containsKey(protocol)) {
						Activator.log(NLS.bind(ServerMessages.duplicate_protocol_provider, className));
						continue;
					}
					jmxProviderCache.put(protocol, obj.getClass());
				} catch (CoreException e) {
					Activator.log(e);
				}
			}
		}
	}

	private static Map getJMXProviderCache() {
		if (jmxProviderCache == null) {
			jmxProviderCache = new HashMap();
			loadProviderExtensions();
			RegistryFactory.getRegistry().addRegistryChangeListener(new JMXProviderExtensionListener(), Activator.PI_NAMESPACE + "." + Activator.PT_PROVIDER);
		}
		return jmxProviderCache;
	}

	private static class JMXProviderExtensionListener implements IRegistryChangeListener {

		/* (non-Javadoc)
		 * @see org.eclipse.core.runtime.IRegistryChangeListener#registryChanged(org.eclipse.core.runtime.IRegistryChangeEvent)
		 */
		public void registryChanged(IRegistryChangeEvent event) {
			IExtensionDelta[] deltas = event.getExtensionDeltas(Activator.PI_NAMESPACE, Activator.PT_PROVIDER);
			for (int i = 0; i < deltas.length; i++) {
				IExtensionDelta delta = deltas[i];
				loadProviderConfigurationElements(delta.getExtensionPoint().getConfigurationElements());
			}
		}
	}
}
