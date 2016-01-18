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

import java.net.URL;
import java.util.*;
import javax.management.*;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.common.ContributionNotificationEvent;
import org.eclipse.equinox.jmx.common.JMXConstants;
import org.eclipse.equinox.jmx.server.Contribution;
import org.osgi.framework.*;

/**
 * A <code>BundleContribution</code> exposes a <code>Bundle</code> as a <code>Contribution</code>.
 * 
 * @since 1.0
 */
public class BundleContribution extends Contribution implements BundleListener {
	private static final long serialVersionUID = 1180032465513448129L;
	private static final String BUNDLE_IMG_PATH = "icons/bundles/bundle.gif"; //$NON-NLS-1$
	private static final Map stateStrings = new HashMap();
	static {
		stateStrings.put(new Integer(Bundle.ACTIVE), "ACTIVE"); //$NON-NLS-1$
		stateStrings.put(new Integer(Bundle.INSTALLED), "INSTALLED"); //$NON-NLS-1$
		stateStrings.put(new Integer(Bundle.RESOLVED), "RESOLVED"); //$NON-NLS-1$
		stateStrings.put(new Integer(Bundle.STARTING), "STARTING"); //$NON-NLS-1$
		stateStrings.put(new Integer(Bundle.STOPPING), "STOPPING"); //$NON-NLS-1$
		stateStrings.put(new Integer(Bundle.UNINSTALLED), "UNINSTALLED"); //$NON-NLS-1$
	}
	private ServiceReference service;

	/**
	 * Allocate a <code>BundleContribution</code> from the <code>bundle</code> provided.
	 * 
	 * @param bundle The bundle to expose as a contribution.
	 */
	public BundleContribution(Bundle bundle) {
		this(bundle, null);
	}

	/**
	 * Allocate a <code>BundleContribution</code> from the <code>bundle</code> that
	 * is associated with the <code>service</code> provided.
	 * 
	 * @param bundle The bundle to expose as a contribution.
	 * @param service The service associated with this contribution.
	 */
	public BundleContribution(Bundle bundle, ServiceReference service) {
		super(bundle);
		this.bundle = bundle;
		this.service = service;
		setProperties(bundle);
		Activator.getBundleContext().addBundleListener(this);
	}

    /* (non-Javadoc)
     * @see org.eclipse.equinox.jmx.server.Contribution#getObjectName()
     */
    protected ObjectName getObjectName() {
        try {
            return new ObjectName(JMXConstants.DEFAULT_DOMAIN + ":type=Bundle,name=" + getName()); //$NON-NLS-1$
        } catch (Exception e) {
            return super.getObjectName();
        }
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
		Set ret = new TreeSet();
		ret.add("Symbolic-name: " + symbolicName); //$NON-NLS-1$
		ret.add("Bundle ID: " + Long.toString(id)); //$NON-NLS-1$
		ret.add("Description: " + description); //$NON-NLS-1$
		ret.add("State: " + (String) stateStrings.get(new Integer(getState()))); //$NON-NLS-1$
		ret.add("Vendor: " + vendor); //$NON-NLS-1$
		ret.add("Contact Address: " + contactAddress); //$NON-NLS-1$
		ret.add("Location: " + location); //$NON-NLS-1$
		ret.add("DocURL: " + docURL); //$NON-NLS-1$
		return ret;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		String imgPath = BUNDLE_IMG_PATH;
		if (service != null) {
			// check if we are a providing this bundle
			if (service.getBundle() == bundle) {
				imgPath = ServiceContribution.IMG_REG_SERVICE_PATH;
				// check if we are also using this bundle
				Bundle[] usingBundles = service.getUsingBundles();
				if (usingBundles != null) {
					for (int i = 0; i < usingBundles.length; i++) {
						if (usingBundles[i] == bundle) {
							imgPath = ServiceContribution.IMG_USE_REG_SERVICE_PATH;
							break;
						}
					}
				}
			} else {
				// bundle is merely a consumer of this service
				imgPath = ServiceContribution.IMG_USE_SERVICE_PATH;
			}
		}
		return FileLocator.find(Activator.getBundle(), new Path(imgPath), null);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		if (args.length == 0 && argTypes.length == 0) {
			try {
				if (operationName.equals("start")) { //$NON-NLS-1$
					bundle.start();
				} else if (operationName.equals("stop")) { //$NON-NLS-1$
					bundle.stop();
				} else if (operationName.equals("uninstall")) { //$NON-NLS-1$
					bundle.uninstall();
					//BundleContributionProvider.refreshPackages(new Bundle[] {bundle});
				}
			} catch (Exception e) {
				return e.getMessage();
			}
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleListener#bundleChanged(org.osgi.framework.BundleEvent)
	 */
	public void bundleChanged(BundleEvent event) {
		Bundle b = (Bundle) event.getSource();
		if (b != bundle && event.getType() != getState()) {
			return;
		}
		setProperties(bundle);
		ContributionNotificationEvent cEvent = null;
		switch (event.getType()) {
			case BundleEvent.UNINSTALLED :
				cEvent = new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_REMOVED);
				break;
			case BundleEvent.STARTED :
			case BundleEvent.STOPPED :
				cEvent = new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_UPDATED);
				break;
			default :
				return;
		}
		super.contributionStateChanged(cEvent);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	public String getName() {
		return symbolicName + "-" + version; //$NON-NLS-1$
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
		MBeanInfo info = new MBeanInfo(getClass().getName(), BundleContributionMessages.bundle_description, getAttributes(), getConstructorInfo(), getOperationInfo(), getNotificationInfo());
		return info;
	}

	/**
	 * Returns the <code>contactAddress</code> for this bundle, or <code>null</code> if one doesn't exist.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @return The <code>contactAddress</code> of a specific bundle
	 */
	public String getContactAddress() {
		return contactAddress;
	}

	/**
	 * Sets the <code>contactAddress</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders() 
	 * @param contactAddress is a <code>String<code> which specifying the <code>contactAddress</code> of a specific bundle
	 */
	public void setContactAddress(String contactAddress) {
		this.contactAddress = contactAddress;
	}

	/**
	 * Returns the <code>description</code> for this bundle, or null if one doesn't exist.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @return The <code>description</code> of a specific bundle
	 */
	public String getDescription() {
		return description;
	}

	/**
	 * Sets the <code>description</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @param description is a <code>String</code> which specifying the <code>description</code> of a specific bundle
	 */
	public void setDescription(String description) {
		this.description = description;
	}

	/**
	 * Returns the <code>dorURL</code> for this bundles, or <code>null</code> if one doesn't exist.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @return The <code>dorURL</code> of a specific bundle
	 */
	public String getDocURL() {
		return docURL;
	}

	/**
	 * Sets the <code>docURL</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders() 
	 * @param docURL is a <code>String</code> which specifying the <code>DocURL</code> of a specific bundle
	 */
	public void setDocURL(String docURL) {
		this.docURL = docURL;
	}

	/**
	 * Returns the identifier for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getBundleId()
	 * @return The <code>id</code> is a unique identifier of a specific bundle
	 */
	public long getId() {
		return id;
	}

	/**
	 * Sets the bundle's identifier for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getBundleId()
	 * @param id is a <code>long<code> that specifying the identifier of a specific bundle
	 * @exception <code>IllegalArgumentException</code> if input id smaller than 0
	 */
	public void setId(long id) throws IllegalArgumentException {
		Assert.isLegal(id >= 0);
		this.id = id;
	}

	/**
	 * Returns the date when this bundle was last modified.
	 * 
	 * @see org.osgi.framework.Bundle#getLastModified()
	 * @return The <code>lastModified</code> which indicates when this bundle was last modified
	 */
	public long getLastModified() {
		return lastModified;
	}

	/**
	 * Sets the time when this bundle was last modified.
	 * 
	 * @see org.osgi.framework.Bundle#getLastModified()
	 * @param lastModified the bundle's last modified time
	 */
	public void setLastModified(long lastModified) {
		this.lastModified = lastModified;
	}

	/**
	 * Returns the <code>location</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getLocation()
	 * @return The <code>location</code> of a specific bundle
	 */
	public String getLocation() {
		return location;
	}

	/**
	 * Sets the <code>location</code> of a specific bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getLocation()
	 * @param location is a <code>String<code> which specifying the <code>location</code> 
	 *        of a specific bundle
	 * @exception IllegalArgumentException if <code>location<code> is null or empty string
	 */
	public void setLocation(String location) throws IllegalArgumentException {
		Assert.isLegal(location != null && !location.equals("")); //$NON-NLS-1$
		this.location = location;
	}

	/**
	 * Return the <code>state</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getState()
	 * @return The current <code>state</code> of a specific bundle
	 */
	public int getState() {
		return state;
	}

	/**
	 * Sets the <code>state</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getState()
	 * @param state is a <code>int</code> which specifying the <code>state</code> of a specific bundle
	 */
	public void setState(int state) {
		this.state = state;
	}

	/**
	 * Returns the </code>symbolicName</code> for this bundle, or <code>null</code> if one doesn't exist.
	 * 
	 * @see org.osgi.framework.Bundle#getSymbolicName()
	 * @return The symbolic name of a specific bundle
	 */
	public String getSymbolicName() {
		return symbolicName;
	}

	/**
	 * Sets the </code>symbolicName</code> of for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getSymbolicName()
	 * @param symbolicName is a <code>String</code> which specifying the <code>symbolicName<code> of a specific bundle
	 */
	public void setSymbolicName(String symbolicName) {
		this.symbolicName = symbolicName;
	}

	/**
	 * Returns the <code>vendor</code> for this bundle, or <code>null</code> if one doesn't exist.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @return The <code>vendor</code> of a specific bundle
	 */
	public String getVendor() {
		return vendor;
	}

	/**
	 * Sets the <code>vendor</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @param vendor specifying the vendor for the bundle.
	 */
	public void setVendor(String vendor) {
		this.vendor = vendor;
	}

	/**
	 * Returns the <code>version</code> for this bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @return version of a specific bundle
	 */
	public Version getVersion() {
		return version;
	}

	/**
	 * Sets the <code>version</code> of a specific bundle.
	 * 
	 * @see org.osgi.framework.Bundle#getHeaders()
	 * @param version is a <code>Version</code> that specifying the <code>version</code> for a specific bundle
	 * @exception <code>IllegalArgumentException</code> if given version is <code>null</code> 
	 */
	public void setVersion(Version version) throws IllegalArgumentException {
		Assert.isLegal(version != null);
		this.version = version;
	}

	private MBeanAttributeInfo[] getAttributes() {
		MBeanAttributeInfo[] ret = new MBeanAttributeInfo[0];
		return ret;
	}

	private MBeanConstructorInfo[] getConstructorInfo() {
		MBeanConstructorInfo[] ret = new MBeanConstructorInfo[0];
		return ret;
	}

	private MBeanOperationInfo[] getOperationInfo() {
		if (BundleContributionProvider.isPluginDependency(bundle)) {
			// expose no operations
			return new MBeanOperationInfo[0];
		}
		return new MBeanOperationInfo[] {createStartOperation(), createStopOperation(), createUninstallOperation()};
	}

	private void setProperties(Bundle bundle) {
		setContactAddress((String) bundle.getHeaders().get("Bundle-ContactAddress")); //$NON-NLS-1$
		setDescription((String) bundle.getHeaders().get("Bundle-Description")); //$NON-NLS-1$
		setDocURL((String) bundle.getHeaders().get("Bundle-DocURL")); //$NON-NLS-1$
		setId(bundle.getBundleId());
		setLastModified(bundle.getLastModified());
		setLocation(bundle.getLocation());
		setState(bundle.getState());
		setSymbolicName(bundle.getSymbolicName());
		setVendor((String) bundle.getHeaders().get("Bundle-Vendor")); //$NON-NLS-1$
		setVersion(new Version((String) bundle.getHeaders().get("Bundle-Version"))); //$NON-NLS-1$
	}

	private static MBeanOperationInfo createStartOperation() {
		return new MBeanOperationInfo("start", BundleContributionMessages.start_operation_desc, new MBeanParameterInfo[0], Void.TYPE.getName(), 0);//$NON-NLS-1$
	}

	private static MBeanOperationInfo createStopOperation() {
		return new MBeanOperationInfo("stop", BundleContributionMessages.stop_operation_desc, new MBeanParameterInfo[0], Void.TYPE.getName(), 0); //$NON-NLS-1$
	}

	private static MBeanOperationInfo createUninstallOperation() {
		return new MBeanOperationInfo("uninstall", BundleContributionMessages.stop_operation_desc, new MBeanParameterInfo[0], Void.TYPE.getName(), 0); //$NON-NLS-1$
	}

	private long id;
	private long lastModified;
	private int state;
	private Version version;
	private String symbolicName;
	private String vendor;
	private String description;
	private String docURL;
	private String contactAddress;
	private String location;
	private Bundle bundle;
}
