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
import java.util.*;
import javax.management.*;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.common.ContributionNotificationEvent;
import org.eclipse.equinox.jmx.server.Contribution;
import org.eclipse.equinox.jmx.server.ContributionProvider;
import org.eclipse.osgi.util.NLS;
import org.osgi.framework.*;
import org.osgi.service.packageadmin.PackageAdmin;

/**
 * @since 1.0
 */
public class BundleContributionProvider extends ContributionProvider implements BundleListener {

	private static final long serialVersionUID = -8497760387909218802L;
	private static final String BUNDLE_IMG_PATH = "icons/bundles/bundle.gif"; //$NON-NLS-1$
	private static Set serverPluginDependencies;
	private ServiceReference serviceReference;

	/**
	 * Default constructor.
	 */
	public BundleContributionProvider() {
		this(null);
	}

	public BundleContributionProvider(ServiceReference serviceReference) {
		this.serviceReference = serviceReference;
		if (serverPluginDependencies == null) {
			try {
				serverPluginDependencies = BundleUtils.computeDependencies(Activator.getBundle(), Activator.getBundleContext());
			} catch (Exception e) {
				e.printStackTrace();
				throw new RuntimeException(e.getMessage());
			}
		}
	}

	public static boolean isPluginDependency(Bundle bundle) {
		Assert.isNotNull(serverPluginDependencies);
		Iterator iter = serverPluginDependencies.iterator();
		while (iter.hasNext()) {
			String name = (String) iter.next();
			if (null != bundle.getSymbolicName() && bundle.getSymbolicName().equals(name)) {
				return true;
			}
		}
		return false;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#providesType(java.lang.Object)
	 */
	protected boolean providesType(Object obj) {
		return obj instanceof ServiceReference;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#isProviderForType(java.lang.Object)
	 */
	protected boolean contributesType(Object obj) {
		return obj instanceof Bundle;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createProvider(java.lang.Object)
	 */
	protected ContributionProvider createProvider(Object obj) {
		return providesType(obj) ? new BundleContributionProvider((ServiceReference) obj) : null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.ContributionProvider#createContribution(java.lang.Object)
	 */
	protected Contribution createContribution(Object obj) throws MalformedObjectNameException, NullPointerException {
		// if this service provider is associated with a bundle, we wrap the service contribution in a specialized type to display bundle associated information
		if (contributesType(obj)) {
			if (serviceReference != null) {
				return new BundleContribution((Bundle) obj, serviceReference);
			}
			return new BundleContribution((Bundle) obj);
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	public Object[] getChildren() {
		Set result = null;
		if (serviceReference != null) {
			result = new TreeSet();
			Object[] usingBundles = serviceReference.getUsingBundles();
			if (usingBundles != null) {
				for (int i = 0; i < usingBundles.length; i++) {
					result.add(usingBundles[i]);
				}
			}
			// add service provider bundle
			result.add(serviceReference.getBundle());
		}
		return result == null ? Activator.getBundleContext().getBundles() : result.toArray();
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	public String getName() {
		return serviceReference == null ? BundleContributionMessages.bundle_contribution_name : NLS.bind(BundleContributionMessages.service_bundle_contribution_name, ServiceContribution.getServiceReferenceName(serviceReference));
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getProperties()
	 */
	public Set getProperties() {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return FileLocator.find(Activator.getBundle(), new Path(BUNDLE_IMG_PATH), null);
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
	 * @see com.jmx.server.contrib.ContributionProvider#getMBeanInfo(java.lang.Object)
	 */
	public MBeanInfo getMBeanInfo(Object delegate) {
		return new MBeanInfo(getClass().getName(), null, null, null, new MBeanOperationInfo[] {createInstallOperation()}, null);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	public Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		if (operationName.equals("install") && args.length == 1 && argTypes.length == 1 && args[0] instanceof String) { //$NON-NLS-1$
			String installUrl = (String) args[0];
			try {
				Bundle bundle = Activator.getBundleContext().installBundle(installUrl);
				refreshPackages(new Bundle[] {bundle});
			} catch (BundleException e) {
				return e.getMessage();
			}
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleListener#bundleChanged(org.osgi.framework.BundleEvent)
	 */
	public void bundleChanged(BundleEvent event) {
		switch (event.getType()) {
			case BundleEvent.INSTALLED :
				super.contributionStateChanged(new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_ADDED));
				return;
		}
	}

	public static void refreshPackages(Bundle[] bundles) {
		if (bundles.length == 0) {
			return;
		}
		BundleContext context = Activator.getBundleContext();
		ServiceReference packageAdminRef = context.getServiceReference(PackageAdmin.class.getName());
		PackageAdmin packageAdmin = null;
		if (packageAdminRef != null) {
			packageAdmin = (PackageAdmin) context.getService(packageAdminRef);
			if (packageAdmin == null) {
				return;
			}
		}
		packageAdmin.refreshPackages(bundles);
		context.ungetService(packageAdminRef);
	}

	private static MBeanOperationInfo createInstallOperation() {
		return new MBeanOperationInfo("install", BundleContributionMessages.install_operation_desc, new MBeanParameterInfo[] {new MBeanParameterInfo("bundleURLAsString", String.class.getName(), BundleContributionMessages.bundle_url_desc)}, Void.TYPE.getName(), 0); //$NON-NLS-1$ //$NON-NLS-2$
	}
}
