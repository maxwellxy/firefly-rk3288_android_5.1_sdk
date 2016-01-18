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
package org.eclipse.osgi.jmx.internal;

import java.lang.reflect.Method;
import java.net.URL;
import java.util.HashSet;
import java.util.Set;
import javax.management.*;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.equinox.jmx.common.ContributionNotificationEvent;
import org.eclipse.equinox.jmx.common.util.MBeanInfoWrapper;
import org.eclipse.equinox.jmx.server.Contribution;
import org.osgi.framework.*;
import org.osgi.util.tracker.ServiceTracker;

/**
 * @since 1.0
 */
public class ServiceContribution extends Contribution implements ServiceListener {

	static final String IMG_REG_SERVICE_PATH = "icons/services/bundle_reg_service.png"; //$NON-NLS-1$
	static final String IMG_USE_REG_SERVICE_PATH = "icons/services/bundle_use_reg_service.png"; //$NON-NLS-1$
	static final String IMG_USE_SERVICE_PATH = "icons/services/bundle_use_service.png"; //$NON-NLS-1$

	private static final String IMG_SERVICE_PATH = "icons/services/service.gif"; //$NON-NLS-1$

	private final MBeanInfo mbeanInfo;
	private final ServiceReference serviceDelegate;
	private final Bundle bundle;
	private Class serviceDelegateImplClass;

	public ServiceContribution(ServiceReference serviceDelegate) {
		this(serviceDelegate, null);
	}

	public ServiceContribution(ServiceReference serviceDelegate, Bundle bundle) {
		super(serviceDelegate);
		if (serviceDelegate == null) {
			throw new IllegalArgumentException();
		}
		this.serviceDelegate = serviceDelegate;
		this.bundle = bundle;
		try {
			serviceDelegateImplClass = getClass().getClassLoader().loadClass(getName());
		} catch (ClassNotFoundException e) {
			serviceDelegateImplClass = serviceDelegate.getClass();
		}
		mbeanInfo = MBeanInfoWrapper.createMBeanInfo(serviceDelegateImplClass, ServiceContributionMessages.service_contribution_name, new MBeanAttributeInfo[0], new MBeanNotificationInfo[0]);
		Activator.getBundleContext().addServiceListener(this);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getObjectName()
	 */
	protected ObjectName getObjectName() {
		try {
			return new ObjectName("jmxserver:type=Service,name=" + getName()); //$NON-NLS-1$
		} catch (Exception e) {
			return super.getObjectName();
		}
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return ServiceContribution.getServiceReferenceName(serviceDelegate);

	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getProperties()
	 */
	protected Set getProperties() {
		Set ret = new HashSet();
		String[] propKeys = serviceDelegate.getPropertyKeys();
		for (int i = 0; i < propKeys.length; i++) {
			ret.add(propKeys[i] + ":" + serviceDelegate.getProperty(propKeys[i]).toString()); //$NON-NLS-1$
		}
		return ret;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		String imgPath = IMG_SERVICE_PATH;
		if (bundle != null) {
			boolean isUsingService = false, hasRegisteredService = false;
			ServiceReference[] servicesInUse = bundle.getServicesInUse();
			for (int i = 0; i < servicesInUse.length; i++) {
				if (serviceDelegate == servicesInUse[i]) {
					isUsingService = true;
				}
			}
			ServiceReference[] registeredServices = bundle.getRegisteredServices();
			for (int i = 0; i < registeredServices.length; i++) {
				if (serviceDelegate == registeredServices[i]) {
					hasRegisteredService = true;
				}
			}
			if (isUsingService || hasRegisteredService) {
				if (isUsingService && hasRegisteredService) {
					imgPath = IMG_USE_REG_SERVICE_PATH;
				} else if (isUsingService) {
					imgPath = IMG_USE_SERVICE_PATH;
				} else {
					imgPath = IMG_REG_SERVICE_PATH;
				}
			}
		}
		return FileLocator.find(Activator.getBundle(), new Path(imgPath), null);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		Object[] ret = null;
		if (serviceDelegate.getUsingBundles() != null) {
			ret = new Object[serviceDelegate.getUsingBundles().length + 1];
			System.arraycopy(serviceDelegate.getUsingBundles(), 0, ret, 0, serviceDelegate.getUsingBundles().length);
		} else {
			ret = new Object[1];
		}
		System.arraycopy(new Object[] {serviceDelegate.getBundle()}, 0, ret, (serviceDelegate.getUsingBundles() == null ? 0 : serviceDelegate.getUsingBundles().length), 1);
		return ret;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		Object ret = null;
		// determine if bundle has registered the exposed service
		BundleContext context = Activator.getBundleContext();
		if (context != null) {
			// attempt to locate service
			ServiceTracker st = new ServiceTracker(context, serviceDelegate, null);
			st.open();
			Object serviceImpl = st.getService(serviceDelegate);
			if (serviceImpl != null) {
				try {
					Method[] methods = serviceImpl.getClass().getMethods();
					for (int i = 0; i < methods.length; i++) {
						if (methods[i].getName().equals(operationName)) {
							ret = methods[i].invoke(serviceImpl, args);
							break;
						}
					}
				} catch (Exception e) {
					ret = e.getMessage();
				}
				st.close();
				st = null;
			}
		} else {
			// bundle which contains service has not been started
			ret = new String(ServiceContributionMessages.controlling_bundle_stopped);
		}
		return ret;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttribute(java.lang.String)
	 */
	public Object getAttribute(String attribute) throws AttributeNotFoundException, MBeanException, ReflectionException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException {
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
	 * @see com.jmx.server.contrib.Contribution#getMBeanInfo(java.lang.Object)
	 */
	public MBeanInfo getMBeanInfo(Object delegate) {
		return mbeanInfo;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.ServiceListener#serviceChanged(org.osgi.framework.ServiceEvent)
	 */
	public void serviceChanged(ServiceEvent event) {
		if (serviceDelegate != event.getServiceReference()) {
			return;
		}
		ContributionNotificationEvent cEvent = null;
		switch (event.getType()) {
			case ServiceEvent.MODIFIED :
				cEvent = new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_UPDATED);
				break;
			case ServiceEvent.UNREGISTERING :
				cEvent = new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_REMOVED);
				break;
			default :
				return;
		}
		super.contributionStateChanged(cEvent);
	}

	public static String getServiceReferenceName(ServiceReference serviceReference) {
		Object prop = serviceReference.getProperty("objectClass"); //$NON-NLS-1$
		if (prop instanceof String[]) {
			String[] props = (String[]) prop;
			if (props.length == 1) {
				return props[0];
			}
		}
		return serviceReference.toString();
	}

}
