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

import java.util.*;
import javax.management.ObjectName;
import org.eclipse.core.runtime.Assert;
import org.eclipse.equinox.jmx.internal.common.Activator;

public class RootContribution extends ContributionProxy implements RootContributionMBean {
	private static final long serialVersionUID = 7227341044245163642L;

	public static final String CLASS_NAME = RootContribution.class.getName();
	public static final String NOTIFICATION_TYPE = "contributions"; //$NON-NLS-1$
	public static final String DEFAULT_DOMAIN = Activator.DEFAULT_DOMAIN;
	public static final String OBJECT_NAME_STR = DEFAULT_DOMAIN + ":type=" + CLASS_NAME; //$NON-NLS-1$
	public static ObjectName OBJECT_NAME;
	static {
		try {
			OBJECT_NAME = ObjectName.getInstance(OBJECT_NAME_STR);
		} catch (Exception e) {
			Activator.logError(e);
		}
	}
	private final Set contributions;

	/**
	 * Default constructor for root contribution item.
	 */
	public RootContribution() {
		this(new ContributionProxy[0]);
	}

	/**
	 * Constructor for allocating a new <code>RootContributions</code> object which
	 * contains the provided contributions as children.
	 * 
	 * @param rootContributionProxies The immediate children of this contribution.
	 */
	public RootContribution(ContributionProxy[] rootContributionProxies) {
		super("root", null, null, OBJECT_NAME, null); //$NON-NLS-1$
		this.contributions = new HashSet();
		updateContributionProxies(rootContributionProxies);
	}

	public void registerContributionProxy(ContributionProxy proxy) {
		Iterator iter = contributions.iterator();
		boolean addproxy = true;
		while (iter.hasNext()) {
			ContributionProxy rootProxy = (ContributionProxy) iter.next();
			if (rootProxy.getName().equals(proxy.getName())) {
				addproxy |= false;
				break;
			}
		}
		if (addproxy) {
			contributions.add(proxy);
		}
	}

	public void unregisterContributionProxy(ContributionProxy proxy) {
		Iterator iter = contributions.iterator();
		while (iter.hasNext()) {
			ContributionProxy rootProxy = (ContributionProxy) iter.next();
			if (rootProxy.getName().equals(proxy.getName())) {
				iter.remove();
				break;
			}
		}
	}

	public void updateContributionProxies(ContributionProxy[] proxies) {
		Assert.isNotNull(proxies);
		contributions.clear();
		for (int i = 0; i < proxies.length; i++) {
			this.contributions.add(proxies[i]);
		}
	}

	public Set getRootContributionProxies() {
		return contributions;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.common.RootContributionMBean#queryRootContribution()
	 */
	public RootContribution queryRootContribution() {
		return this;
	}

	/* (non-Javadoc)
	 * @see com.jmx.common.contrib.RootContributionMBean#getRootContributions()
	 */
	public ContributionProxy[] queryRootContributions() {
		return (ContributionProxy[]) contributions.toArray(new ContributionProxy[contributions.size()]);
	}
}
