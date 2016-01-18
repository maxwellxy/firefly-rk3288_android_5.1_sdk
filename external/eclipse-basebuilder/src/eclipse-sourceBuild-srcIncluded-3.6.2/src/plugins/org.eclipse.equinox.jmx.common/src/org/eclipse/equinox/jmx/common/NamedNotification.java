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

import javax.management.Notification;
import javax.management.ObjectName;

public class NamedNotification extends Notification implements Comparable {

	private static final long serialVersionUID = 5605927554935255591L;

	private static long notificationCounter;
	private final long notificationId;
	private ObjectName name;

	public NamedNotification(ObjectName name, Notification notification) {
		super(notification.getType(), name, notification.getSequenceNumber(), notification.getTimeStamp(), notification.getMessage());
		this.notificationId = notificationCounter++;
		this.name = name;
	}

	public ObjectName getObjectName() {
		return name;
	}

	public long getNotificationId() {
		return notificationId;
	}

	/* (non-Javadoc)
	 * @see java.lang.Comparable#compareTo(T)
	 */
	public int compareTo(Object obj) {
		if (!(obj instanceof NamedNotification)) {
			if (obj instanceof Comparable) {
				return ((Comparable) obj).compareTo(this);
			}
			throw new ClassCastException();
		}
		NamedNotification nn = (NamedNotification) obj;
		int ret = getObjectName().toString().compareTo(nn.getObjectName().toString());
		return ret;
	}
}
