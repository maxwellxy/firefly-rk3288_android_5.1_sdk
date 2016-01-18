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

public class ContributionNotificationEvent {

	public static final String NOTIFICATION_REMOVED = "contribution.removed"; //$NON-NLS-1$
	public static final String NOTIFICATION_ADDED = "contribution.added";//$NON-NLS-1$
	public static final String NOTIFICATION_UPDATED = "contribution.updated";//$NON-NLS-1$

	private final String eventType;

	public ContributionNotificationEvent(String eventType) {
		this.eventType = eventType;
	}

	public String getType() {
		return eventType;
	}
}
