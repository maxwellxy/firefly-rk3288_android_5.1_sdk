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
package org.eclipse.equinox.jmx.internal.client.ui.contributionsview;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import javax.management.MBeanInfo;
import javax.management.MBeanOperationInfo;
import org.eclipse.equinox.jmx.common.ContributionProxy;
import org.eclipse.equinox.jmx.common.ContributionProxyMBean;
import org.eclipse.equinox.jmx.common.util.ByteArrayHolder;
import org.eclipse.equinox.jmx.internal.client.Activator;
import org.eclipse.equinox.jmx.internal.client.ui.util.ByteImageRegistry;
import org.eclipse.jface.viewers.*;
import org.eclipse.swt.graphics.Image;

/**
 * @since 1.0
 */
public class ContributionLabelProvider implements ILabelProvider {

	private ArrayList labelDecorators;
	private BundleStatusLabelDecorator bundleStatusDecorator = new BundleStatusLabelDecorator();
	private ByteImageRegistry imageRegistry = new ByteImageRegistry();

	public ContributionLabelProvider() {
		addLabelDecorator(bundleStatusDecorator);
	}

	/**
	 * Add a decorator to this label provider.
	 */
	public void addLabelDecorator(ILabelDecorator decorator) {
		if (labelDecorators == null) {
			labelDecorators = new ArrayList(2);
		}
		labelDecorators.add(decorator);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILabelProvider#getImage(java.lang.Object)
	 */
	public Image getImage(Object element) {
		if (element instanceof ContributionProxy) {
			ByteArrayHolder holder = ((ContributionProxy) element).getImageData();
			byte[] imageData;
			if (holder != null && (imageData = holder.value) != null) {
				try {
					return decorateImage(imageRegistry.getImage(imageData), element);
				} catch (IOException e) {
					Activator.log(e);
				}
			}
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILabelProvider#getText(java.lang.Object)
	 */
	public String getText(Object element) {
		if (element instanceof ContributionProxyMBean) {
			return decorateText(((ContributionProxyMBean) element).getName(), element);
		} else if (element instanceof MBeanInfo) {
			return decorateText(ContributionViewMessages.operations, element);
		} else if (element instanceof MBeanOperationInfo) {
			return ((MBeanOperationInfo) element).getName();
		}
		return decorateText(element.toString(), element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	public void addListener(ILabelProviderListener listener) {
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	public void dispose() {
		imageRegistry.dispose();
		labelDecorators.clear();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	public boolean isLabelProperty(Object element, String property) {
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	public void removeListener(ILabelProviderListener listener) {
	}

	protected String decorateText(String text, Object element) {
		if (labelDecorators != null && text.length() > 0) {
			Iterator iter = labelDecorators.iterator();
			while (iter.hasNext()) {
				ILabelDecorator decorator = (ILabelDecorator) iter.next();
				text = decorator.decorateText(text, element);
			}
		}
		return text;
	}

	protected Image decorateImage(Image image, Object element) {
		if (labelDecorators != null && image != null) {
			Iterator iter = labelDecorators.iterator();
			while (iter.hasNext()) {
				ILabelDecorator decorator = (ILabelDecorator) iter.next();
				image = decorator.decorateImage(image, element);
			}
		}
		return image;
	}
}
