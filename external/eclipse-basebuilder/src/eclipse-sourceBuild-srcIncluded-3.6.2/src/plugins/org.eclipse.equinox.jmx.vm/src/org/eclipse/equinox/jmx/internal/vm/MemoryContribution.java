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
package org.eclipse.equinox.jmx.internal.vm;

import java.lang.management.MemoryMXBean;
import java.lang.management.MemoryUsage;
import java.net.URL;
import java.util.Set;
import java.util.TreeSet;
import javax.management.*;
import org.eclipse.equinox.jmx.common.ContributionNotificationEvent;
import org.eclipse.equinox.jmx.common.util.MBeanInfoWrapper;
import org.eclipse.equinox.jmx.server.Contribution;

public class MemoryContribution extends Contribution implements NotificationListener {

	private static final String ICON_PATH = "icons/memory.gif"; //$NON-NLS-1$
	private long heapCommited, heapInit, heapMax, heapUsed;
	private long nonHeapCommited, nonHeapInit, nonHeapMax, nonHeapUsed;
	private MBeanInfo mbeanInfo;

	public MemoryContribution(MemoryMXBean delegate) {
		super(delegate);
		NotificationEmitter emitter = (NotificationEmitter) delegate;
		emitter.addNotificationListener(this, null, null);
		mbeanInfo = MBeanInfoWrapper.createMBeanInfo(delegate.getClass(), delegate.toString(), new MBeanAttributeInfo[0], new MBeanNotificationInfo[0]);

		new Thread() {
			public void run() {
				while (true) {
					if (MemoryContribution.this.memUsageChanged()) {
						MemoryContribution.this.contributionStateChanged(new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_UPDATED));
					}
					try {
						sleep(1500);
					} catch (InterruptedException e) {
					}
				}
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getName()
	 */
	protected String getName() {
		return VMStatsMessages.mem_title;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getProperties()
	 */
	protected Set<String> getProperties() {
		Set<String> result = new TreeSet<String>();
		MemoryMXBean memDelegate = (MemoryMXBean) contributionDelegate;

		MemoryUsage mu = memDelegate.getHeapMemoryUsage();
		result.add(VMStatsMessages.mem_heapusage);
		result.add(VMStatsMessages.mem_commited + ": " + (mu.getCommitted() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$
		result.add(VMStatsMessages.mem_initreq + ": " + (mu.getInit() >> 10) + "KB");  //$NON-NLS-1$//$NON-NLS-2$
		result.add(VMStatsMessages.mem_max + ": " + (mu.getMax() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$
		result.add(VMStatsMessages.mem_used + ": " + (mu.getUsed() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$

		mu = memDelegate.getNonHeapMemoryUsage();
		result.add(VMStatsMessages.mem_noheapusage);
		result.add(VMStatsMessages.mem_commited + ": " + (mu.getCommitted() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$
		result.add(VMStatsMessages.mem_initreq + ": " + (mu.getInit() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$
		result.add(VMStatsMessages.mem_max + ": " + (mu.getMax() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$
		result.add(VMStatsMessages.mem_used + ": " + (mu.getUsed() >> 10) + "KB"); //$NON-NLS-1$ //$NON-NLS-2$
		return result;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return mbeanInfo;
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
	public Object getAttribute(String attribute) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute attribute) {
		//
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
	 * @see javax.management.NotificationListener#handleNotification(javax.management.Notification, java.lang.Object)
	 */
	public void handleNotification(Notification notification, Object handback) {
		super.contributionStateChanged(new ContributionNotificationEvent(ContributionNotificationEvent.NOTIFICATION_UPDATED));
	}

	private boolean memUsageChanged() {
		MemoryMXBean mbean = (MemoryMXBean) contributionDelegate;
		MemoryUsage m = mbean.getHeapMemoryUsage();
		boolean changed = false;
		if (m.getCommitted() != heapCommited || m.getInit() != heapInit || m.getMax() != heapMax || m.getUsed() != heapUsed) {
			changed = true;
			heapCommited = m.getCommitted();
			heapInit = m.getInit();
			heapMax = m.getMax();
			heapUsed = m.getUsed();
		}
		m = mbean.getNonHeapMemoryUsage();
		if (m.getCommitted() != nonHeapCommited || m.getInit() != nonHeapInit || m.getMax() != nonHeapMax || m.getUsed() != nonHeapUsed) {
			changed = true;
			nonHeapCommited = m.getCommitted();
			nonHeapInit = m.getInit();
			nonHeapMax = m.getMax();
			nonHeapUsed = m.getUsed();
		}
		return changed;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return Activator.getImageLocation(ICON_PATH);
	}
}
