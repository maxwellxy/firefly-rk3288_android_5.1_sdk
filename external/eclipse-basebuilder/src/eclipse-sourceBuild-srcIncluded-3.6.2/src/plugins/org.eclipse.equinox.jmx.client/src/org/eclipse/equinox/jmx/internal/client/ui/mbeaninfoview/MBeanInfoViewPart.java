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
package org.eclipse.equinox.jmx.internal.client.ui.mbeaninfoview;

import java.util.Iterator;
import javax.management.*;
import org.eclipse.equinox.jmx.common.ContributionProxy;
import org.eclipse.equinox.jmx.common.IContributionStateChangeListener;
import org.eclipse.equinox.jmx.internal.client.ui.ClientUI;
import org.eclipse.equinox.jmx.internal.client.ui.invocationView.InvocationView;
import org.eclipse.equinox.jmx.internal.client.ui.viewsupport.ViewUtil;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.osgi.util.NLS;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.*;
import org.eclipse.ui.*;
import org.eclipse.ui.forms.ManagedForm;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.part.ViewPart;

/**
 * @since 1.0
 */
public class MBeanInfoViewPart extends ViewPart implements ISelectionListener, IContributionStateChangeListener {

	protected ContributionProxy fSelectedContribution;
	private ManagedForm fManagedForm;
	private MBeanOpTable fOpTable;
	private Font fBFont;
	private Text fPropertiesText;
	private Section fContributionSection;
	private Section fPropertiesSection;
	private Section fOperationsSection;
	private Label fC_title;
	private Label fC_desc;
	private Text fAttributesText;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createPartControl(Composite parent) {
		FontData fd[] = parent.getFont().getFontData();
		fBFont = new Font(parent.getDisplay(), fd[0].getName(), fd[0].getHeight(), SWT.BOLD);
		fManagedForm = new ManagedForm(parent);
		fManagedForm.getForm().setText(MBeanInfoViewMessages.MBeanInfoViewPart_8);

		Composite body = fManagedForm.getForm().getBody();
		body.setLayout(new GridLayout());
		body.setLayoutData(new GridData(GridData.FILL_BOTH));

		createContributionTitleArea(body);
		createContributionPropertiesArea(body);
		createOperationsArea(body);

		getSite().getPage().addSelectionListener(this);
	}

	protected FormToolkit getToolkit() {
		return fManagedForm.getToolkit();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	public void dispose() {
		fBFont.dispose();
		fManagedForm.dispose();
		getSite().getPage().removePostSelectionListener(this);
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	public void setFocus() {
		fManagedForm.setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISelectionListener#selectionChanged(org.eclipse.ui.IWorkbenchPart, org.eclipse.jface.viewers.ISelection)
	 */
	public void selectionChanged(IWorkbenchPart part, ISelection selection) {
		if (!(selection instanceof IStructuredSelection))
			return;

		Object obj = ((IStructuredSelection) selection).getFirstElement();

		String id = part.getSite().getId();
		if (id.equals(ClientUI.VIEWID_CONTRIBUTIONS)) {
			if (obj instanceof ContributionProxy) {
				ContributionProxy contrib = (ContributionProxy) obj;
				if (contrib == fSelectedContribution) {
					return;
				} else if (fSelectedContribution != null) {
					// remove listener from previously selected proxy
					fSelectedContribution.removeStateChangeListener(this);
				}
				// update the currently selected contribution to the one to be displayed, if null
				// the controls displayed are still disposed, this is to reflect a removed contribution
				fSelectedContribution = contrib;
				updateContributionTitle(true);
				updateProperties(true);
				updateOperations(true);
				if (fSelectedContribution == null)
					return;
				fSelectedContribution.addStateChangeListener(this);
			} else if (obj == null) {
				contributionRemoved(fSelectedContribution = null);
			}
		}
	}

	private void createContributionTitleArea(Composite parent) {
		Composite comp = ViewUtil.createSection(MBeanInfoViewMessages.MBeanInfoViewPart_0, null, fManagedForm, parent, 2, false, true);
		FormToolkit toolkit = fManagedForm.getToolkit();
		fContributionSection = (Section) comp.getParent();

		toolkit.createLabel(comp, MBeanInfoViewMessages.contribution_name);
		fC_title = toolkit.createLabel(comp, ""); //$NON-NLS-1$
		fC_title.setFont(fBFont);
		fC_title.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		toolkit.createLabel(comp, MBeanInfoViewMessages.mbean_description);
		fC_desc = toolkit.createLabel(comp, ""); //$NON-NLS-1$
		fC_desc.setFont(fBFont);
		fC_desc.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
	}

	private void updateContributionTitle(boolean forceExpand) {
		boolean enabled = fSelectedContribution != null;

		fContributionSection.setEnabled(enabled);
		if (!fContributionSection.isEnabled() || forceExpand)
			fContributionSection.setExpanded(enabled);
		if (!enabled)
			return;

		String name = fSelectedContribution.getName();
		fC_title.setText(name);
		fManagedForm.getForm().setText(NLS.bind(MBeanInfoViewMessages.MBeanInfoViewPart_11, name));

		MBeanInfo info = fSelectedContribution.getMBeanInfo();
		if (info.getDescription() != null)
			fC_desc.setText(info.getDescription());
		else
			fC_desc.setText(""); //$NON-NLS-1$
	}

	private void createContributionPropertiesArea(Composite parent) {
		FormToolkit toolkit = fManagedForm.getToolkit();
		Composite comp = ViewUtil.createSection(MBeanInfoViewMessages.MBeanInfoViewPart_1, null, fManagedForm, parent, 1, false, true);
		fPropertiesSection = (Section) comp.getParent();
		fPropertiesText = toolkit.createText(comp, "", SWT.MULTI | SWT.H_SCROLL | SWT.READ_ONLY | SWT.BORDER); //$NON-NLS-1$
		GridData gd = new GridData(GridData.FILL_HORIZONTAL | GridData.GRAB_HORIZONTAL);
		gd.widthHint = 100;
		fPropertiesText.setLayoutData(gd);
	}

	private void updateProperties(boolean forceExpand) {
		boolean enabled = fSelectedContribution != null && fSelectedContribution.getContributionProperties() != null && fSelectedContribution.getContributionProperties().iterator().hasNext();

		fPropertiesSection.setEnabled(enabled);
		if (!fPropertiesSection.isEnabled() || forceExpand)
			fPropertiesSection.setExpanded(enabled);
		if (!enabled)
			return;

		Iterator iter = fSelectedContribution.getContributionProperties().iterator();
		StringBuffer sb = new StringBuffer();
		while (iter.hasNext())
			sb.append(iter.next().toString() + (iter.hasNext() ? "\n" : "")); //$NON-NLS-1$ //$NON-NLS-2$
		fPropertiesText.setText(sb.toString());
	}

	private void createOperationsArea(Composite parent) {
		Composite comp = ViewUtil.createSection(MBeanInfoViewMessages.MBeanInfoViewPart_6, MBeanInfoViewMessages.MBeanInfoViewPart_7, fManagedForm, parent, 1, true, true);
		fOperationsSection = (Section) comp.getParent();

		fOpTable = new MBeanOpTable(comp, this);

		FormToolkit toolkit = fManagedForm.getToolkit();
		toolkit.createLabel(comp, MBeanInfoViewMessages.description);
		fAttributesText = toolkit.createText(comp, "", SWT.MULTI | SWT.H_SCROLL | SWT.READ_ONLY | SWT.BORDER); //$NON-NLS-1$
		GridData gd = new GridData(GridData.FILL_HORIZONTAL | GridData.GRAB_HORIZONTAL);
		gd.widthHint = 100;
		gd.heightHint = 30;
		fAttributesText.setLayoutData(gd);
	}

	private void updateOperations(boolean forceExpand) {
		boolean enabled = fSelectedContribution != null && fSelectedContribution.getMBeanInfo() != null && (fSelectedContribution.getMBeanInfo().getOperations().length > 2 || fSelectedContribution.getMBeanInfo().getAttributes().length > 0);

		fOperationsSection.setEnabled(enabled);
		if (!fOperationsSection.isEnabled() || forceExpand)
			fOperationsSection.setExpanded(enabled);
		if (!enabled)
			return;

		MBeanInfo info = fSelectedContribution.getMBeanInfo();

		MBeanOperationInfo[] ops = info.getOperations();
		if (ops.length > 0)
			fOpTable.setInput(fSelectedContribution);

		StringBuffer sb = new StringBuffer();
		MBeanAttributeInfo[] attrs = info.getAttributes();
		for (int i = 0; i < attrs.length; i++) {
			if (i != 0)
				sb.append('\n');
			sb.append(attrs[i].getName());
			sb.append(':');
			sb.append(' ');
			sb.append(attrs[i].getDescription());
		}
	}

	/**
	 * This is called when the mbean contribution resource associated with the proxy
	 * has been removed, it proceeds to invalidate the view.
	 * 
	 * @param contribution The contribution proxy that represents the contribution resource removed.
	 */
	public void contributionRemoved(ContributionProxy contribution) {
		fContributionSection.setEnabled(false);
		fContributionSection.setExpanded(false);
		fPropertiesSection.setEnabled(false);
		fPropertiesSection.setExpanded(false);
		fOperationsSection.setEnabled(false);
		fOperationsSection.setExpanded(false);
		InvocationView iv = (InvocationView) PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().findView(ClientUI.VIEWID_INVOCATION);
		if (iv != null) {
			iv.clear();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.common.IContributionStateChangeListener#stateChanged(org.eclipse.equinox.jmx.common.ContributionProxy)
	 */
	public void stateChanged(final ContributionProxy source) {
		fManagedForm.getForm().getDisplay().asyncExec(new Runnable() {
			public void run() {
				fSelectedContribution = source;
				// only updating properties on state change
				updateProperties(false);
			}
		});
	}
}
