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
package org.eclipse.equinox.jmx.internal.client.ui.invocationView;

import javax.management.MBeanOperationInfo;
import javax.management.MBeanParameterInfo;
import org.eclipse.equinox.jmx.common.ContributionProxy;
import org.eclipse.equinox.jmx.common.util.MBeanUtils;
import org.eclipse.equinox.jmx.internal.client.Activator;
import org.eclipse.equinox.jmx.internal.client.MBeanServerProxy;
import org.eclipse.equinox.jmx.internal.client.ui.ClientUI;
import org.eclipse.equinox.jmx.internal.client.ui.contributionsview.ContributionsViewPart;
import org.eclipse.equinox.jmx.internal.client.ui.mbeaninfoview.MBeanInfoViewMessages;
import org.eclipse.equinox.jmx.internal.client.ui.viewsupport.ViewUtil;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.*;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.forms.ManagedForm;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.part.ViewPart;

public class InvocationView extends ViewPart implements ISelectionListener {

	private Composite fInvocationComposite;
	private ManagedForm fManagedForm;
	private MBeanOperationInfo fSelectedOperation;
	private Font fItalicFont;
	protected ContributionProxy fSelectedContribution;
	private Composite parentComp;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createPartControl(Composite parent) {
		parentComp = parent;
		FontData fd[] = parent.getFont().getFontData();
		fItalicFont = new Font(parent.getDisplay(), fd[0].getName(), fd[0].getHeight(), SWT.ITALIC);
		getSite().getPage().addSelectionListener(this);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	public void dispose() {
		fItalicFont.dispose();
		if (fManagedForm != null)
			fManagedForm.dispose();
		getSite().getPage().removePostSelectionListener(this);
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	public void setFocus() {
		if (fManagedForm != null)
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
		if (id.equals(ClientUI.VIEWID_MBEANINFO) && obj instanceof MBeanOperationInfo) {
			MBeanOperationInfo contrib = (MBeanOperationInfo) obj;
			if (contrib == fSelectedOperation) {
				return;
			}
			// update the currently selected contribution to the one to be displayed, if null
			// the controls displayed are still disposed, this is to reflect a removed contribution
			fSelectedOperation = contrib;
			drawInvocationDetails(contrib);
		} else if (id.equals(ClientUI.VIEWID_CONTRIBUTIONS) && obj instanceof ContributionProxy) {
			fSelectedContribution = (ContributionProxy) obj;
			clear();
		}
	}

	public void clear() {
		drawInvocationDetails(null);
	}

	protected void drawInvocationDetails(MBeanOperationInfo opInfo) {
		if (parentComp != null && !parentComp.isDisposed()) {
			// remove any controls created from prior selections
			Control[] childs = parentComp.getChildren();
			if (childs.length > 0) {
				for (int i = 0; i < childs.length; i++) {
					childs[i].dispose();
				}
			}
		}
		if (opInfo == null) {
			return;
		}
		if (fManagedForm != null) {
			fManagedForm.dispose();
		}
		fManagedForm = new ManagedForm(parentComp);
		Composite body = fManagedForm.getForm().getBody();
		body.setLayout(new GridLayout());
		body.setLayoutData(new GridData(GridData.FILL_BOTH));

		fInvocationComposite = ViewUtil.createSection(MBeanInfoViewMessages.InvocationView_0, null, fManagedForm, body, 1, true, false);

		FormToolkit toolkit = fManagedForm.getToolkit();
		String desc = opInfo.getDescription();
		if (desc != null && desc.length() > 0) {
			Composite c = toolkit.createComposite(fInvocationComposite, SWT.NONE);
			c.setLayout(new GridLayout());
			toolkit.createLabel(c, desc);
		}
		// composite for method signature [ return type | name | Composite(1..n parameters) | invoke button ] 
		Composite c = toolkit.createComposite(fInvocationComposite, SWT.NONE);
		c.setLayout(new GridLayout(4, false));
		// return type
		toolkit.createLabel(c, opInfo.getReturnType() != null ? opInfo.getReturnType() : "void"); //$NON-NLS-1$
		// method name
		toolkit.createLabel(c, opInfo.getName() + ' ').setFont(fItalicFont);
		// parameters
		final MBeanParameterInfo[] params = opInfo.getSignature();
		Text[] textParams = null;
		if (params.length > 0) {
			Composite paramsComposite = toolkit.createComposite(c, SWT.NONE);
			paramsComposite.setLayout(new GridLayout(params.length + 1 /* button */, false));
			textParams = new Text[params.length];
			for (int j = 0; j < params.length; j++) {
				MBeanParameterInfo param = params[j];
				textParams[j] = new Text(paramsComposite, SWT.SINGLE | SWT.BORDER);
				textParams[j].setText(param.getType() + "(" + param.getName() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
				textParams[j].setLayoutData(new GridData(GridData.GRAB_HORIZONTAL | GridData.HORIZONTAL_ALIGN_FILL));
			}
			paramsComposite.pack();
		}
		new InvokeOperationButton(opInfo.getName(), textParams, params, c, SWT.PUSH);
		fInvocationComposite.pack();
		parentComp.layout();
	}

	private class InvokeOperationButton extends SelectionAdapter {

		private String fMethodName;
		private Text[] fTextParams;
		private MBeanParameterInfo[] fParamTypes;
		private Button fButton;

		public InvokeOperationButton(String methodName, Text[] textParams, MBeanParameterInfo[] paramTypes, Composite parent, int style) {
			fMethodName = methodName;
			fTextParams = textParams;
			fParamTypes = paramTypes;
			fButton = fManagedForm.getToolkit().createButton(parent, MBeanInfoViewMessages.button_invoke, style);
			fButton.addSelectionListener(this);
		}

		/* (non-Javadoc)
		 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
		 */
		public void widgetSelected(SelectionEvent event) {
			try {
				Object[] paramList = null;
				if (fTextParams != null) {
					String[] strs = new String[fTextParams.length];
					for (int i = 0; i < strs.length; i++) {
						strs[i] = fTextParams[i].getText();
					}
					paramList = MBeanUtils.getParameters(strs, fParamTypes);
				}
				// invoke operation
				ContributionsViewPart contribsViewPart = ViewUtil.getContributionsView();
				if (contribsViewPart != null) {
					MBeanServerProxy serverProxy = contribsViewPart.getMBeanServerProxy();
					Object result;
					if (paramList != null) {
						String[] paramSig = new String[fParamTypes.length];
						for (int i = 0; i < paramSig.length; i++) {
							paramSig[i] = fParamTypes[i].getType();
						}
						result = serverProxy.invokeContributionOperation(fSelectedContribution, fMethodName, paramList, paramSig);
					} else {
						result = serverProxy.invokeContributionOperation(fSelectedContribution, fMethodName, new Object[0], new String[0]);
					}
					if (result != null) {
						String message = result.toString();
						MessageDialog.openInformation(fManagedForm.getForm().getShell(), MBeanInfoViewMessages.invoke_result, message);
					}
				}
			} catch (Exception e) {
				Activator.logError(e);
				MessageDialog.openError(null, MBeanInfoViewMessages.error, e.getMessage());
			}
		}
	}

}
