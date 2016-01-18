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

import java.net.URL;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.internal.client.Activator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.osgi.framework.Bundle;

/**
 * @since 1.0
 */
public class JMXPluginImages {

	public static final IPath ICONS_PATH = new Path("$nl$/icons/"); //$NON-NLS-1$
	public static final IPath ICONS_MB_PATH = new Path(ICONS_PATH.append("mb/").toString()); //$NON-NLS-1$

	private static ImageRegistry registry;

	public static final Image IMG_MANAGED_BUNDLE = createManaged(ICONS_MB_PATH, "bundle.gif"); //$NON-NLS-1$
	public static final Image IMG_MANAGED_COMPONENT = createManaged(ICONS_MB_PATH, "component.gif"); //$NON-NLS-1$
	public static final Image IMG_MBEAN_OPERATION = createManaged(ICONS_MB_PATH, "mbean_operation.gif"); //$NON-NLS-1$
	public static final Image IMG_MBEAN_OPERATIONS = createManaged(ICONS_MB_PATH, "mbean_operations.gif"); //$NON-NLS-1$
	public static final Image IMG_MBEAN_OPERATIONS_NONE = createManaged(ICONS_MB_PATH, "mbean_operation_none.gif"); //$NON-NLS-1$

	public static final ImageDescriptor IMGDESC_OVR_BUNDLE_ACTIVE = create("mb/", "ovr_bundle_active.gif", true); //$NON-NLS-1$ //$NON-NLS-2$
	public static final ImageDescriptor IMGDESC_OVR_BUNDLE_INSTALLED = create("mb/", "ovr_bundle_installed.gif", true); //$NON-NLS-1$ //$NON-NLS-2$
	public static final ImageDescriptor IMGDESC_OVR_BUNDLE_RESOLVED = create("mb/", "ovr_bundle_resolved.gif", true); //$NON-NLS-1$ //$NON-NLS-2$

	public static Image createManaged(IPath path, String name) {
		ImageDescriptor desc = createImageDescriptor(Activator.getDefault().getBundle(), path.append(name), true);
		if (registry == null) {
			registry = new ImageRegistry();
		}
		return registry.get(desc);
	}

	public Image getImage(ImageDescriptor descriptor) {
		return registry.get(descriptor);
	}

	public static ImageDescriptor createImageDescriptor(Bundle bundle, IPath path, boolean useMissingImageDescriptor) {
		URL url = FileLocator.find(bundle, path, null);
		if (url != null) {
			return ImageDescriptor.createFromURL(url);
		}
		if (useMissingImageDescriptor) {
			return ImageDescriptor.getMissingImageDescriptor();
		}
		return null;
	}

	private static ImageDescriptor create(String prefix, String name, boolean useMissingImageDescriptor) {
		IPath path = ICONS_PATH.append(prefix).append(name);
		return createImageDescriptor(Activator.getDefault().getBundle(), path, useMissingImageDescriptor);
	}
}
