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
package org.eclipse.equinox.internal.preferences.jmx;

import java.net.URL;
import java.util.Set;
import javax.management.*;
import org.eclipse.equinox.jmx.server.Contribution;
import org.eclipse.equinox.jmx.server.ContributionProvider;

/**
 * @since 1.0
 */
public class KVPContributionProvider extends ContributionProvider {

	public KVPContributionProvider() {
		super();
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createContribution(java.lang.Object)
	 */
	protected Contribution createContribution(Object obj) {
		return contributesType(obj) ? new KeyValuePairContribution((KVP) obj) : null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createProvider(java.lang.Object)
	 */
	protected ContributionProvider createProvider(Object obj) {
		return providesType(obj) ? new KVPContributionProvider() : null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return Messages.kvpContributionName;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getProperties()
	 */
	protected Set getProperties() {
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return new MBeanInfo(getClass().getName(), null, null, null, null, null);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		// do nothing
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttribute(java.lang.String)
	 */
	public Object getAttribute(String attribute) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute attribute) {
		// do nothing
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
	 */
	public AttributeList getAttributes(String[] attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
	 */
	public AttributeList setAttributes(AttributeList attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#contributesType(java.lang.Object)
	 */
	protected boolean contributesType(Object obj) {
		return obj instanceof KVP;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#providesType(java.lang.Object)
	 */
	protected boolean providesType(Object obj) {
		return obj instanceof KVP;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return null;
	}
}
