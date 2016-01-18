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
package org.eclipse.equinox.jmx.internal.common;

import org.eclipse.core.runtime.IStatus;
import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.service.log.LogService;
import org.osgi.util.tracker.ServiceTracker;

public class Activator implements BundleActivator {

	// default server settings
	public static final String DEFAULT_DOMAIN = "jmxserver"; //$NON-NLS-1$
	public static final String DEFAULT_PORT = "3600"; //$NON-NLS-1$
	public static final int DEFAULT_PORT_AS_INT = 3600;
	public static final String DEFAULT_HOST_PORT = "127.0.0.1:" + DEFAULT_PORT; //$NON-NLS-1$

	public static final String PLUGIN_ID = "org.eclipse.equinox.jmx.common"; //$NON-NLS-1$

	// id constants of available extension points
	public static final String PT_CONTRIBUTIONUI = "contributionui"; //$NON-NLS-1$

	//The shared instance.
	private static Activator instance;
	private static BundleContext bundleContext;
	private static ServiceTracker logService;

	/**
	 * The constructor.
	 */
	public Activator() {
		instance = this;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleActivator#start(org.osgi.framework.BundleContext)
	 */
	public void start(BundleContext context) throws Exception {
		bundleContext = context;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleActivator#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext context) throws Exception {
		instance = null;
		if (logService != null) {
			logService.close();
			logService = null;
		}
	}

	/**
	 * Return this bundle's context.
	 * 
	 * @return the bundle context
	 */
	public BundleContext getBundleContext() {
		return bundleContext;
	}

	/**
	 * Returns the shared instance.
	 */
	public static Activator getDefault() {
		return instance;
	}

	/**
	 * Log the given message and exception to the log file.
	 * 
	 * @param message The message to log.
	 * @param exception The exception to log.
	 * @param iStatusSeverity The <code>IStatus</code> severity level.
	 */
	public static void log(String message, Throwable exception, int iStatusSeverity) {
		if (message == null) {
			message = exception.getMessage();
			if (message == null)
				message = CommonMessages.exception_occurred;
		}
		if (logService == null) {
			logService = new ServiceTracker(bundleContext, LogService.class.getName(), null);
			logService.open();
		}
		LogService log = (LogService) logService.getService();
		int severity = LogService.LOG_INFO;
		switch (iStatusSeverity) {
			case IStatus.ERROR :
				severity = LogService.LOG_ERROR;
				break;
			case IStatus.WARNING :
				severity = LogService.LOG_WARNING;
				break;
			case IStatus.INFO :
			default :
				severity = LogService.LOG_INFO;
				break;
		}
		if (log == null) {
			System.out.println(PLUGIN_ID);
			System.out.println(severity);
			System.out.println(message);
			if (exception != null)
				exception.printStackTrace(System.out);
		} else
			log.log(severity, message, exception);
	}

	/**
	 * Log the given message and exception to the log file with a
	 * status code of <code>IStatus.ERROR</code>.
	 * 
	 * @param message The message to log.
	 * @param exception The thrown exception.
	 */
	public static void logError(String message, Throwable exception) {
		log(message, exception, IStatus.ERROR);
	}

	/**
	 * Log the given exception to the log file with a
	 * status code of <code>IStatus.ERROR</code>.
	 *
	 * @param exception The thrown exception.
	 */
	public static void logError(Throwable exception) {
		log(exception.getMessage(), exception, IStatus.ERROR);
	}

	/**
	 * Log the given message to the log file with a
	 * status code of <code>IStatus.INFO</code>.
	 * 
	 * @param message The message to log.
	 */
	public static void log(String message) {
		log(message, null, IStatus.INFO);
	}

	/**
	 * Log the given exception to the log file with a
	 * status code of <code>IStatus.INFO</code>.
	 * 
	 * @param exception The thrown exception.
	 */
	public static void log(Throwable exception) {
		log(exception.getMessage(), exception, IStatus.INFO);
	}

}
