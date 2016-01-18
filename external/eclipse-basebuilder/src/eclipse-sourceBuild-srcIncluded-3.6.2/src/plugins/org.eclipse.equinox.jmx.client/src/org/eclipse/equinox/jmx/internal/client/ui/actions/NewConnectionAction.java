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
package org.eclipse.equinox.jmx.internal.client.ui.actions;

import java.io.IOException;
import java.util.*;
import javax.management.*;
import javax.management.remote.JMXConnector;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.client.IJMXConnectorProvider;
import org.eclipse.equinox.jmx.common.JMXConstants;
import org.eclipse.equinox.jmx.internal.client.Activator;
import org.eclipse.equinox.jmx.internal.client.MBeanServerProxy;
import org.eclipse.equinox.jmx.internal.client.ui.ClientUI;
import org.eclipse.equinox.jmx.internal.client.ui.contributionsview.ContributionsViewPart;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.*;

/**
 * @since 1.0
 */
public class NewConnectionAction implements IWorkbenchWindowActionDelegate {

	protected IWorkbenchWindow window;
	protected JMXConnector connector;
	private MBeanServerProxy serverProxy;
	private boolean connected;
	private boolean isClosing;
	private boolean isConnecting;
	//container for available transports
	private static Map transports;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
	 */
	public void init(IWorkbenchWindow window) {
		this.window = window;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	public void run(final IAction action) {
		// invalidate the server proxy
		serverProxy = null;
		if (!connected) {
			MBeanServer mbeanServer = findLocalMBeanServer();
			if (mbeanServer != null) {
				// ask if user wishes to connect to local server
				if (MessageDialog.openConfirm(window.getShell(), ActionMessages.info_message, ActionMessages.local_mbean_server)) {
					serverProxy = new MBeanServerProxy(mbeanServer);
					action.setText(ActionMessages.connect_host);
					connected = false;
					loadContributionsView();
					return;
				}
			}
			// if we did not locate the server locally, prompt for location
			if (serverProxy == null) {
				loadTransportExtensions();
				if (transports.size() == 0) {
					MessageDialog.openInformation(null, ActionMessages.info_message, ActionMessages.no_transports_available);
					return;
				}
				ConnectionSelectionDialog dialog = new ConnectionSelectionDialog(window.getShell(), transports);
				if (dialog.open() != Window.OK) {
					return;
				}
				try {
					connector = dialog.getJMXConnector();
					isConnecting = true;
					connector.addConnectionNotificationListener(new NotificationListener() {
						public void handleNotification(Notification notification, Object handback) {
							if (isClosing || isConnecting) {
								// receiving callback from user action
								return;
							}
							connected = false;
							connector = null;
							serverProxy = null;
							action.setText(ActionMessages.connect_host);
							window.getShell().getDisplay().asyncExec(new Runnable() {
								public void run() {
									MessageDialog.openError(window.getShell(), ActionMessages.info_message, ActionMessages.server_closed_connection);
								}
							});
						}
					}, null, null);
					connector.connect();
					isConnecting = false;
					serverProxy = new MBeanServerProxy(connector.getMBeanServerConnection());
				} catch (Exception e) {
					Activator.logError(e.getMessage(), e);
					MessageDialog.openError(window.getShell(), ActionMessages.error_message, e.getMessage());
				}
			}
			loadContributionsView();
			connected = true;
			action.setText(ActionMessages.disconnect_action);
		} else {
			// check if we must close a remote conneciton
			if (connector != null) {
				try {
					isClosing = true;
					connector.close();
					isClosing = false;
					connector = null;
				} catch (IOException e) {
					MessageDialog.openError(window.getShell(), ActionMessages.error_message, e.getMessage());
					Activator.log(e);
				}
			}
			connected = false;
			serverProxy = null;
			loadContributionsView();
			action.setText(ActionMessages.connect_host);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	public void selectionChanged(IAction action, ISelection selection) {
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
	 */
	public void dispose() {
	}

	private MBeanServer findLocalMBeanServer() {
		// null returns all mbeans servers in this vm, we do this because otherwise
		// we would have to specify an agentID that is the domain name appended with
		// the timestamp that the server was started.
		ArrayList mbeanServers = MBeanServerFactory.findMBeanServer(null);
		Iterator iter = mbeanServers.iterator();
		while (iter.hasNext()) {
			MBeanServer mbeanServer = (MBeanServer) iter.next();
			if (mbeanServer.getDefaultDomain().equals(JMXConstants.DEFAULT_DOMAIN)) {
				return mbeanServer;
			}
		}
		return null;
	}

	private void loadContributionsView() {
		try {
			ContributionsViewPart contributionsView = (ContributionsViewPart) PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().showView(ClientUI.VIEWID_CONTRIBUTIONS);
			if (contributionsView != null) {
				contributionsView.setMBeanServerProxy(serverProxy);
			}
		} catch (PartInitException e) {
			Activator.log(e);
		}
	}

	private static void loadTransportExtensions() {
		if (transports == null) {
			transports = new HashMap();
		}
		IExtensionPoint point = RegistryFactory.getRegistry().getExtensionPoint(Activator.PI_NAMESPACE, Activator.PT_TRANSPORT);
		IExtension[] types = point.getExtensions();
		for (int i = 0; i < types.length; i++) {
			loadTransportConfigurationElements(types[i].getConfigurationElements());
		}
	}

	private static void loadTransportConfigurationElements(IConfigurationElement[] configElems) {
		for (int j = 0; j < configElems.length; j++) {
			IConfigurationElement element = configElems[j];
			final String elementName = element.getName();
			String transport;
			if (elementName.equals(Activator.PT_TRANSPORT) && null != element.getAttribute("class") //$NON-NLS-1$
					&& null != (transport = element.getAttribute("protocol"))) //$NON-NLS-1$
			{
				try {
					Object obj = element.createExecutableExtension("class"); //$NON-NLS-1$
					if (obj instanceof IJMXConnectorProvider) {
						transports.put(transport, obj);
					}
				} catch (CoreException e) {
					Activator.log(e);
				}
			}
		}
	}
}
