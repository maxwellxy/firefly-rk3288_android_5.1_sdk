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

import org.eclipse.equinox.jmx.internal.client.ui.ClientUI;
import org.eclipse.equinox.jmx.internal.client.ui.contributionsview.ContributionsViewPart;
import org.eclipse.equinox.jmx.internal.client.ui.invocationView.InvocationView;
import org.eclipse.equinox.jmx.internal.client.ui.mbeaninfoview.MBeanInfoViewPart;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.SectionPart;
import org.eclipse.ui.forms.widgets.*;

public class ViewUtil {

	public static Composite createSection(String sectionTitle, String description, IManagedForm form, Composite parent, int cols, boolean fillBoth, boolean twistie) {
		FormToolkit toolkit = form.getToolkit();
		int flags = ExpandableComposite.TITLE_BAR;
		if (twistie)
			flags |= ExpandableComposite.TWISTIE;
		if (description != null)
			flags |= Section.DESCRIPTION;
		SectionPart sectionPart = new SectionPart(parent, toolkit, flags);
		sectionPart.initialize(form);
		Section section = sectionPart.getSection();
		section.setText(sectionTitle);
		if (description != null)
			section.setDescription(description);
		flags = fillBoth ? GridData.FILL_BOTH : GridData.FILL_HORIZONTAL;
		section.setLayoutData(new GridData(flags));
		Composite composite = toolkit.createComposite(section);
		composite.setLayout(new GridLayout(cols, false));
		composite.setLayoutData(new GridData(flags));
		section.setClient(composite);
		if (twistie) {
			section.setEnabled(false);
			section.setExpanded(false);
		}
		return composite;
	}

	public static ContributionsViewPart getContributionsView() {
		return (ContributionsViewPart) getView(ClientUI.VIEWID_CONTRIBUTIONS);
	}

	public static InvocationView getInvocationView() {
		return (InvocationView) getView(ClientUI.VIEWID_INVOCATION);
	}

	public static MBeanInfoViewPart getBeanInfoView() {
		return (MBeanInfoViewPart) getView(ClientUI.VIEWID_MBEANINFO);
	}

	private static IViewPart getView(String viewId) {
		return PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().findView(viewId);
	}
}
