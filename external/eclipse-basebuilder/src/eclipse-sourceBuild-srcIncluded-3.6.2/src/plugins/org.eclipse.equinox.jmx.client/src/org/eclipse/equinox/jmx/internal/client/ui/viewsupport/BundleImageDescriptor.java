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

import org.eclipse.jface.resource.CompositeImageDescriptor;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.osgi.framework.Bundle;

/**
 * @since 1.0
 */
public class BundleImageDescriptor extends CompositeImageDescriptor {

	private ImageDescriptor baseDescriptor;
	private int bundleState;
	private Point size;

	public BundleImageDescriptor(ImageDescriptor baseDescriptor, int bundleState, Point size) {
		this.baseDescriptor = baseDescriptor;
		this.bundleState = bundleState;
		this.size = size;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.resource.CompositeImageDescriptor#drawCompositeImage(int, int)
	 */
	protected void drawCompositeImage(int width, int height) {
		ImageData bg = getImageData(baseDescriptor);
		// draw the base image as background
		drawImage(bg, 0, 0);
		// draw the status overlay in the bottom left
		drawBottomLeft();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.resource.CompositeImageDescriptor#getSize()
	 */
	protected Point getSize() {
		return size;
	}

	private void drawBottomLeft() {
		Point size = getSize();
		int x = 0; // apply additional icons?
		ImageData data = null;
		switch (bundleState) {
			case Bundle.ACTIVE :
				data = getImageData(JMXPluginImages.IMGDESC_OVR_BUNDLE_ACTIVE);
				break;
			case Bundle.INSTALLED :
				data = getImageData(JMXPluginImages.IMGDESC_OVR_BUNDLE_INSTALLED);
				break;
			case Bundle.RESOLVED :
				data = getImageData(JMXPluginImages.IMGDESC_OVR_BUNDLE_RESOLVED);
				break;
			default :
				return;
		}
		drawImage(data, x, size.y - data.height);
		x += data.width;
	}

	private ImageData getImageData(ImageDescriptor descriptor) {
		ImageData data = descriptor.getImageData();
		if (data == null) {
			data = DEFAULT_IMAGE_DATA;
		}
		return data;
	}
}
