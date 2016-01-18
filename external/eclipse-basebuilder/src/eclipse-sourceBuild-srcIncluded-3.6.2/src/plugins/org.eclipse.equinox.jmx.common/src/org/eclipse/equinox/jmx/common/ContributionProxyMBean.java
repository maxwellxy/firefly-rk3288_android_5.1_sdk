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
package org.eclipse.equinox.jmx.common;

import java.util.Set;
import javax.management.MBeanInfo;
import javax.management.ObjectName;

public interface ContributionProxyMBean {

	/**
	 * Get the <code>ObjectName</code> for the resource
	 * which the implementing <code>ContributionProxy</code>
	 * is a proxy for.
	 * 
	 * @return The <code>ObjectName</code> which identifies this resource on the server.
	 */
	public ObjectName getObjectName();

	/**
	 * A concise name which is used to identify this contribution in the UI.
	 * 
	 * @return The name of this contribution.
	 */
	public String getName();

	/**
	 * Return the <code>MBeanInfo</code> which describes the operations this contribution
	 * exposes to clients.
	 * 
	 * @return The <code>MBeanInfo</code> associated with this contribution.
	 */
	public MBeanInfo getMBeanInfo();

	/**
	 * Get the properties of which describe the contribution. 
	 * The element[i][1..n] entries are to be interpreted as 
	 * values for the property name at element[i][0].
	 * 
	 * @return The properties of the contribution to be displayed in the ui.
	 */
	public Set getContributionProperties();
}
