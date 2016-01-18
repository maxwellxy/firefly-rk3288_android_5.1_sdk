/*******************************************************************************
 * Copyright (c) 2005, 2008 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 * IBM Corporation - initial API and implementation
 * yyyymmdd   bug    Email and other contact information
 * -------- -------- -----------------------------------------------------------
 * 20070201   154100 pmoogk@ca.ibm.com - Peter Moogk, Port internet code from WTP to Eclipse base.
 *******************************************************************************/
package org.eclipse.ui.internal.net;

import java.util.Collection;

import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.Viewer;

public class NonProxyHostsContentProvider implements IStructuredContentProvider {

	public NonProxyHostsContentProvider() {
		super();
	}

	public void dispose() {
		// Do nothing
	}

	public Object[] getElements(Object inputElement) {
		Collection coll = (Collection) inputElement;
		return coll.toArray(new ProxyBypassData[0]);
	}

	public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		// Do nothing
	}

}
