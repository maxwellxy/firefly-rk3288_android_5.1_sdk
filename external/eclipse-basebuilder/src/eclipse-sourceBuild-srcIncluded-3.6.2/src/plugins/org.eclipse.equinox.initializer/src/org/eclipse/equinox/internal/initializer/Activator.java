/*******************************************************************************
 * Copyright (c) 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     Red Hat, Inc. and IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.equinox.internal.initializer;

import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.osgi.framework.*;

public class Activator implements BundleActivator {
	private static BundleContext context;

	public void start(BundleContext context) throws Exception {
		Activator.context = context;
	}

	public void stop(BundleContext context) throws Exception {
		Activator.context = null;
	}

	static BundleContext getContext() {
		return context;
	}

	public static void log(IStatus status) {
		BundleContext current = context;
		if (current == null)
			return;
		Platform.getLog(current.getBundle()).log(status);
	}
}
