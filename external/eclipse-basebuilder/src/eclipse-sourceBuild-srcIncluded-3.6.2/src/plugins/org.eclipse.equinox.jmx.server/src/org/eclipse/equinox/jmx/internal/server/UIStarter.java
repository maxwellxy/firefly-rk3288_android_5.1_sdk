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

import org.eclipse.core.runtime.Assert;
import org.eclipse.ui.IStartup;
import org.osgi.framework.Bundle;
import org.osgi.framework.BundleException;

public class UIStarter implements IStartup {

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	public void earlyStartup() {
		// we really don't need to do anything as the plugin.xml is configured such that
		// the Activator's start() has is invoked (before earlyStartup()) when a class belonging 
		// to this plugin is loaded, but we will make sure that no one has disabled that option and perform
		// the check...
		Bundle bundle = Activator.getDefault().getBundleContext().getBundle();
		Assert.isNotNull(bundle); // this should not be null as this code exists within the referenced bundle
		if ((bundle.getState() & (Bundle.STARTING | Bundle.ACTIVE)) == 0) {
			// start the bundle
			try {
				bundle.start();
			} catch (BundleException e) {
				Activator.log(e);
			}
		}
	}
}
