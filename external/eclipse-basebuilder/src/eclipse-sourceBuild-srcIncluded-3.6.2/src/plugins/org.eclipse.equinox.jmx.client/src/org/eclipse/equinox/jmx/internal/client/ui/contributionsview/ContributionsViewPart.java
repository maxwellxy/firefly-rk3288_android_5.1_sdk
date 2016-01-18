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
import javax.management.*;
import org.eclipse.equinox.jmx.internal.client.Activator;
import org.eclipse.equinox.jmx.internal.client.MBeanServerProxy;
import org.eclipse.equinox.jmx.internal.client.ui.actions.ActionMessages;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.ViewerSorter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.part.ViewPart;

/**
 * @since 1.0
 */
public class ContributionsViewPart extends ViewPart {

	protected TreeViewer viewer;
	protected ContributionContentProvider contentProvider;
	protected ContributionLabelProvider labelProvider;
	protected MBeanServerProxy serverProxy;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createPartControl(Composite parent) {
		viewer = new TreeViewer(parent, SWT.SINGLE | SWT.V_SCROLL | SWT.H_SCROLL);
		viewer.setUseHashlookup(true);
		viewer.setSorter(new ViewerSorter());
		setProviders();
		getSite().setSelectionProvider(viewer);
	}

	protected ContributionContentProvider createContentProvider() {
		return new ContributionContentProvider(viewer, null);
	}

	protected ContributionLabelProvider createLabelProvider() {
		return new ContributionLabelProvider();
	}

	protected void setProviders() {
		contentProvider = createContentProvider();
		viewer.setContentProvider(contentProvider);
		labelProvider = createLabelProvider();
		viewer.setLabelProvider(labelProvider);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	public void setFocus() {
		viewer.getTree().setFocus();
	}

	public void setMBeanServerProxy(MBeanServerProxy proxy) {
		this.serverProxy = proxy;
		try {
			contentProvider.setServerContributionProxy(proxy);
			updateViewer();
		} catch (Exception e) {
			Activator.logError(e);
		}
	}

	public MBeanServerProxy getMBeanServerProxy() {
		return serverProxy;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	public void dispose() {
		super.dispose();
	}

	public void connectionClosed(String message) {
		MessageDialog.openInformation(viewer.getControl().getShell(), ActionMessages.info_message, message);
	}

	private void updateViewer() throws InstanceNotFoundException, MBeanException, ReflectionException, IOException, NotCompliantMBeanException {
		if (serverProxy == null) {
			viewer.setSelection(null);
			viewer.setInput(null);
			return;
		}
		viewer.setInput(serverProxy.getRootContribution());
	}
}