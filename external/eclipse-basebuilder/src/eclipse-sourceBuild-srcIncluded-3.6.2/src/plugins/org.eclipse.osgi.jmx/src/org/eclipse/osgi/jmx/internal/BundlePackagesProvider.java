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
package org.eclipse.osgi.jmx.internal;

import java.net.URL;
import java.util.Set;
import javax.management.*;
import org.eclipse.equinox.jmx.server.Contribution;
import org.eclipse.equinox.jmx.server.ContributionProvider;
import org.eclipse.osgi.service.resolver.BundleDescription;
import org.eclipse.osgi.service.resolver.ImportPackageSpecification;
import org.osgi.framework.Bundle;

/**
 * @since 1.0
 */
public class BundlePackagesProvider extends ContributionProvider {

	// the bundle being provided for
	private Bundle bundle;

	/**
	 * Default Constructor.
	 */
	public BundlePackagesProvider() {
		this(null);
	}

	/**
	 * Constructor specifying the bundle which this provider is associated with.
	 * 
	 * @param bundle The providers associated bundle.
	 */
	public BundlePackagesProvider(Bundle bundle) {
		this.bundle = bundle;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#contributesType(java.lang.Object)
	 */
	protected boolean contributesType(Object obj) {
		return obj instanceof ImportPackageSpecification;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#createContribution(java.lang.Object)
	 */
	protected Contribution createContribution(Object obj) throws MalformedObjectNameException {
		return contributesType(obj) ? new Contribution(obj) {

			/* (non-Javadoc)
			 * @see org.eclipse.equinox.jmx.server.Contribution#getChildren()
			 */
			protected Object[] getChildren() {
				return null;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
			 */
			protected URL getImageLocation() {
				return null;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.equinox.jmx.server.Contribution#getMBeanInfo(java.lang.Object)
			 */
			protected MBeanInfo getMBeanInfo(Object contributionDelegate) {
				return null;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.equinox.jmx.server.Contribution#getName()
			 */
			protected String getName() {
				return contributionDelegate.toString();
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
			public Object getAttribute(String attribute) throws AttributeNotFoundException, MBeanException, ReflectionException {
				return null;
			}

			/* (non-Javadoc)
			 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
			 */
			public AttributeList getAttributes(String[] attributes) {
				return null;
			}

			/* (non-Javadoc)
			 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
			 */
			public void setAttribute(Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException {

			}

			/* (non-Javadoc)
			 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
			 */
			public AttributeList setAttributes(AttributeList attributes) {
				return null;
			}
		} : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#createProvider(java.lang.Object)
	 */
	protected ContributionProvider createProvider(Object obj) {
		return providesType(obj) ? new BundlePackagesProvider((Bundle) obj) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#providesType(java.lang.Object)
	 */
	protected boolean providesType(Object obj) {
		return obj instanceof Bundle;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		Object[] ret = null;
		if (bundle != null) {
			try {
				BundleDescription desc = BundleUtils.getBundleDescription(bundle, Activator.getBundleContext());
				return desc.getImportPackages();
			} catch (Exception e) {
			}
		}
		return ret;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getName()
	 */
	protected String getName() {
		return BundleMessages.packages_name;
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
	public Object getAttribute(String attribute) throws AttributeNotFoundException, MBeanException, ReflectionException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
	 */
	public AttributeList getAttributes(String[] attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException {
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
	 */
	public AttributeList setAttributes(AttributeList attributes) {
		return null;
	}
}
