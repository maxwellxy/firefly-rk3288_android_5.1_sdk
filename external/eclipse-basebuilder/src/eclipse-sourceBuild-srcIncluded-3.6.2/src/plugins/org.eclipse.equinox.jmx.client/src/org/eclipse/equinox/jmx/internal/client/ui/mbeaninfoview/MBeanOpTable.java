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

import javax.management.MBeanOperationInfo;
import javax.management.MBeanParameterInfo;
import org.eclipse.equinox.jmx.common.ContributionProxy;
import org.eclipse.equinox.jmx.internal.client.ui.invocationView.InvocationView;
import org.eclipse.equinox.jmx.internal.client.ui.viewsupport.ViewUtil;
import org.eclipse.jface.viewers.*;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.*;

public class MBeanOpTable {

	protected class MBeanOpContentProvider implements IStructuredContentProvider {
		private MBeanOperationInfo[] fOps;

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
		 */
		public Object[] getElements(Object inputElement) {
			if (fOps == null)
				return new Object[0];
			return fOps;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
		 */
		public void dispose() {
			// nothing needs to be disposed
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
			fOps = (MBeanOperationInfo[]) newInput;
		}
	}

	protected class MBeanOpLabelProvider extends LabelProvider implements ITableLabelProvider {
		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
		 */
		public Image getColumnImage(Object element, int columnIndex) {
			return null;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
		 */
		public String getColumnText(Object element, int columnIndex) {
			if (!(element instanceof MBeanOperationInfo))
				return super.getText(element);

			MBeanOperationInfo info = (MBeanOperationInfo) element;
			switch (columnIndex) {
				case 0 :
					return ""; //$NON-NLS-1$
				case 1 :
					return info.getReturnType() != null ? info.getReturnType() : "void"; //$NON-NLS-1$
				case 2 :
					return info.getName();
				case 3 :
					MBeanParameterInfo[] params = info.getSignature();
					StringBuffer sb = new StringBuffer();
					for (int j = 0; j < params.length; j++) {
						String type = params[j].getType();
						if (j != 0)
							sb.append(", "); //$NON-NLS-1$
						sb.append(type);
						sb.append('(');
						sb.append(params[j].getName());
						sb.append(')');
					}
					return sb.toString();
			}
			return getText(element);
		}
	}

	protected class MBeanOpViewerFilter extends ViewerFilter {
		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		public boolean select(Viewer viewer, Object parentElement, Object element) {
			if (element instanceof MBeanOperationInfo) {
				MBeanOperationInfo info = (MBeanOperationInfo) element;
				return !info.getName().equals("getChildContributions") && !info.getName().equals("createProxy"); //$NON-NLS-1$ //$NON-NLS-2$
			}
			return false;
		}
	}

	protected class MBeanOpViewerSorter extends ViewerSorter {
		int fDirection, fIndex;

		protected MBeanOpViewerSorter(int direction, int index) {
			fDirection = direction == SWT.UP ? -1 : 1;
			fIndex = index;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
		 */
		public int compare(Viewer viewer, Object e1, Object e2) {
			if (e1 instanceof MBeanOperationInfo && e2 instanceof MBeanOperationInfo) {
				MBeanOperationInfo op1 = (MBeanOperationInfo) e1;
				MBeanOperationInfo op2 = (MBeanOperationInfo) e2;
				switch (fIndex) {
					case 1 :
						String a1 = op1.getReturnType();
						String a2 = op2.getReturnType();
						int p = a1.lastIndexOf('.');
						if (p != -1)
							a1 = a1.substring(p + 1);
						p = a2.lastIndexOf('.');
						if (p != -1)
							a2 = a2.substring(p + 1);
						return fDirection * a1.compareTo(a2);
					case 2 :
						return fDirection * op1.getName().compareTo(op2.getName());
					case 3 :
						MBeanParameterInfo[] info1 = op1.getSignature();
						MBeanParameterInfo[] info2 = op2.getSignature();
						if (info2.length == 0)
							return fDirection;
						if (info1.length == 0)
							return -fDirection;

						return fDirection * (info1[0].getType().compareTo(info2[0].getType()));
				}
			}
			return fDirection * super.compare(viewer, e1, e2);
		}
	}

	private TableViewer fViewer;

	public MBeanOpTable(Composite parent, final MBeanInfoViewPart beanView) {
		final Table opTable = beanView.getToolkit().createTable(parent, SWT.BORDER | SWT.SINGLE | SWT.FLAT | SWT.FULL_SELECTION | SWT.V_SCROLL | SWT.H_SCROLL);
		createColumns(opTable);
		opTable.setLayoutData(new GridData(GridData.FILL_BOTH));
		opTable.setLinesVisible(true);
		opTable.setHeaderVisible(true);
		opTable.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				if (e.item == null || e.item.getData() == null)
					return;
				InvocationView view = ViewUtil.getInvocationView();
				view.selectionChanged(beanView, new StructuredSelection(e.item.getData()));
			}
		});

		fViewer = new TableViewer(opTable);
		fViewer.setContentProvider(new MBeanOpContentProvider());
		fViewer.setLabelProvider(new MBeanOpLabelProvider());
		fViewer.addFilter(new MBeanOpViewerFilter());
	}

	private void createColumns(final Table opTable) {
		TableColumn blankCol = new TableColumn(opTable, SWT.NONE);
		blankCol.setText(""); //$NON-NLS-1$
		blankCol.setWidth(20);
		final TableColumn returnType = new TableColumn(opTable, SWT.NONE);
		returnType.setText(MBeanInfoViewMessages.MBeanOpTable_returnType);
		returnType.setWidth(100);
		final TableColumn opName = new TableColumn(opTable, SWT.NONE);
		opName.setText(MBeanInfoViewMessages.MBeanOpTable_name);
		opName.setWidth(150);
		final TableColumn params = new TableColumn(opTable, SWT.NONE);
		params.setText(MBeanInfoViewMessages.MBeanOpTable_params);
		params.setWidth(300);

		Listener sortListener = new Listener() {
			public void handleEvent(Event e) {
				// determine new sort column and direction
				TableColumn sortColumn = opTable.getSortColumn();
				TableColumn currentColumn = (TableColumn) e.widget;

				int dir = opTable.getSortDirection();
				if (sortColumn == currentColumn) {
					dir = dir == SWT.UP ? SWT.DOWN : SWT.UP;
				} else {
					opTable.setSortColumn(currentColumn);
					dir = SWT.UP;
				}
				int colIndex;
				if (currentColumn == returnType)
					colIndex = 1;
				else if (currentColumn == opName)
					colIndex = 2;
				else if (currentColumn == params)
					colIndex = 3;
				else
					return;

				// sort the data based on column and direction
				opTable.setSortDirection(dir);
				fViewer.setSorter(new MBeanOpViewerSorter(dir, colIndex));
			}
		};
		returnType.addListener(SWT.Selection, sortListener);
		opName.addListener(SWT.Selection, sortListener);
		params.addListener(SWT.Selection, sortListener);
		opTable.setSortColumn(opName);
		opTable.setSortDirection(SWT.UP);
	}

	protected void setInput(ContributionProxy input) {
		if (input == null || input.getMBeanInfo() == null)
			fViewer.setInput(null);
		else
			fViewer.setInput(input.getMBeanInfo().getOperations());
	}
}
