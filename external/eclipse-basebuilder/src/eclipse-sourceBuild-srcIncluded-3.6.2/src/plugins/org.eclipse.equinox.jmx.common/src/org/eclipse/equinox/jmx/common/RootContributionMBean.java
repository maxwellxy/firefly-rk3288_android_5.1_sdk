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
package org.eclipse.equinox.jmx.common;

public interface RootContributionMBean {

	public static final String OP_GET_ROOT_CONTRIBUTIONS = "queryRootContributions";//$NON-NLS-1$
	public static final String OP_GET_ROOT_CONTRIBUTION = "queryRootContribution";//$NON-NLS-1$

	/**
	 * Get the list of root <code>ContributionProxy</code>s.
	 * 
	 * @return The list of root contributions.
	 */
	public ContributionProxy[] queryRootContributions();

	public RootContribution queryRootContribution();
}