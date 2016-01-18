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
package org.eclipse.equinox.jmx.internal.client.ui.viewsupport;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;

/**
 * @since 1.0
 */
public class ImageImageDescriptor extends ImageDescriptor {

	private Image image;

	/**
	 * Constructor for ImagImageDescriptor.
	 */
	public ImageImageDescriptor(Image image) {
		super();
		this.image = image;
	}

	/* (non-Javadoc)
	 * @see ImageDescriptor#getImageData()
	 */
	public ImageData getImageData() {
		return image.getImageData();
	}

	/* (non-Javadoc)
	 * @see Object#equals(Object)
	 */
	public boolean equals(Object obj) {
		return (obj != null) && getClass().equals(obj.getClass()) && image.equals(((ImageImageDescriptor) obj).image);
	}

	/* (non-Javadoc)
	 * @see Object#hashCode()
	 */
	public int hashCode() {
		return image.hashCode();
	}

}