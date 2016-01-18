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

import java.lang.management.*;
import java.net.URL;
import java.util.HashSet;
import java.util.Set;
import javax.management.*;
import org.eclipse.equinox.jmx.server.Contribution;
import org.eclipse.equinox.jmx.server.ContributionProvider;

public class VMStatsProvider extends ContributionProvider {

	static final String BUNDLE_NAME = "org.eclipse.equinox.jmx.vm"; //$NON-NLS-1$
	private static final String ICON_PATH = "icons/vmstats.gif"; //$NON-NLS-1$

	public VMStatsProvider() {
		super();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#contributesType(java.lang.Object)
	 */
	protected boolean contributesType(Object obj) {
		return obj instanceof ClassLoadingMXBean || obj instanceof CompilationMXBean || obj instanceof MemoryMXBean || obj instanceof OperatingSystemMXBean || obj instanceof RuntimeMXBean;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#providesType(java.lang.Object)
	 */
	protected boolean providesType(Object obj) {
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#createProvider(java.lang.Object)
	 */
	protected ContributionProvider createProvider(Object obj) {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.ContributionProvider#createContribution(java.lang.Object)
	 */
	protected Contribution createContribution(Object obj) {
		if (contributesType(obj)) {
			if (obj instanceof ClassLoadingMXBean) {
				return new ClassLoadingContribution(ManagementFactory.getClassLoadingMXBean());
			} else if (obj instanceof CompilationMXBean) {
				return new CompilationContribution(ManagementFactory.getCompilationMXBean());
			} else if (obj instanceof MemoryMXBean) {
				return new MemoryContribution(ManagementFactory.getMemoryMXBean());
			} else if (obj instanceof OperatingSystemMXBean) {
				return new OperatingSystemContribution(ManagementFactory.getOperatingSystemMXBean());
			} else if (obj instanceof RuntimeMXBean) {
				return new RuntimeContribution(ManagementFactory.getRuntimeMXBean());
			}
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getName()
	 */
	protected String getName() {
		return VMStatsMessages.title;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		return new Object[] {ManagementFactory.getClassLoadingMXBean(), ManagementFactory.getCompilationMXBean(), ManagementFactory.getMemoryMXBean(), ManagementFactory.getOperatingSystemMXBean(), ManagementFactory.getRuntimeMXBean()};
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getProperties()
	 */
	protected Set<String> getProperties() {
		return new HashSet<String>();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return new MBeanInfo(getClass().getName(), null, null, null, null, null);
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
		//
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
