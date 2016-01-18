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

import java.util.HashMap;
import java.util.Iterator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.util.Assert;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;

/**
 * A registry that maps <code>ImageDescriptors</code> to <code>Image</code>.
 * 
 * @since 1.0
 */
public class ImageRegistry {

	private HashMap registry = new HashMap(10);
	private Display display;

	/**
	 * Creates a new image descriptor registry for the current or default display,
	 * respectively.
	 */
	public ImageRegistry() {
		this(getStandardDisplay());
	}

	/**
	 * Creates a new image descriptor registry for the given display. All images
	 * managed by this registry will be disposed when the display gets disposed.
	 * 
	 * @param display the display the images managed by this registry are allocated for 
	 */
	public ImageRegistry(Display display) {
		this.display = display;
		Assert.isNotNull(display);
		hookDisplay();
	}

	/**
	 * Returns the image associated with the given image descriptor.
	 * 
	 * @param descriptor the image descriptor for which the registry manages an image
	 * @return the image associated with the image descriptor or <code>null</code>
	 *  if the image descriptor can't create the requested image.
	 */
	public Image get(ImageDescriptor descriptor) {
		Image result = (Image) registry.get(descriptor);
		if (result != null) {
			return result;
		}
		Assert.isTrue(display == getStandardDisplay(), "Allocating image for wrong display."); //$NON-NLS-1$
		result = descriptor.createImage();
		if (result != null) {
			registry.put(descriptor, result);
		}
		return result;
	}

	/**
	 * Disposes all images managed by this registry.
	 */
	public void dispose() {
		for (Iterator iter = registry.values().iterator(); iter.hasNext();) {
			Image image = (Image) iter.next();
			image.dispose();
		}
		registry.clear();
	}

	// free all image resources on dispose
	private void hookDisplay() {
		display.disposeExec(new Runnable() {
			public void run() {
				dispose();
			}
		});
	}

	/**
	 * Returns the standard display to be used. The method first checks, if
	 * the thread calling this method has an associated disaply. If so, this
	 * display is returned. Otherwise the method returns the default display.
	 */
	public static Display getStandardDisplay() {
		Display display;
		display = Display.getCurrent();
		if (display == null)
			display = Display.getDefault();
		return display;
	}
}