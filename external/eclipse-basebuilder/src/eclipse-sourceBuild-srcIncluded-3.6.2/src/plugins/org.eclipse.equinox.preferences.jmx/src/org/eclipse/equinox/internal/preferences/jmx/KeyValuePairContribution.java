/*******************************************************************************
 * Copyright (c) 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials 
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 * 	Jeff Mesnil - Bug 151266 - [monitoring] Browsing our server in jconsole is really unfriendly
 *     IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.equinox.internal.preferences.jmx;

import java.net.URL;
import java.util.*;
import javax.management.*;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.equinox.jmx.common.JMXConstants;
import org.eclipse.equinox.jmx.server.Contribution;
import org.osgi.service.prefs.Preferences;

/**
 * @since 1.0
 */
public class KeyValuePairContribution extends Contribution {

	private static final String KVP_IMAGE_PATH = "icons/kvp.gif"; //$NON-NLS-1$
	private static final String METHOD_REMOVE = "remove"; //$NON-NLS-1$

	public KeyValuePairContribution(KVP delegate) {
		super(delegate);
	}

    /* (non-Javadoc)
     * @see org.eclipse.equinox.jmx.server.Contribution#getObjectName()
     */
    protected ObjectName getObjectName() {
        try {
            return new ObjectName(JMXConstants.DEFAULT_DOMAIN + ":type=KVPair,Preference=" + getDelegate().getNode().name() //$NON-NLS-1$
            		+ "name=" + getDelegate().getKey()); //$NON-NLS-1$
        } catch (Exception e) {
            return super.getObjectName();
        }
    }

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return getDelegate().getKey() + '=' + getDelegate().getValue();
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
		return new MBeanInfo(getClass().getName(), null, null, null, createOperations(), null);
	}

	/*
	 * Return the list of operations available to this contribution.
	 */
	private MBeanOperationInfo[] createOperations() {
		List result = new ArrayList();
		result.add(createRemoveOperation());
		return (MBeanOperationInfo[]) result.toArray(new MBeanOperationInfo[result.size()]);
	}

	/*
	 * Create and return an operation to delete this key/value pair.
	 */
	private MBeanOperationInfo createRemoveOperation() {
		return new MBeanOperationInfo(METHOD_REMOVE, Messages.operation_removeKVP, null, Void.TYPE.getName(), 0);
	}

	/*
	 * Return this contribution's delegate.
	 */
	private KVP getDelegate() {
		return (KVP) contributionDelegate;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		if (METHOD_REMOVE.equals(operationName)) {
			Preferences node = getDelegate().getNode();
			node.remove(getDelegate().getKey());
		}
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
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return FileLocator.find(Activator.getContext().getBundle(), new Path(KVP_IMAGE_PATH), null);
	}
}
