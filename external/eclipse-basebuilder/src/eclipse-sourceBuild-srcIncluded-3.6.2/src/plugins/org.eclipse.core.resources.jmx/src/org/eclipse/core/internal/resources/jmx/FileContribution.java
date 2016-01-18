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

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;

/**
 * @since 1.0
 */
public class FileContribution extends ResourceContribution {

	private static final IResource[] EMPTY_ARRAY = new IResource[0];
	private static final String ICON_PATH = "icons/file.gif";//$NON-NLS-1$

	/*
	 * Constructor for the class.
	 */
	public FileContribution(IFile file) {
		super(file);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.internal.resources.jmx.ResourceContribution#getIconPath()
	 */
	protected IPath getIconPath() {
		return new Path(ICON_PATH);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return EMPTY_ARRAY;
	}

}
