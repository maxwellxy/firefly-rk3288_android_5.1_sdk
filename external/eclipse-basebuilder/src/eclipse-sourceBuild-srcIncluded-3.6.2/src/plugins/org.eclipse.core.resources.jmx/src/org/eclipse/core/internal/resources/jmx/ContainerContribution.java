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
package org.eclipse.core.internal.resources.jmx;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;

/**
 * @since 1.0
 */
public abstract class ContainerContribution extends ResourceContribution {

	private static final IResource[] EMPTY_ARRAY = new IResource[0];

	/*
	 * Constructor for the class.
	 */
	public ContainerContribution(IContainer container) {
		super(container);
	}

	private IContainer getDelegate() {
		return (IContainer) contributionDelegate;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		try {
			return getDelegate().members();
		} catch (CoreException e) {
			return EMPTY_ARRAY;
		}
	}

}
