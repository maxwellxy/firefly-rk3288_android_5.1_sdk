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
package org.eclipse.equinox.jmx.internal.client.ui.contributionsview;

import java.io.IOException;
import java.util.*;
import javax.management.*;
import org.eclipse.equinox.jmx.common.*;
import org.eclipse.equinox.jmx.internal.client.Activator;
import org.eclipse.equinox.jmx.internal.client.MBeanServerProxy;
import org.eclipse.equinox.jmx.internal.client.ui.ClientUI;
import org.eclipse.equinox.jmx.internal.client.ui.mbeaninfoview.MBeanInfoViewPart;
import org.eclipse.jface.viewers.*;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.PlatformUI;

/**
 * @since 1.0
 */
public class ContributionContentProvider implements ITreeContentProvider, NotificationListener {

	private static final Object[] NO_CHILDS = new Object[0];
	private MBeanServerProxy serverProxy;
	private TreeViewer viewer;
	private Hashtable objNameProxies = new Hashtable();

	/**
	 * Allocate and a <code>ContributionContentProvider</code>.
	 */
	public ContributionContentProvider(TreeViewer viewer, MBeanServerProxy serverProxy) {
		super();
		this.viewer = viewer;
		this.serverProxy = serverProxy;
	}

	public void setServerContributionProxy(MBeanServerProxy serverProxy) {
		// receiving updated proxy, clear current objectname table
		removeListeners();
		objNameProxies.clear();
		this.serverProxy = serverProxy;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	public Object[] getChildren(Object parentElement) {
		if (parentElement instanceof ContributionProxy) {
			ContributionProxy parentProxy = (ContributionProxy) parentElement;
			try {
				monitorProxies(new ContributionProxy[] {parentProxy});
				MBeanServerConnection mbServer = serverProxy.getMBeanServerConnection();
				ContributionProxy[] contribs = parentProxy.getChildContributions(mbServer);
				monitorProxies(contribs);
				Object[] ret = null;
				if (contribs != null && contribs.length > 0) {
					ret = new Object[contribs.length];
					System.arraycopy(contribs, 0, ret, 0, ret.length);
					return ret;
				}
			} catch (Exception e) {
				Activator.logError(e);
			}
		} else if (parentElement instanceof ContributionProxy[]) {
			try {
				monitorProxies((ContributionProxy[]) parentElement);
			} catch (Exception e) {
				Activator.logError(e);
			}
			return (ContributionProxy[]) parentElement;
		}
		return NO_CHILDS;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	public Object getParent(Object element) {
		return element;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	public boolean hasChildren(Object element) {
		if (element instanceof ContributionProxyMBean && serverProxy != null) {
			try {
				ContributionProxy[] contribs = ((ContributionProxy) element).getChildContributions(serverProxy.getMBeanServerConnection());
				return (contribs != null && contribs.length > 0);
			} catch (Exception e) {
				Activator.logError(e);
			}
		}
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	public Object[] getElements(Object inputElement) {
		if (inputElement instanceof RootContribution) {
			RootContribution rc = (RootContribution) inputElement;
			try {
				monitorProxies(new ContributionProxy[] {rc});
			} catch (Exception e) {
				Activator.log(e);
			}
			return getChildren(rc.queryRootContributions());
		}
		return getChildren(inputElement);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	public void dispose() {
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
	}

	/* (non-Javadoc)
	 * @see javax.management.NotificationListener#handleNotification(javax.management.Notification, java.lang.Object)
	 */
	public synchronized void handleNotification(Notification notification, Object handback) {
		Object obj = notification.getSource();
		if (!(obj instanceof ObjectName) || serverProxy == null) {
			return;
		}
		final ObjectName objectName = (ObjectName) obj;
		List proxiesRef = (List) objNameProxies.get(objectName);
		if (proxiesRef == null) {
			return;
		}
		final List proxies = new ArrayList(proxiesRef);
		final Iterator iter = proxies.iterator();
		String type = notification.getType();
		if (type.equals(ContributionNotificationEvent.NOTIFICATION_UPDATED) || type.equals(ContributionNotificationEvent.NOTIFICATION_ADDED)) {
			Display.getDefault().asyncExec(new Runnable() {
				public void run() {
					// check if updating the root contribution
					if (proxies.size() == 1 && proxies.get(0) instanceof RootContribution) {
						try {
							// remove the previous root proxy from the list of monitored proxies
							objNameProxies.remove(((RootContribution) viewer.getInput()).getObjectName());
							viewer.setInput(serverProxy.getRootContribution());
						} catch (Exception e) {
							Activator.log(e);
						}
						return;
					}
					// not updating the root, therefore refresh all nodes
					// that are references by the same object name
					while (iter.hasNext()) {
						ContributionProxy proxy = (ContributionProxy) iter.next();
						try {
							proxy.refresh(serverProxy.getMBeanServerConnection());
							viewer.update(proxy, null);
							viewer.refresh(proxy);
						} catch (Exception e) {
							Activator.log(e);
						}
					}
				}
			});
		} else if (type.equals(ContributionNotificationEvent.NOTIFICATION_REMOVED)) {
			Display.getDefault().asyncExec(new Runnable() {
				public void run() {
					objNameProxies.remove(objectName);
					MBeanInfoViewPart vp = getMBeanInfoViewPart();
					while (iter.hasNext()) {
						ContributionProxy proxy = (ContributionProxy) iter.next();
						// why doesnt remove emit a selection notification for the viewers...
						// i am required to explicitly close the relevant views
						viewer.remove(proxy);
						// if viewing object that was removed, clear ui
						if (vp != null) {
							vp.contributionRemoved(proxy);
						}
					}
				}

				private MBeanInfoViewPart getMBeanInfoViewPart() {
					return (MBeanInfoViewPart) PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().findView(ClientUI.VIEWID_MBEANINFO);
				}
			});
		}
	}

	private void monitorProxies(ContributionProxy[] proxies) throws InstanceNotFoundException, IOException {
		for (int i = 0; i < proxies.length; i++) {
			ObjectName name = proxies[i].getObjectName();
			List l = null;
			if ((l = (List) objNameProxies.get(name)) == null) {
				l = new Vector(2);
				serverProxy.getMBeanServerConnection().addNotificationListener(name, this, null, null);
				objNameProxies.put(name, l);
			}
			l.add(proxies[i]);
		}
	}

	private void removeListeners() {
		if (serverProxy == null) {
			return;
		}
		Iterator iter = objNameProxies.keySet().iterator();
		try {
			while (iter.hasNext()) {
				ObjectName name = (ObjectName) iter.next();
				serverProxy.getMBeanServerConnection().removeNotificationListener(name, this);
			}
		} catch (Exception e) {
			Activator.log(e);
		}
	}
}