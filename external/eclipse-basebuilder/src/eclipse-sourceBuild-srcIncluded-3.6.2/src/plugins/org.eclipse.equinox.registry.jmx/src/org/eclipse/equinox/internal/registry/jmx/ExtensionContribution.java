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
package org.eclipse.equinox.internal.registry.jmx;

import java.net.URL;
import java.util.Set;
import javax.management.*;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.server.Contribution;

/**
 * @since 3.2
 */
public class ExtensionContribution extends Contribution {

	public ExtensionContribution(Object obj) {
		super(obj);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return new Object[0];
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return FileLocator.find(Activator.getContext().getBundle(), new Path("icons/extension.gif"), null); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return null;
	}

	private IExtension getDelegate() {
		return (IExtension) contributionDelegate;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getName()
	 */
	protected String getName() {
		String name = getDelegate().getUniqueIdentifier();
		if (name != null)
			return name;
		StringBuffer buffer = new StringBuffer();
		buffer.append("anonymous "); //$NON-NLS-1$
		IContributor contributor = getDelegate().getContributor();
		if (contributor != null) {
			buffer.append('[');
			buffer.append(contributor.getName());
			buffer.append(']');
		}
		return buffer.toString();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getObjectName()
	 */
	protected ObjectName getObjectName() {
		try {
			return new ObjectName("jmxserver:type=Extension,ExtensionPoint=" + getDelegate().getExtensionPointUniqueIdentifier() //$NON-NLS-1$
					+ ",name=" + getName()); //$NON-NLS-1$
		} catch (Exception e) {
			return super.getObjectName();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getProperties()
	 */
	protected Set getProperties() {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttribute(java.lang.String)
	 */
	public Object getAttribute(String arg0) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
	 */
	public AttributeList getAttributes(String[] arg0) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute arg0) {
		//
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
	 */
	public AttributeList setAttributes(AttributeList arg0) {
		return null;
	}

}
