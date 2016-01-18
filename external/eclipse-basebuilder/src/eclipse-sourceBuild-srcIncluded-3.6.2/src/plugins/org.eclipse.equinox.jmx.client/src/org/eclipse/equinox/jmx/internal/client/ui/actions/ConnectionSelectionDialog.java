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
package org.eclipse.equinox.jmx.internal.client.ui.actions;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Iterator;
import java.util.Map;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXServiceURL;
import org.eclipse.equinox.jmx.client.IJMXConnectorProvider;
import org.eclipse.equinox.jmx.common.JMXConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.*;
import org.eclipse.ui.dialogs.SelectionDialog;

/**
 * @since 1.0
 */
public class ConnectionSelectionDialog extends SelectionDialog {

	// sizing constants
	private final static int SIZING_SELECTION_WIDGET_HEIGHT = 250;
	private final static int SIZING_SELECTION_WIDGET_WIDTH = 300;

	private JMXConnector connector;

	private Text hostText, portText;
	private Combo transport;
	private Shell parent;
	private final Map transports;

	public ConnectionSelectionDialog(Shell parentShell, Map transports) {
		super(parentShell);
		this.transports = transports;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.dialogs.SelectionDialog#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText(ActionMessages.connection_selection_dialog_title);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(final Composite parent) {
		this.parent = parent.getShell();
		Composite composite = (Composite) super.createDialogArea(parent);
		Font font = parent.getFont();
		composite.setFont(font);

		createMessageArea(composite);

		GridData data = new GridData(GridData.FILL_BOTH);
		data.heightHint = SIZING_SELECTION_WIDGET_HEIGHT;
		data.widthHint = SIZING_SELECTION_WIDGET_WIDTH;

		Composite fieldComposite = new Composite(composite, SWT.NULL);
		fieldComposite.setLayout(new GridLayout(5, false));
		// 1 host label
		Label label = new Label(fieldComposite, SWT.CENTER);
		label.setText(ActionMessages.host);
		// 2 host text entry
		hostText = new Text(fieldComposite, SWT.BORDER);
		hostText.setText("localhost");
		hostText.selectAll();
		data = new GridData();
		data.widthHint = convertWidthInCharsToPixels(25);
		hostText.setLayoutData(data);
		// 3 port label
		label = new Label(fieldComposite, SWT.CENTER);
		label.setText(ActionMessages.port);
		// 4 port text entry
		portText = new Text(fieldComposite, SWT.BORDER);
		portText.setTextLimit(5);
		portText.setText(JMXConstants.DEFAULT_PORT);
		data = new GridData();
		data.widthHint = convertWidthInCharsToPixels(6);
		portText.setLayoutData(data);
		// 5 protocol selection combo box
		transport = new Combo(fieldComposite, SWT.DROP_DOWN | SWT.READ_ONLY);
		// add transport provided as extensions to supported list
		Iterator iter = transports.keySet().iterator();
		while (iter.hasNext()) {
			transport.add((String) iter.next());
		}
		transport.select(0);
		return composite;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed() {
		if (hostText.getText().equals("")) { //$NON-NLS-1$
			MessageDialog.openError(parent.getShell(), ActionMessages.error_message, ActionMessages.invalid_host);
			return;
		}
		try {
			InetAddress.getByName(hostText.getText());
		} catch (UnknownHostException e) {
			MessageDialog.openError(parent.getShell(), ActionMessages.error_message, ActionMessages.invalid_host);
			return;
		}
		if (portText.getText().equals("")) { //$NON-NLS-1$
			MessageDialog.openError(parent.getShell(), ActionMessages.error_message, ActionMessages.invalid_port);
			return;
		}
		int port;
		try {
			port = Integer.parseInt(portText.getText());
			if (port < 1 || port > 0xffff) {
				throw new NumberFormatException();
			}
		} catch (NumberFormatException e) {
			MessageDialog.openError(parent.getShell(), ActionMessages.error_message, ActionMessages.invalid_port);
			return;
		}
		try {
			IJMXConnectorProvider ctorp = (IJMXConnectorProvider) transports.get(transport.getText());
			JMXServiceURL url = ctorp.getJMXServiceURL(hostText.getText(), port, transport.getText(), JMXConstants.DEFAULT_DOMAIN);
			connector = ctorp.newJMXConnector(url, null);
		} catch (Exception e) {
			MessageDialog.openError(null, ActionMessages.error_message, e.getMessage());
			super.cancelPressed();
			return;
		}
		super.okPressed();
	}

	public JMXConnector getJMXConnector() {
		return connector;
	}
}