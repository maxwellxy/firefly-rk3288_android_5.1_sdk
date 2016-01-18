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
package org.eclipse.core.internal.resources.jmx;

import java.net.URL;
import java.util.*;
import javax.management.*;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IPath;
import org.eclipse.equinox.jmx.common.JMXConstants;
import org.eclipse.equinox.jmx.server.Contribution;

/**
 * Class which represents an {@link IResource} as a contribution.
 * 
 * @since 1.0
 */
public abstract class ResourceContribution extends Contribution {

	private static final String METHOD_DELETE = "delete"; //$NON-NLS-1$

	/*
	 * Constructor for the class. Create a new contribution for the given
	 * resource.
	 */
	public ResourceContribution(IResource delegate) {
		super(delegate);
	}

    /* (non-Javadoc)
     * @see org.eclipse.equinox.jmx.server.Contribution#getObjectName()
     */
    protected ObjectName getObjectName() {
        try {
            return new ObjectName(JMXConstants.DEFAULT_DOMAIN +":type=Resource,name=" + getDelegate().getFullPath()); //$NON-NLS-1$
        } catch (Exception e) {
            return super.getObjectName();
        }
    }

	/*
	 * Return this contribution's delegate. It is stored in the super-class as
	 * an Object so we need to cast it to our specific type.
	 */
	private IResource getDelegate() {
		return (IResource) contributionDelegate;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return getDelegate().getName();
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getProperties()
	 */
	protected Set getProperties() {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return Activator.getImageLocation(getIconPath().toOSString());
	}

	/**
	 * Return a path to the icon used to represent this contributions. 
	 * 
	 * @return a path in the file-system to the icon
	 */
	protected abstract IPath getIconPath();

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		List list = createOperations();
		MBeanOperationInfo[] operations = (MBeanOperationInfo[]) list.toArray(new MBeanOperationInfo[list.size()]);
		return new MBeanInfo(getClass().getName(), null, null, null, operations, null);
	}

	/*
	 * Return the list of operations known to this contribution type.
	 * Sub-classes to override. Should call "super" as well. 
	 */
	protected List createOperations() {
		List result = new ArrayList();
		result.add(new MBeanOperationInfo(METHOD_DELETE, Messages.operation_delete, null, Void.TYPE.getName(), 0));
		return result;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		if (METHOD_DELETE.equals(operationName)) {
			try {
				getDelegate().delete(IResource.FORCE, null);
			} catch (CoreException e) {
				Activator.log(e.getMessage(), e);
			}
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

}
