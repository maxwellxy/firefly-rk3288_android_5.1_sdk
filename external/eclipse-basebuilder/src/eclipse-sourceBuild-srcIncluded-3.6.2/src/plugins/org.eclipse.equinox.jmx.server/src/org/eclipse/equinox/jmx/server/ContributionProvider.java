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
package org.eclipse.equinox.jmx.server;

import java.util.*;
import javax.management.MalformedObjectNameException;
import org.eclipse.equinox.jmx.internal.server.ServerExtensionManager;
import org.eclipse.equinox.jmx.internal.server.ServerExtensionManager.ContributionExtensionDefinition;

/**
 * Required base class for all classes extending the <code>org.eclipse.equinox.jmx.server.contribution</code> extension point.
 * <p>
 * A <code>ContributionProvider</code> has the ability to create concrete <code>Contribution</code>s
 * from a given <code>Object</code> type.  After creation, the <code>ContributionProvider</code> is added
 * to an internal registry of known providers.  Any <code>Object</code>s wishing to be contributed must
 * first locate a supported provider. 
 * 
 * @see Contribution
 * @since 1.0
 */
public abstract class ContributionProvider extends Contribution {

	/**
	 * Default constructor for all derived <code>ContributionProviders</code>.  Adds
	 * this provider to the list of available contribution providers.
	 * 
	 * @param contributionDelegate
	 */
	public ContributionProvider() {
		super();
	}

	/**
	 * Returns <code>true</code> if this provider supports creating a <code>Contribution</code>
	 * from the provided object.
	 * 
	 * @param obj The object to contribute.
	 * @return <code>true</code> if the object is supported by this provider and 
	 * 	<code>false</code> otherwise
	 */
	protected abstract boolean contributesType(Object obj);

	/**
	 * Returns <code>true</code> if this provider supports the {@link #createProvider(Object)} operation
	 * for the supplied object.  Providing a type typically implies that the provider
	 * bases its decision on which objects to return as children using logic associated
	 * with the object provided.
	 * 
	 * @param obj The object to provide for.
	 * @return <code>true</code> if the object is provided by this provider and
	 * 	<code>false</code> otherwise
	 */
	protected abstract boolean providesType(Object obj);

	/**
	 * Allocate a <code>ContributionProvider</code> that is specialized 
	 * with the provided object.
	 * 
	 * @param obj The object to associate the allocated provider with.
	 * @return The newly allocated specialized provider.
	 */
	protected abstract ContributionProvider createProvider(Object obj);

	/**
	 * Allocate a <code>Contribution</code> from the object provided. If the 
	 * implementing <code>ContributionProvider</code> does not support the 
	 * type of object provided, <code>null</code> is returned.
	 * 
	 * @param obj The object to to contribute.
	 * @return A <code>Contribution</code> object or null if the object type is not supported.
	 * @throws MalformedObjectNameException
	 */
	protected abstract Contribution createContribution(Object obj) throws MalformedObjectNameException;

	/**
	 * Convenience method for classes to query the provider registry to determine
	 * if a <code>ContributionProvider</code> exists for the object provided.  The first
	 * provider found that supports contributing the object type is returned.
	 * 
	 * @param obj The object to located a provider for.
	 * @return A <code>ContributionProvider</code> which supports the provided object, null if no such provider exists.
	 */
	public static ContributionProvider getProvider(Object obj) {
		Collection contribDefns = ServerExtensionManager.getInstance().getContributionExtensionDefinitions();
		Iterator iter = contribDefns.iterator();
		while (iter.hasNext()) {
			ContributionProvider provider = ((ContributionExtensionDefinition) iter.next()).getContributionProvider();
			if (provider.contributesType(obj)) {
				return provider;
			}
		}
		return null;
	}

	/**
	 * Returns a list of the <code>ContributionProvider</code>s that supports the {@link #providesType(Object)}
	 * method for the supplied object.
	 * 
	 * @param obj The object for which to locate a <code>ContributionProvider</code>.
	 * @return The list of identified providers.
	 */
	public static ContributionProvider[] getExtendingProviders(Object obj) {
		Collection contribDefns = ServerExtensionManager.getInstance().getContributionExtensionDefinitions();
		Iterator iter = contribDefns.iterator();
		List providers = null;
		while (iter.hasNext()) {
			ContributionExtensionDefinition defn = (ContributionExtensionDefinition) iter.next();
			// iterate over definitions list of supported class types and for assignable match
			ContributionProvider provider = defn.getContributionProvider();
			if (provider.providesType(obj)) {
				if (providers == null) {
					providers = new ArrayList(2);
				}
				providers.add(provider.createProvider(obj));
			}
		}
		ContributionProvider[] ret = null;
		if (providers != null) {
			ret = (ContributionProvider[]) providers.toArray(new ContributionProvider[providers.size()]);
		}
		return ret;
	}
}
