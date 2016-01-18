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
package org.eclipse.equinox.jmx.internal.client.ui.util;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.equinox.jmx.internal.client.ui.viewsupport.ImageRegistry;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;

/**
 * Extended image registry to support byte array keys.
 * 
 * @since 1.0
 */
public class ByteImageRegistry extends ImageRegistry {

	Map cache;

	public Image getImage(byte[] imageData) throws IOException {
		Image result = (Image) getCache().get(imageData);
		if (result == null) {
			getCache().put(imageData, result = super.get(createImageDescriptorFromByteArray(imageData)));
		}
		return result;
	}

	public Map getCache() {
		if (cache == null) {
			cache = new HashMap(10);
		}
		return cache;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.internal.client.ui.viewsupport.ImageRegistry#dispose()
	 */
	public void dispose() {
		getCache().clear();
		super.dispose();
	}

	public static ImageDescriptor createImageDescriptorFromByteArray(byte[] imageData) throws IOException {
		ByteArrayInputStream is = new ByteArrayInputStream(imageData);
		ImageDescriptor desc = ImageDescriptor.createFromImageData(new ImageData(is));
		is.close();
		return desc;
	}
}
