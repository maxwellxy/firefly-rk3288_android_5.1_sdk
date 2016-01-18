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
package org.eclipse.equinox.jmx.internal.vm;

import java.lang.management.CompilationMXBean;
import java.net.URL;
import java.util.HashSet;
import java.util.Set;
import javax.management.*;
import org.eclipse.equinox.jmx.common.util.MBeanInfoWrapper;
import org.eclipse.equinox.jmx.server.Contribution;

public class CompilationContribution extends Contribution {

	private static final String ICON_PATH = "icons/compilation.gif"; //$NON-NLS-1$
	private MBeanInfo mbeanInfo;

	public CompilationContribution(CompilationMXBean delegate) {
		super(delegate);
		mbeanInfo = MBeanInfoWrapper.createMBeanInfo(delegate.getClass(), delegate.toString(), new MBeanAttributeInfo[0], new MBeanNotificationInfo[0]);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getName()
	 */
	protected String getName() {
		return VMStatsMessages.cmp_title;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getProperties()
	 */
	protected Set<String> getProperties() {
		Set<String> result = new HashSet<String>();
		CompilationMXBean mbean = (CompilationMXBean) contributionDelegate;
		result.add(VMStatsMessages.cmp_jitname + ": " + mbean.getName()); //$NON-NLS-1$
		result.add(VMStatsMessages.cmp_tottime + ": " + mbean.getTotalCompilationTime()); //$NON-NLS-1$
		return result;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return mbeanInfo;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttribute(java.lang.String)
	 */
	public Object getAttribute(String attribute) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute attribute) {
		// do nothing
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
	 */
	public AttributeList getAttributes(String[] attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
	 */
	public AttributeList setAttributes(AttributeList attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return Activator.getImageLocation(ICON_PATH);
	}
}
