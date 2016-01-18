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
package org.eclipse.equinox.jmx.internal.server;

import java.util.*;
import org.eclipse.core.runtime.*;
import org.eclipse.equinox.jmx.server.ContributionProvider;

public class ServerExtensionManager extends Observable implements IRegistryChangeListener {

	public static class ContributionExtensionDefinition {

		private static final String ATTRIBUTE_CLASS = "class"; //$NON-NLS-1$
		private static final String ATTRIBUTE_IS_ROOT = "isroot"; //$NON-NLS-1$
		private static final String ELEMENT_CONTRIBUTION = "contribution"; //$NON-NLS-1$
		private static final String ELEMENT_EXTENDS_CLASS = "extendsClass"; //$NON-NLS-1$

		private String providerClassName;
		private boolean isRootProvider;
		private Set extendsClasses;
		private ContributionProvider provider;

		ContributionExtensionDefinition(final IConfigurationElement contribElem) {
			final String elementName = contribElem.getName();
			// check for require mbean element and associated class attribute
			if (elementName.equalsIgnoreCase(ELEMENT_CONTRIBUTION) && (providerClassName = contribElem.getAttribute(ATTRIBUTE_CLASS)) != null) {
				try {
					Object obj = contribElem.createExecutableExtension(ATTRIBUTE_CLASS);
					if (obj instanceof ContributionProvider) {
						provider = (ContributionProvider) obj;
						// check if provider is a root provider and set flag appropriately
						String attrIsRoot = null;
						if ((attrIsRoot = contribElem.getAttribute(ATTRIBUTE_IS_ROOT)) != null) {
							isRootProvider = attrIsRoot.equals("true"); //$NON-NLS-1$
						}
						// cache any types which this provider extends
						IConfigurationElement[] extendsElems = contribElem.getChildren(ELEMENT_EXTENDS_CLASS);
						extendsClasses = new TreeSet();
						for (int j = 0; j < extendsElems.length; j++) {
							IConfigurationElement extendsElem = extendsElems[j];
							extendsClasses.add(extendsElem.getAttribute(ATTRIBUTE_CLASS));
						}
					}
				} catch (CoreException e) {
					Activator.logError(e);
				}
			} else {
				throw new IllegalArgumentException();
			}
		}

		public String getProviderClassName() {
			return providerClassName;
		}

		public boolean isRootProvider() {
			return isRootProvider;
		}

		public ContributionProvider getContributionProvider() {
			return provider;
		}

		public Set getExtendsClasses() {
			return extendsClasses;
		}
	}

	private static Map providers;
	private static ServerExtensionManager instance;

	private ServerExtensionManager() {
		providers = new Hashtable();
		initExtensions();
	}

	public static ServerExtensionManager getInstance() {
		if (instance == null) {
			instance = new ServerExtensionManager();
		}
		return instance;
	}

	public Collection getContributionExtensionDefinitions() {
		return providers.values();
	}

	public ContributionExtensionDefinition getContributionExtensionDefinition(String providerClassName) {
		return (ContributionExtensionDefinition) providers.get(providerClassName);
	}

	private void initExtensions() {
		registerContributions();
		RegistryFactory.getRegistry().addRegistryChangeListener(this);
	}

	private void registerContributions() {
		IExtensionPoint point = RegistryFactory.getRegistry().getExtensionPoint(Activator.PI_NAMESPACE, Activator.PT_CONTRIBUTION);
		IExtension[] types = point.getExtensions();
		for (int i = 0; i < types.length; i++) {
			registerContribution(types[i]);
		}
	}

	private void registerContribution(IExtension ext) {
		IConfigurationElement[] configElems = ext.getConfigurationElements();
		for (int j = 0; j < configElems.length; j++) {
			ContributionExtensionDefinition defn = new ContributionExtensionDefinition(configElems[j]);
			if (defn.getContributionProvider() == null) {
				// extension was removed or an error loading the class occurred
				// remove from map if exists
				defn = (ContributionExtensionDefinition) providers.remove(defn.getProviderClassName());
			} else {
				providers.put(defn.getProviderClassName(), defn);
			}
			setChanged();
			notifyObservers(defn);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IRegistryChangeListener#registryChanged(org.eclipse.core.runtime.IRegistryChangeEvent)
	 */
	public void registryChanged(IRegistryChangeEvent event) {
		IExtensionDelta[] deltas = event.getExtensionDeltas(Activator.PI_NAMESPACE, Activator.PT_CONTRIBUTION);
		for (int i = 0; i < deltas.length; i++) {
			IExtensionDelta delta = deltas[i];
			registerContribution(delta.getExtension());
		}
	}
}
