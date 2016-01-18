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

import org.osgi.framework.*;
import org.osgi.service.packageadmin.PackageAdmin;
import org.osgi.util.tracker.ServiceTracker;

/**
 * The activator for this bundle.
 * 
 * @since 1.0
 */
public class Activator implements BundleActivator {

	private static BundleContext context;
	private static ServiceTracker bundleTracker;

	/**
	 * The constructor.
	 */
	public Activator() {
		super();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.Plugin#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext bundleContext) throws Exception {
		Activator.context = bundleContext;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.Plugin#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext bundleContext) throws Exception {
		if (bundleTracker != null) {
			bundleTracker.close();
			bundleTracker = null;
		}
		Activator.context = null;
	}

	/**
	 * @return The bundle context.
	 */
	public static BundleContext getBundleContext() {
		return context;
	}

	public static Bundle getBundle() {
		return context.getBundle();
	}

	/**
	 * Return the resolved bundle with the specified symbolic name.
	 * 
	 * @see PackageAdmin#getBundles(String, String)
	 */
	public static Bundle getBundle(String symbolicName) {
		if (bundleTracker == null) {
			bundleTracker = new ServiceTracker(getBundleContext(), PackageAdmin.class.getName(), null);
			bundleTracker.open();
		}
		PackageAdmin admin = (PackageAdmin) bundleTracker.getService();
		if (admin == null)
			return null;
		Bundle[] bundles = admin.getBundles(symbolicName, null);
		if (bundles == null)
			return null;
		//Return the first bundle that is not installed or uninstalled
		for (int i = 0; i < bundles.length; i++) {
			if ((bundles[i].getState() & (Bundle.INSTALLED | Bundle.UNINSTALLED)) == 0) {
				return bundles[i];
			}
		}
		return null;
	}
}
