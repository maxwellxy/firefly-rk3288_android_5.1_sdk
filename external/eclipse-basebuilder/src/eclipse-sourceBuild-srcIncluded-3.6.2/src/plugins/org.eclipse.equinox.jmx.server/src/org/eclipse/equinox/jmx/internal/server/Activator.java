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
package org.eclipse.equinox.jmx.internal.server;

import java.util.*;
import javax.management.*;
import javax.management.remote.JMXConnectorServer;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.equinox.jmx.common.*;
import org.eclipse.equinox.jmx.internal.server.ServerExtensionManager.ContributionExtensionDefinition;
import org.eclipse.equinox.jmx.server.ContributionProvider;
import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.service.log.LogService;
import org.osgi.util.tracker.ServiceTracker;

/**
 * The plug-in startup class for the jmx server plug-in.
 * 
 * @since 1.0
 */
public class Activator implements BundleActivator {

	static final String PLUGIN_ID = "org.eclipse.equinox.jmx.server"; //$NON-NLS-1$;
	static final String PI_NAMESPACE = PLUGIN_ID;
	static final String PT_CONTRIBUTION = "contribution"; //$NON-NLS-1$
	static final String PT_PROVIDER = "provider"; //$NON-NLS-1$

	static final String PROTOCOL_PROPERTY_KEY = PI_NAMESPACE + ".protocol"; //$NON-NLS-1$
	static final String PORT_PROPERTY_KEY = PI_NAMESPACE + ".port"; //$NON-NLS-1$
	static final String DOMAIN_PROPERTY_KEY = PI_NAMESPACE + ".domain"; //$NON-NLS-1$

	//The shared instance.
	private static Activator instance;
	private static BundleContext bundleContext;
	private static JMXConnectorServer jmxServer;
	private static RootContribution rootContribution;
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
		if (jmxServer == null) {
			createServer();
		}
		jmxServer.start();
	}

	/**
	 * Create the jmx server.  This should only be invoked once - the first time this bundle is started.
	 * 
	 * @throws Exception If an exception occurs when attempting to start the server.
	 */
	public void createServer() throws Exception {
		// determine which protocol to use
		String protocol = System.getProperty(PROTOCOL_PROPERTY_KEY);
		if (protocol == null) {
			protocol = JMXConstants.DEFAULT_PROTOCOL;
		}
		// determine port to listen on
		int port;
		String strPort = System.getProperty(PORT_PROPERTY_KEY);
		if (strPort == null) {
			port = JMXConstants.DEFAULT_PORT_AS_INT;
		} else {
			try {
				port = Integer.parseInt(strPort);
			} catch (NumberFormatException nfe) {
				log(nfe);
				port = JMXConstants.DEFAULT_PORT_AS_INT;
			}
		}
		String domain = System.getProperty(DOMAIN_PROPERTY_KEY);
		if (domain == null) {
			domain = JMXConstants.DEFAULT_DOMAIN;
		}
		jmxServer = JMXServerFactory.createJMXServer("127.0.0.1", port, protocol, JMXConstants.DEFAULT_DOMAIN, null);
		registerContributions();
	}

	private void registerContributions() throws IntrospectionException, ReflectionException, MBeanRegistrationException, NotCompliantMBeanException {
		final MBeanServer mbeanServer = jmxServer.getMBeanServer();
		try {
			mbeanServer.getMBeanInfo(RootContribution.OBJECT_NAME);
		} catch (Exception e) {
			//load extensions and add to contribution model
			Collection providers = ServerExtensionManager.getInstance().getContributionExtensionDefinitions();
			Iterator iter = providers.iterator();
			List proxiesToRegister = new ArrayList();
			while (iter.hasNext()) {
				ContributionExtensionDefinition defn = (ContributionExtensionDefinition) iter.next();
				ContributionProvider provider = defn.getContributionProvider();
				// register the providers with the mbean server
				provider.registerContribution(mbeanServer);
				if (defn.isRootProvider()) {
					proxiesToRegister.add(provider.createProxy());
				}
			}
			rootContribution = new RootContribution((ContributionProxy[]) proxiesToRegister.toArray(new ContributionProxy[proxiesToRegister.size()]));
			try {
				mbeanServer.registerMBean(rootContribution, RootContribution.OBJECT_NAME);
			} catch (Exception e1) {
				// should not occur since we previously checked for existence
			}
			ServerExtensionManager.getInstance().addObserver(new Observer() {
				public void update(Observable o, Object arg) {
					if (!(arg instanceof ContributionExtensionDefinition)) {
						return;
					}
					ContributionExtensionDefinition defn = (ContributionExtensionDefinition) arg;
					ContributionProvider rootProvider = defn.getContributionProvider();
					if (ServerExtensionManager.getInstance().getContributionExtensionDefinition(defn.getProviderClassName()) == null) {
						// root provider has been removed
						rootContribution.unregisterContributionProxy(rootProvider.createProxy());
						rootProvider.sendNotification(new Notification(ContributionNotificationEvent.NOTIFICATION_REMOVED, rootProvider, 0));
					} else {
						// new root provider installed or updated
						try {
							rootProvider.registerContribution(mbeanServer);
							rootContribution.registerContributionProxy(rootProvider.createProxy());
							rootContribution.sendNotification(new Notification(ContributionNotificationEvent.NOTIFICATION_UPDATED, rootContribution, 0));
						} catch (Exception e) {
							log(e);
						}
					}
				}
			});
		}
	}

	public static RootContribution getRootContribution() {
		return rootContribution;
	}

	/* (non-Javadoc)
	 * @see org.osgi.framework.BundleActivator#stop(org.osgi.framework.BundleContext)
	 */
	public void stop(BundleContext context) throws Exception {
		jmxServer.stop();
		if (logService != null) {
			logService.close();
			logService = null;
		}
		instance = null;
	}

	/**
	 * @return The bundle context.
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
	 * Return the underlying <code>Server</code> which provides an interface to the jmx agent.
	 * 
	 * @return The server.
	 */
	public MBeanServer getServer() {
		return jmxServer.getMBeanServer();
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
				message = ServerMessages.exception_occurred;
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
