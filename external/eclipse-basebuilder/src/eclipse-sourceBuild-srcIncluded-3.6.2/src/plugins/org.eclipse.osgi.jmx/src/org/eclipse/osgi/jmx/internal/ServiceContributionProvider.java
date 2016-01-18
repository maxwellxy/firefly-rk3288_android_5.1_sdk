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
import java.util.HashSet;
import java.util.Set;
import javax.management.*;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.equinox.jmx.server.Contribution;
import org.eclipse.equinox.jmx.server.ContributionProvider;
import org.eclipse.osgi.util.NLS;
import org.osgi.framework.*;

/**
 * Eclipse service contribution provider.  Determines the available services
 * and exports supported services for remote inspection.  
 *  
 * @since 1.0
 */
public class ServiceContributionProvider extends ContributionProvider {

	private static final String SERVICE_IMG_PATH = "icons/services/service.gif"; //$NON-NLS-1$

	private Bundle bundle;

	public ServiceContributionProvider() {
		this(null);
	}

	public ServiceContributionProvider(Bundle bundle) {
		this.bundle = bundle;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return bundle == null ? ServiceContributionMessages.service_contribution_name : NLS.bind(ServiceContributionMessages.bundle_service_contribution_name, bundle.getSymbolicName());
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
		return FileLocator.find(Activator.getBundle(), new Path(SERVICE_IMG_PATH), null);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#contributesType(java.lang.Object)
	 */
	protected boolean contributesType(Object obj) {
		return obj instanceof ServiceReference;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#providesType(java.lang.Object)
	 */
	protected boolean providesType(Object obj) {
		return obj instanceof Bundle;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createProvider(java.lang.Object)
	 */
	protected ContributionProvider createProvider(Object obj) {
		return providesType(obj) ? new ServiceContributionProvider((Bundle) obj) : null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createContribution(java.lang.Object)
	 */
	protected Contribution createContribution(Object obj) throws MalformedObjectNameException, NullPointerException {
		// if this service provider is associated with a bundle, we wrap the service contribution in a specialized type to display bundle associated information
		if (contributesType(obj)) {
			if (bundle != null) {
				return new ServiceContribution((ServiceReference) obj, bundle);
			}
			return new ServiceContribution((ServiceReference) obj);
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		if (bundle != null) {
			Set services = null;
			if (bundle.getRegisteredServices() != null && bundle.getServicesInUse() != null) {
				services = new HashSet();
				if (bundle.getRegisteredServices() != null) {
					for (int i = 0; i < bundle.getRegisteredServices().length; i++) {
						services.add(bundle.getRegisteredServices()[i]);
					}
				}
				if (bundle.getServicesInUse() != null) {
					for (int i = 0; i < bundle.getServicesInUse().length; i++) {
						services.add(bundle.getServicesInUse()[i]);
					}
				}
			}
			return services == null ? null : services.toArray();
		}
		BundleContext bc = Activator.getBundleContext();
		try {
			return bc.getServiceReferences(null, null);
		} catch (InvalidSyntaxException e) {
			// invalid syntax on null, null...
		}
		return null;
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
	public Object getAttribute(String arg0) throws AttributeNotFoundException, MBeanException, ReflectionException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute arg0) throws AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException {
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
	 */
	public AttributeList getAttributes(String[] arg0) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
	 */
	public AttributeList setAttributes(AttributeList arg0) {
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getMBeanInfo(java.lang.Object)
	 */
	public MBeanInfo getMBeanInfo(Object obj) {
		return new MBeanInfo(getClass().getName(), null, null, null, null, null);
	}
}
