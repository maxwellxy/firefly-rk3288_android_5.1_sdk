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
package org.eclipse.core.internal.resources.jmx;

import java.net.URL;
import java.util.Set;
import javax.management.*;
import org.eclipse.core.resources.*;
import org.eclipse.equinox.jmx.server.Contribution;
import org.eclipse.equinox.jmx.server.ContributionProvider;

/**
 * @since 1.0
 */
public class ResourceContributionProvider extends ContributionProvider {

	private static final String ICON_PATH = "icons/resources.gif"; //$NON-NLS-1$

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#contributesType(java.lang.Object)
	 */
	protected boolean contributesType(Object obj) {
		return obj instanceof IResource;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#providesType(java.lang.Object)
	 */
	protected boolean providesType(Object obj) {
		return false;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createProvider(java.lang.Object)
	 */
	protected ContributionProvider createProvider(Object obj) {
		return providesType(obj) ? new ResourceContributionProvider() : null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createContribution(java.lang.Object)
	 */
	protected Contribution createContribution(Object obj) {
		if (!contributesType(obj))
			return null;
		IResource resource = (IResource) obj;
		switch (resource.getType()) {
			case IResource.FILE :
				return new FileContribution((IFile) obj);
			case IResource.FOLDER :
				return new FolderContribution((IFolder) obj);
			case IResource.PROJECT :
				return new ProjectContribution((IProject) obj);
			case IResource.ROOT :
				return null;
			default :
				throw new IllegalArgumentException("Resource type unknown: " + resource.getType()); //$NON-NLS-1$
		}
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return Messages.contributionName;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return ResourcesPlugin.getWorkspace().getRoot().getProjects();
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
		return Activator.getImageLocation(ICON_PATH);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return new MBeanInfo(getClass().getName(), null, null, null, createOperations(), null);
	}

	protected MBeanOperationInfo[] createOperations() {
		return new MBeanOperationInfo[0];
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
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
