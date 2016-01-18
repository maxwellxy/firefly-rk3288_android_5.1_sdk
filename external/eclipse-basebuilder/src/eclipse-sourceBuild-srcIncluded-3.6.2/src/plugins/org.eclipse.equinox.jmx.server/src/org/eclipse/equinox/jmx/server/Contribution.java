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
package org.eclipse.equinox.jmx.server;

import java.io.*;
import java.net.URL;
import java.util.*;
import javax.management.*;
import org.eclipse.equinox.jmx.common.*;
import org.eclipse.equinox.jmx.common.util.ByteArrayHolder;
import org.eclipse.equinox.jmx.internal.server.Activator;
import org.eclipse.equinox.jmx.internal.server.ContributionMessages;

/**
 * A <code>Contribution</code> acts as a mediator between the objects which users would like to
 * provide, and the server which exposes those objects.  To expose the contribution to clients,
 * a <code>ContributionProxy</code> is created which encapsulates all the required information
 * in order for this contribution to be used remotely.  This proxy, which is implemented
 * as a standard mbean exposes an operation to retrieve the <code>ObjectName</code> of this
 * contribution.  Succinctly, the <code>ContributionProxy</code> delegates all operations
 * to the <code>Contribution</code> associated with it's <code>ObjectName</code> on
 * the server.
 * 
 * @see com.jmx.common.ContributionProvider
 * @since 1.0
 */

public abstract class Contribution extends NotificationBroadcasterSupport implements DynamicMBean {

	private static long sequenceNumber;

	// container for all contributions' delegate objects, weak insurance for contributions
	// attempting to utilize the same delegate
	private static Map contributionDelegates = new Hashtable();

	// contribution image cache
	private static Map imageCache = new Hashtable();

	// current proxy for this contribution which is exposed to clients
	private ContributionProxy proxy;

	// set true when the implementing contribution fires a state changed event, calling create proxy with this set to true would result in a new proxy being allocated
	private boolean stateChanged;

	// the unique object name of this contribution that is registered with the server
	private ObjectName objectName;

	/**
	 * The <code>Object</code> delegate that is associated with this <code>Contribution</code>.
	 * <p>
	 * The delegate is provided in the constructor by implementing classes, and is accessible by
	 * those classes for use.  The delegate is declared as final to enforce and make 
	 * explicit its consistency.
	 */
	protected final Object contributionDelegate;

	/**
	 * Default constructor.
	 * 
	 * @see Contribution#Contribution(Object) where <code>Object</code> is null.
	 */
	public Contribution() {
		this(null);
	}

	/**
	 * Allocate a <code>Contribution</code> which provides an interface to the
	 * operations provided by the <code>contributionDelegate</code>.  The 
	 * <code>contributionDelegate</code> may be null, this is typical of 
	 * <code>ContributionProvider</code>s who usually don't manipulate
	 * any underlying resource.
	 * 
	 * @param contributionDelegate The object which the contribution delegates its operations, null if not applicable.
	 */
	public Contribution(Object contributionDelegate) {
		this.contributionDelegate = contributionDelegate;
		Contribution priorContrib = null;
		if (contributionDelegate != null) {
			priorContrib = (Contribution) contributionDelegates.get(contributionDelegate);
		}
		if (priorContrib != null) {
			objectName = priorContrib.getObjectName();
		} else {
			try {
				String objectNameStr = JMXConstants.DEFAULT_DOMAIN + ":type=" + getClass().getName() + hashCode(); //$NON-NLS-1$
				objectName = ObjectName.getInstance(objectNameStr);
			} catch (MalformedObjectNameException e) {
				Activator.logError(e);
			}
			if (contributionDelegate != null) {
				contributionDelegates.put(this.contributionDelegate, this);
			}
		}
	}

	/**
	 * Get the name of this contribution.
	 * 
	 * @return The name of this contribution.
	 */
	protected abstract String getName();

	/**
	 * Get the list of objects which this <code>Contribution</code>
	 * wishes to contribute as sub-contributions.  The objects 
	 * returned may or may not be available to clients; their availability depends
	 * on a supporting <code>ContributionProvider</code> existing.
	 * 
	 * @return The objects this <code>Contribution</code> wishes to contribute.
	 */
	protected abstract Object[] getChildren();

	/**
	 * Derived classes have the option to provide list of properties
	 * associated with this contribution to be displayed in the UI.
	 * 
	 * @return The properties of the contribution to be displayed in the UI, or <code>null</code> if none.
	 */
	protected abstract Set getProperties();

	/**
	 * Get the location for the image to be associated with this contribution.
	 * 
	 * @return The location of the image.
	 */
	protected abstract URL getImageLocation();

	/**
	 * Derived classes must return an <code>MBeanInfo</code> object 
	 * which contains the operations intended to be exposed.  The object
	 * returned is manipulated to include <code>Contribution</code> specific
	 * operations to support traversal.
	 * 
	 * @param delegate The delegate object associated with this contribution.
	 * @return The MBeanInfo object which encapsulates the functionality of the derived contribution.
	 */
	protected abstract MBeanInfo getMBeanInfo(Object delegate);

	/**
	 * This method is forwarded by the contribution to the derived <code>Contribution</code> when a operation
	 * is to be invoked.
	 * 
	 * @param operationName The name of the operation to invoke.
	 * @param args List of arguments of the operation to invoke.
	 * @param argTypes List of argument types of the operation to invoke.
	 * @return The Object result of the operation, null if not relevant.
	 */
	protected abstract Object invokeOperation(String operationName, Object[] args, String[] argTypes);

	/**
	 * Gets the <code>Contribution</code> instance that was constructed with the provided
	 * <code>contributionDelegate</code> or null if no such <code>Contribution</code> was created.
	 * @param contributionDelegate The object used during construction of the <code>Contribution</code>.
	 * @return The <code>Contribution</code>, or null if not found.
	 */
	public static final Contribution getContribution(final Object contributionDelegate) {
		return (Contribution)contributionDelegates.get(contributionDelegate);
	}
	
	/**
	 * Get the object delegate that is associated with this contribution.  The
	 * delegate may be null; this is typical for provider contributions..
	 * 
	 * @return The object delegate for this contribution, may be null.
	 */
	public Object getContributionDelegate() {
		return contributionDelegate;
	}

	/**
	 * Allocate a <code>ContributionProxy</code> from this <code>Contribution</code>.
	 * 
	 * @see org.eclipse.equinox.jmx.common.ContributionProxy
	 * @return A newly allocated <code>ContributionProxy</code>.
	 */
	public final ContributionProxy createProxy() {
		if (proxy == null || stateChanged) {
			ByteArrayHolder holder = null;
			URL imageUrl = getImageLocation();
			// image URL may be null depending on contribution state
			if (imageUrl != null) {
				if ((holder = (ByteArrayHolder) imageCache.get(imageUrl)) == null) {
					// create byte array holder from image data and add to cache
					InputStream in = null;
					try {
						in = imageUrl.openStream();
						ByteArrayOutputStream bout = new ByteArrayOutputStream();
						byte[] buf = new byte[512];
						int nread;
						while ((nread = in.read(buf)) != -1) {
							bout.write(buf, 0, nread);
						}
						holder = new ByteArrayHolder(bout.toByteArray());
						imageCache.put(imageUrl, holder);
					} catch (IOException e) {
						Activator.logError(e);
					} finally {
						if (in != null) {
							try {
								in.close();
							} catch (IOException e) {
								Activator.logError(e);
							}
						}
					}
				}
			}
			proxy = new ContributionProxy(getName(), getProperties(), holder, getObjectName(), getMBeanInfo());
			stateChanged = false;
		}
		return proxy;
	}

	/**
	 * Get this contribution's list of child contributions as type <code>ContributionProxy</code>.
	 * A <code>Contribution</code>s children are only registered with the server if
	 * <code>ContributionProvider</code> exists which can wrap each child returned by
	 * the <code>Contribution</code>.
	 * 
	 * @return The list of child contributions, null if no children.
	 */
	public final ContributionProxy[] getChildContributions() {
		ContributionProxy[] result = getChildContributionProxies();
		if (!(this instanceof ContributionProvider)) {
			// attempt to locate any providers which may also contribute to
			// this contributions's delegate.
			if (contributionDelegate != null) {
				ContributionProxy[] delegateProxies = null;
				ContributionProvider[] delegateProviders = ContributionProvider.getExtendingProviders(contributionDelegate);
				if (delegateProviders != null) {
					delegateProxies = getChildContributions(delegateProviders);
				}
				if (delegateProxies != null) {
					ContributionProxy[] oldResult = (result == null ? (result = new ContributionProxy[0]) : result);
					result = new ContributionProxy[result.length + delegateProxies.length];
					System.arraycopy(oldResult, 0, result, 0, oldResult.length);
					System.arraycopy(delegateProxies, 0, result, oldResult.length, delegateProxies.length);
				}
			}
		}
		return result;
	}

	private ContributionProxy[] getChildContributionProxies() {
		List proxies = null;
		Object[] childs = getChildren();
		if (childs != null) {
			for (int i = 0; i < childs.length; i++) {
				ContributionProvider provider = null;
				if (this instanceof ContributionProvider && ((ContributionProvider) this).contributesType(childs[i])) {
					provider = (ContributionProvider) this;
				} else {
					provider = ContributionProvider.getProvider(childs[i]);
				}
				if (provider == null) {
					continue;
				}
				try {
					Contribution contrib = provider.createContribution(childs[i]);
					contrib.registerContribution(Activator.getDefault().getServer());
					if (proxies == null) {
						proxies = new ArrayList();
					}
					proxies.add(contrib.createProxy());
				} catch (Exception e) {
					Activator.log(e);
				}
			}
		}
		return proxies == null ? null : (ContributionProxy[]) proxies.toArray(new ContributionProxy[proxies.size()]);
	}

	/**
	 * Register this contribution with the <code>server</code> provided.
	 * 
	 * @param server The server to register this contribution with.
	 * @throws InstanceAlreadyExistsException If the contribution's object name has already been registered with the server.
	 * @throws MBeanRegistrationException
	 * @throws NotCompliantMBeanException
	 */
	public final void registerContribution(MBeanServer mbeanServer) throws MBeanRegistrationException, NotCompliantMBeanException {
		if (!mbeanServer.isRegistered(getObjectName())) {
			try {
				mbeanServer.registerMBean(this, getObjectName());
			} catch (InstanceAlreadyExistsException e) {
				// previously checked for registration, this should not occur
				Activator.logError(e);
			}
		}
	}

	/**
	 * Invoked by the underlying <code>Contribution</code> when its internal state
	 * has changed and a new <code>ContributionProxy</code> should be created
	 * to reflect this change.
	 * 
	 * @param event The event describing the state transition.
	 */
	protected void contributionStateChanged(ContributionNotificationEvent event) {
		if (event == null) {
			return;
		}
		if (event.getType().equals(ContributionNotificationEvent.NOTIFICATION_REMOVED)) {
			// self from currently contributed objects and release hold on delegate
			contributionDelegates.remove(contributionDelegate);
		} else if (event.getType().equals(ContributionNotificationEvent.NOTIFICATION_ADDED)) {
			// not required to add contribution and delegate to map, this is handled in getChildren
		} else if (event.getType().equals(ContributionNotificationEvent.NOTIFICATION_UPDATED)) {
			// currently not required to perform any operations
		}
		stateChanged = true;
		sendNotification(new Notification(event.getType(), this, sequenceNumber++, new Date().getTime()));
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getMBeanInfo()
	 */
	public MBeanInfo getMBeanInfo() {
		// retrieve implementors exposed operations
		MBeanInfo info = getMBeanInfo(contributionDelegate);
		if (info != null) {
			// insert our required getContributions() operation
			MBeanOperationInfo[] ops = info.getOperations();
			MBeanOperationInfo[] opsNew = new MBeanOperationInfo[ops.length + 2];
			System.arraycopy(ops, 0, opsNew, 0, ops.length);
			try {
				opsNew[opsNew.length - 2] = new MBeanOperationInfo(ContributionMessages.desc_getcontribs, Contribution.class.getMethod("getChildContributions", new Class[0])); //$NON-NLS-1$
				opsNew[opsNew.length - 1] = new MBeanOperationInfo("", Contribution.class.getMethod("createProxy", new Class[0])); //$NON-NLS-1$ //$NON-NLS-2$
				return new MBeanInfo(info.getClassName(), info.getDescription(), info.getAttributes(), info.getConstructors(), opsNew /* our customized operations */, info.getNotifications());
			} catch (Exception e) {
				Activator.logError(e);
			}
		} else {
			info = new MBeanInfo(getClass().getName(), getName(), new MBeanAttributeInfo[0], new MBeanConstructorInfo[0], new MBeanOperationInfo[0], new MBeanNotificationInfo[0]);
		}
		return info;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#invoke(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	public Object invoke(String arg0, Object[] arg1, String[] arg2) throws MBeanException, ReflectionException {
		if (arg0.equals(ContributionProxy.OP_GET_CHILD_CONTRIBUTIONS)) {
			return getChildContributions();
		} else if (arg0.equals(ContributionProxy.OP_REFRESH_PROXY)) {
			return createProxy();
		} else if (arg0.equals(ContributionProxy.OP_GET_CONTRIBUTION_UI_URL)) {
			//return UIContributionRegistry.getInstance().getContributionUIUrl(contributionDelegate.getClass());
		}
		// delegate to implementing class
		return invokeOperation(arg0, arg1, arg2);
	}
	
	protected ObjectName getObjectName() {
		return objectName;
	}
	
	private final ContributionProxy[] getChildContributions(ContributionProvider[] delegateProviders) {
		try {
			List delegateProviderList = new ArrayList();
			for (int i = 0; i < delegateProviders.length; i++) {
				ContributionProvider delegateProvider = delegateProviders[i];
				ContributionProxy[] proxies = delegateProvider.getChildContributions();
				if (proxies != null && proxies.length > 0) {
					delegateProvider.registerContribution(Activator.getDefault().getServer());
					delegateProviderList.add(delegateProvider.createProxy());
				}
			}
			if (delegateProviderList.size() == 0) {
				return null;
			}
			return (ContributionProxy[]) delegateProviderList.toArray(new ContributionProxy[delegateProviderList.size()]);
		} catch (Exception e) {
			Activator.logError(e);
		}
		return null;
	}
}
