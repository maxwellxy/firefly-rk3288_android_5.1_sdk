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
package org.eclipse.equinox.jmx.internal.client;

import java.io.IOException;
import javax.management.*;
import org.eclipse.equinox.jmx.common.ContributionProxy;
import org.eclipse.equinox.jmx.common.RootContribution;

/**
 * Provides a more generic and user-friendly interface
 * to the operations exposed by the jmx server.
 * 
 * @since 1.0
 */
public class MBeanServerProxy {

	private final MBeanServerConnection server;

	/**
	 * Allocate a <code>ServiceContributionProxy</code> which interfaces with the provided <code>server</code>.
	 * 
	 * @param server The <code>JMXConnectorServer</code> this proxy interfaces with.
	 */
	public MBeanServerProxy(MBeanServerConnection server) {
		this.server = server;
	}

	/**
	 * Get the <code>MBeanServerConnection</code> associated with this proxy.
	 * 
	 * @return The <code>MBeanServerConnection</code>.
	 */
	public MBeanServerConnection getMBeanServerConnection() {
		return server;
	}

	/**
	 * Convenience method for retrieving the <code>ContributionProxy</code> objects which form the root level
	 * of the contribution view.
	 * 
	 * @return The root <code>ContributionProxy</code> objects.
	 * @throws InstanceNotFoundException
	 * @throws MBeanException
	 * @throws ReflectionException
	 * @throws IOException
	 */
	public ContributionProxy[] getRootContributions() throws InstanceNotFoundException, MBeanException, ReflectionException, IOException {
		Object result = getMBeanServerConnection().invoke(RootContribution.OBJECT_NAME, RootContribution.OP_GET_ROOT_CONTRIBUTIONS, null, null);
		if (result instanceof ContributionProxy[]) {
			return (ContributionProxy[]) result;
		} else if (result instanceof Object[]) {
			Object objProxies[] = (Object[]) result;
			ContributionProxy[] proxies = new ContributionProxy[objProxies.length];
			for (int i = 0; i < objProxies.length; i++) {
				if (objProxies[i] instanceof ContributionProxy) {
					proxies[i] = (ContributionProxy) objProxies[i];
				}
			}
			return proxies;
		}
		return null;
	}

	public RootContribution getRootContribution() throws InstanceNotFoundException, MBeanException, ReflectionException, IOException {
		return (RootContribution) getMBeanServerConnection().invoke(RootContribution.OBJECT_NAME, RootContribution.OP_GET_ROOT_CONTRIBUTION, null, null);
	}

	/**
	 * Convenience method for invoking a method on the <code>Contribution</code> resource on the server.  The object name
	 * of the <code>Contribution</code> mbean is contained within the <code>ContributionProxy</code> that is supplied
	 * as an argument.
	 * 
	 * @param contribution The <code>ContributionProxy</code> which provides the <code>ObjectName</code> of the <code>Contribution</code>
	 * mbean to invoke.
	 * @param methodName The name of the method to invoke.
	 * @param args The list of arguments to the method to invoke.
	 * @param argsSignature The list of argument types that are provided to the method to invoke.
	 * @return The <code>Object</code> returned by the method or null if none.
	 * @throws InstanceNotFoundException
	 * @throws MBeanException
	 * @throws ReflectionException
	 * @throws IOException
	 * @throws NotCompliantMBeanException
	 */
	public Object invokeContributionOperation(ContributionProxy contribution, String methodName, Object[] args, String[] argsSignature) throws InstanceNotFoundException, MBeanException, ReflectionException, IOException, NotCompliantMBeanException {
		return getMBeanServerConnection().invoke(contribution.getObjectName(), methodName, args, argsSignature);
	}
}