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
package org.eclipse.equinox.internal.preferences.jmx;

import org.eclipse.core.runtime.preferences.IPreferencesService;
import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.service.log.LogService;
import org.osgi.util.tracker.ServiceTracker;

/**
 * @since 1.0
 */
public class Activator implements BundleActivator {

	static final String BUNDLE_NAME = "org.eclipse.equinox.preferences.jmx"; //$NON-NLS-1$

	private static BundleContext context = null;
	private static ServiceTracker preferenceService = null;
	private static ServiceTracker logService = null;

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleActivator#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext bundleContext) throws Exception {
		context = bundleContext;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleActivator#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext bundleContext) throws Exception {
		if (preferenceService != null) {
			preferenceService.close();
			preferenceService = null;
		}
		if (logService != null) {
			logService.close();
			logService = null;
		}
		context = null;
	}

	/*
	 * Return the preference service, if available.
	 */
	public static IPreferencesService getPreferenceService() {
		if (preferenceService == null) {
			preferenceService = new ServiceTracker(context, IPreferencesService.class.getName(), null);
			preferenceService.open();
		}
		return (IPreferencesService) preferenceService.getService();
	}

	/*
	 * Log the given error.
	 */
	public static void log(String message, Exception exception) {
		if (logService == null) {
			logService = new ServiceTracker(context, LogService.class.getName(), null);
			logService.open();
		}
		LogService log = (LogService) logService.getService();
		if (log == null) {
			System.out.println(message);
			if (exception != null)
				exception.printStackTrace(System.out);
		} else
			log.log(LogService.LOG_ERROR, message, exception);
	}

	public static BundleContext getContext() {
		return context;
	}
}
