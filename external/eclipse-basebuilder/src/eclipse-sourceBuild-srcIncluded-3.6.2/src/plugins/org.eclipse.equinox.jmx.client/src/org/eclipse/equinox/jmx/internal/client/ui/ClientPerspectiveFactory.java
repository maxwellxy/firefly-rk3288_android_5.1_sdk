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
package org.eclipse.equinox.jmx.internal.client.ui;

import org.eclipse.ui.IPageLayout;
import org.eclipse.ui.IPerspectiveFactory;

/**
 * @since 1.0
 */
public class ClientPerspectiveFactory implements IPerspectiveFactory {

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IPerspectiveFactory#createInitialLayout(org.eclipse.ui.IPageLayout)
	 */
	public void createInitialLayout(IPageLayout layout) {
		layout.setEditorAreaVisible(false);

		layout.addView(ClientUI.VIEWID_CONTRIBUTIONS, IPageLayout.LEFT, 0.3f, layout.getEditorArea());
		layout.addView(ClientUI.VIEWID_MBEANINFO, IPageLayout.TOP, 0.75f, layout.getEditorArea());
		layout.addView(ClientUI.VIEWID_INVOCATION, IPageLayout.BOTTOM, 0.25f, layout.getEditorArea());
	}
}
