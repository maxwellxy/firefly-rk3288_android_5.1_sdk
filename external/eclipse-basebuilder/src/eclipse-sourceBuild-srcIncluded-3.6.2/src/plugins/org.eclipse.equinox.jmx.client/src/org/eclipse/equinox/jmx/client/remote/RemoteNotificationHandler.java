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
package org.eclipse.equinox.jmx.client.remote;

import java.util.*;
import javax.management.*;
import org.eclipse.equinox.jmx.common.NamedNotification;
import org.eclipse.equinox.jmx.internal.client.Activator;

public class RemoteNotificationHandler {

	private int NOTIFICATION_RETRIEVAL_INTERVAL = 5000;

	private final Map objectNameListeners = new HashMap();
	private final NotificationRetriever retriever = new NotificationRetriever();
	private final NotificationDispatcher dispatcher = new NotificationDispatcher();
	private final RemoteMBeanConnection connection;
	private boolean started;

	public RemoteNotificationHandler(RemoteMBeanConnection connection) {
		this.connection = connection;
	}

	public void addNotificationListener(ObjectName name, NotificationListener listener, NotificationFilter filter, Object handback) {
		if (!started) {
			start();
		}
		synchronized (objectNameListeners) {
			List listeners = null;
			if ((listeners = (List) objectNameListeners.get(name)) == null) {
				listeners = new ArrayList();
				objectNameListeners.put(name, listeners);
			}
			listeners.add(listener);
		}
	}

	public synchronized void start() {
		if (started) {
			return;
		}
		started = true;
		retriever.start();
		dispatcher.start();
	}

	public synchronized void stop() {
		started = false;
	}

	private void dispatchNotification(NamedNotification notification) {
		ObjectName name = notification.getObjectName();
		synchronized (objectNameListeners) {
			List listeners = (List) objectNameListeners.get(name);
			if (listeners != null) {
				Iterator iter = listeners.iterator();
				while (iter.hasNext()) {
					((NotificationListener) iter.next()).handleNotification(notification, null);
				}
			}
		}
	}

	private class NotificationRetriever extends Thread {

		private long startId;

		public NotificationRetriever() {
			super();
		}

		public void run() {
			while (started) {
				try {
					NamedNotification[] result = connection.retrieveNotifications(startId);
					if (result.length > 0) {
						dispatcher.dispatchNotifications(result);
						startId = result[result.length - 1].getNotificationId() + 1;
						synchronized (this) {
							this.wait(NOTIFICATION_RETRIEVAL_INTERVAL);
						}
					}
				} catch (Exception e) {
					Activator.log(e);
					break;
				}
			}
			started = false;
		}
	}

	private class NotificationDispatcher extends Thread {

		private List notificationQueue = new ArrayList();

		public NotificationDispatcher() {
			super();
		}

		public void dispatchNotifications(NamedNotification[] notifications) {
			synchronized (notificationQueue) {
				boolean added = false;
				for (int i = 0; i < notifications.length; i++) {
					if (notifications[i] != null) {
						added |= notificationQueue.add(notifications[i]);
					}
				}
				if (added) {
					notificationQueue.notifyAll();
				}
			}
		}

		public void run() {
			outer: while (started && !Thread.currentThread().isInterrupted()) {
				synchronized (notificationQueue) {
					while (notificationQueue.isEmpty()) {
						try {
							notificationQueue.wait();
						} catch (InterruptedException e) {
							Activator.log(e);
							Thread.currentThread().interrupt();
							continue outer;
						}
					}
					while (notificationQueue.size() > 0) {
						NamedNotification nn = (NamedNotification) notificationQueue.remove(0);
						dispatchNotification(nn);
					}
				}
			}
			started = false;
		}
	}
}
