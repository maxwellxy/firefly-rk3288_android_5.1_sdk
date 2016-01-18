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
package org.eclipse.equinox.jmx.common;

import java.io.IOException;
import java.io.Serializable;
import java.util.*;
import javax.management.*;
import org.eclipse.equinox.jmx.common.util.ByteArrayHolder;

/**
 * The <code>ContributionProxy</code> provides a interface for operating on, and querying
 * the <code>Contribution</code> which it is a proxy for.  
 * 
 * <p>
 * This proxy is required because of its <code>Serializable</code> properties which cannot be expected
 * from concrete <code>Contribution</code> objects.  The object name associated
 * with this proxy is the same as the object name of the <code>Contribution</code>
 * resource registered with the server.
 * </p>
 * 
 * @see org.eclipse.equinox.jmx.server.Contribution
 */
public class ContributionProxy extends NotificationBroadcasterSupport implements ContributionProxyMBean, IContributionStateChangedDispatcher, Serializable {

	private static final long serialVersionUID = 2259045008283911348L;

	// methods of the server contribution resource this proxy invokes
	public static final String OP_REFRESH_PROXY = "createProxy"; //$NON-NLS-1$
	public static final String OP_GET_CHILD_CONTRIBUTIONS = "getChildContributions"; //$NON-NLS-1$
	public static final String OP_GET_CONTRIBUTION_UI_URL = "getContributionUIUrl"; //$NON-NLS-1$

	private String name;
	private Set contributionProperties;
	private ObjectName objectName;
	private MBeanInfo info;
	private Set listeners;
	private ByteArrayHolder imageData;

	public ContributionProxy() {
	}

	public ContributionProxy(String name, Set contributionProperties, ByteArrayHolder imageData, ObjectName objectName, MBeanInfo info) {
		this.name = name;
		this.contributionProperties = contributionProperties;
		this.objectName = objectName;
		this.info = info;
		this.imageData = imageData;
	}

	/* (non-Javadoc)
	 * @see com.jmx.common.contrib.ContributionProxyMBean#getObjectName()
	 */
	public ObjectName getObjectName() {
		return objectName;
	}

	/* (non-Javadoc)
	 * @see com.jmx.common.contrib.ContributionProxyMBean#getName()
	 */
	public String getName() {
		return name;
	}

	/* (non-Javadoc)
	 * @see com.jmx.common.contrib.ContributionProxyMBean#getMBeanInfo()
	 */
	public MBeanInfo getMBeanInfo() {
		return info;
	}

	/* (non-Javadoc)
	 * @see com.jmx.common.contrib.ContributionProxyMBean#getContributionProperties()
	 */
	public Set getContributionProperties() {
		return contributionProperties;
	}

	/**
	 * Get the <code>ByteArrayHolder</code> containing the raw image data, or null if no image.
	 * 
	 * @return The <code>ByteArrayHolder</code> for this contribution or null if no image.
	 */
	public ByteArrayHolder getImageData() {
		return imageData;
	}

	public ContributionProxy[] getChildContributions(MBeanServerConnection server) throws InstanceNotFoundException, NotCompliantMBeanException, MBeanException, ReflectionException, IOException {
		Object obj = server.invoke(getObjectName(), ContributionProxy.OP_GET_CHILD_CONTRIBUTIONS, null, null);
		if (obj instanceof ContributionProxy[]) {
			return (ContributionProxy[]) obj;
		} else if (obj instanceof Object[]) {
			Object[] objs = (Object[]) obj;
			ContributionProxy[] proxies = new ContributionProxy[objs.length];
			for (int i = 0; i < objs.length; i++) {
				if (objs[i] instanceof ContributionProxy) {
					proxies[i] = (ContributionProxy) objs[i];
				}
			}
			return proxies;
		}
		return null;
	}

	public void refresh(MBeanServerConnection server) throws InstanceNotFoundException, MBeanException, ReflectionException, IOException {
		ContributionProxy updatedProxy = (ContributionProxy) server.invoke(getObjectName(), OP_REFRESH_PROXY, null, null);
		mirror(updatedProxy);
		stateChanged();
	}

	/* (non-Javadoc)
	 * @see com.jmx.common.contrib.IContributionStateChangedDispatcher#stateChanged()
	 */
	public void stateChanged() {
		if (listeners == null) {
			return;
		}
		Iterator iter = listeners.iterator();
		while (iter.hasNext()) {
			IContributionStateChangeListener listener = (IContributionStateChangeListener) iter.next();
			listener.stateChanged(this);
		}
	}

	/**
	 * Add a listener which will receive state change events from this <code>ContributionProxy</code>.
	 * 
	 * @param listener The listener to add.
	 */
	public void addStateChangeListener(IContributionStateChangeListener listener) {
		if (listeners == null) {
			listeners = new HashSet(2);
		}
		listeners.add(listener);
	}

	/**
	 * Remove the provided listener from this <code>ContributionProxy</code>s list of registered
	 * listeners.
	 * 
	 * @param listener The listener to add.
	 */
	public void removeStateChangeListener(IContributionStateChangeListener listener) {
		if (listeners != null) {
			listeners.remove(listener);
		}
	}

	/**
	 * Update the <code>ContributionProxy</code>.
	 * 
	 * @param name The updated name.
	 * @param contributionProperties The updated properties.
	 * @param imageData The updated image data.
	 * @param objectName The updated object name.
	 * @param info The updated info.
	 */
	public void update(String name, Set contributionProperties, ByteArrayHolder imageData, ObjectName objectName, MBeanInfo info) {
		this.name = name;
		this.contributionProperties = contributionProperties;
		this.objectName = objectName;
		this.info = info;
		this.imageData = imageData;
	}

	private void mirror(ContributionProxy contribution) {
		this.name = contribution.getName();
		this.contributionProperties = contribution.getContributionProperties();
		this.objectName = contribution.getObjectName();
		this.info = contribution.getMBeanInfo();
	}
}