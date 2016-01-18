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
package org.eclipse.equinox.internal.initializer;

import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtension;
import org.eclipse.core.runtime.IExtensionPoint;
import org.eclipse.core.runtime.IPlatformRunnable;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.osgi.util.NLS;

public class Initializer implements IPlatformRunnable {

	public static final String PT_INITIALIZERS = "initializers"; //$NON-NLS-1$
	public static final String PI_INITIALIZER = "org.eclipse.equinox.initializer";

	public Object run(Object args) throws Exception {
		IExtensionPoint xpt = Platform.getExtensionRegistry().getExtensionPoint(PI_INITIALIZER, PT_INITIALIZERS);
		if (xpt == null)
			return IPlatformRunnable.EXIT_OK;
		
		IExtension[] exts = xpt.getExtensions();
		for (int i = 0; i < exts.length; i++) {
			IConfigurationElement[] configs = exts[i].getConfigurationElements();
			IPlatformRunnable initializer = null;
			if (configs.length != 0) {
				try {
					initializer = (IPlatformRunnable) configs[0].createExecutableExtension("initialize"); //$NON-NLS-1$
					initializer.run(args);
				} catch(CoreException e) {
					String msg = NLS.bind(Messages.initializer_error, exts[i].getExtensionPointUniqueIdentifier());
					IStatus status = new Status(IStatus.ERROR, PI_INITIALIZER, Platform.PLUGIN_ERROR, msg, e);
					Activator.log(status);
					return null;
				}
			}
		}
		return IPlatformRunnable.EXIT_OK;
	}

}
