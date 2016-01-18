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
package org.eclipse.osgi.jmx.internal;

import java.io.*;
import java.net.URL;
import java.util.*;
import java.util.jar.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.eclipse.core.runtime.*;
import org.eclipse.osgi.service.pluginconversion.PluginConversionException;
import org.eclipse.osgi.service.pluginconversion.PluginConverter;
import org.eclipse.osgi.service.resolver.*;
import org.osgi.framework.*;
import org.osgi.util.tracker.ServiceTracker;

public class BundleUtils {

	public static Set computeDependencies(Bundle bundle, BundleContext bundleContext) throws PluginConversionException, CoreException, IOException {
		Map disjointSets = new HashMap();

		Bundle[] installedBundles = bundleContext.getBundles();
		for (int i = 0; i < installedBundles.length; i++) {
			BundleDescription desc = getBundleDescription(installedBundles[i], bundleContext);
			if (desc == null) {
				continue;
			}
			// add bundle to map with initial empty dependency list
			List descDeps = new ArrayList();
			disjointSets.put(desc.getName(), descDeps);
			BundleSpecification[] specs = desc.getRequiredBundles();
			for (int j = 0; j < specs.length; j++) {
				descDeps.add(specs[j].getName());
			}
		}
		Set bundleDeps = new HashSet();
		BundleDescription rootDesc = getBundleDescription(bundle, bundleContext);
		BundleSpecification[] rootSpecs = rootDesc.getRequiredBundles();
		for (int i = 0; i < rootSpecs.length; i++) {
			bundleDeps.add(rootSpecs[i].getName());
			List specDeps = (List) disjointSets.get(rootSpecs[i].getName());
			if (specDeps == null) {
				continue;
			}
			mergeDependencies(bundleDeps, specDeps, disjointSets);
		}
		return bundleDeps;
	}

	private static void mergeDependencies(Set bundleDeps, List deps, Map disjointSets) {
		if (deps != null) {
			// merge dependencies
			bundleDeps.addAll(deps);
			Iterator iter = deps.iterator();
			while (iter.hasNext()) {
				String name = (String) iter.next();
				mergeDependencies(bundleDeps, (List) disjointSets.get(name), disjointSets);
			}
		}
	}

	public static BundleDescription getBundleDescription(Bundle bundle, BundleContext bundleContext) throws PluginConversionException, CoreException, IOException {
		String bundleRootLoc = getBundleRoot(bundle);
		//FIXME hack for loading jar files
		int protoIdx = bundleRootLoc.indexOf("file:"); //$NON-NLS-1$
		bundleRootLoc = bundleRootLoc.substring(protoIdx == -1 ? 0 : protoIdx + 5);
		if (bundleRootLoc.endsWith("jar!/")) { //$NON-NLS-1$
			bundleRootLoc = bundleRootLoc.substring(0, bundleRootLoc.length() - 2);
		}
		File bundleLocation = new File(bundleRootLoc);
		Dictionary manifest = loadManifest(bundleLocation);
		long bundleId = bundle.getBundleId();
		boolean hasBundleStructure = manifest != null && manifest.get(Constants.BUNDLE_SYMBOLICNAME) != null;
		if (!hasBundleStructure) {
			if (!bundleLocation.isFile() && !new File(bundleLocation, "plugin.xml").exists() //$NON-NLS-1$
					&& !new File(bundleLocation, "fragment.xml").exists()) { //$NON-NLS-1$
				return null;
			}
			PluginConverter converter = acquirePluginConverter(bundleContext);
			manifest = converter.convertManifest(bundleLocation, false, null, false, null);
			if (manifest == null || Constants.BUNDLE_SYMBOLICNAME == null) {
				return null;
			}
		}
		try {
			return getBundleDescription(manifest, bundleLocation, bundleId);
		} catch (BundleException e) {
			e.printStackTrace();
		}
		return null;
	}

	public static String getBundleRoot(Bundle bundle) throws IOException {
		URL url = FileLocator.find(bundle, new Path("/"), null); //$NON-NLS-1$
		URL resolvedUrl = FileLocator.resolve(url);
		return resolvedUrl.getPath();
	}

	public static BundleDescription getBundleDescription(Dictionary manifest, File bundleLocation, long bundleId) throws BundleException {
		BundleContext context = Activator.getBundleContext();
		ServiceReference platformAdminReference = context.getServiceReference(PlatformAdmin.class.getName());
		if (platformAdminReference == null)
			return null;
		PlatformAdmin admin = (PlatformAdmin) context.getService(platformAdminReference);
		StateObjectFactory stateObjectFactory = admin.getFactory();

		State state = stateObjectFactory.createState(false);
		BundleDescription descriptor = stateObjectFactory.createBundleDescription(state, manifest, bundleLocation.getAbsolutePath(), bundleId);
		context.ungetService(platformAdminReference);
		return descriptor;
	}

	public static Dictionary loadManifest(File bundleLocation) throws IOException {
		ZipFile jarFile = null;
		InputStream manifestStream = null;
		try {
			String extension = new Path(bundleLocation.getName()).getFileExtension();
			bundleLocation = new File(bundleLocation.getAbsolutePath());
			if (extension != null && (extension.equals("jar") || extension.equals("jar!")) && bundleLocation.isFile()) { //$NON-NLS-1$ //$NON-NLS-2$
				jarFile = new ZipFile(bundleLocation, ZipFile.OPEN_READ);
				ZipEntry manifestEntry = jarFile.getEntry(JarFile.MANIFEST_NAME);
				if (manifestEntry != null) {
					manifestStream = jarFile.getInputStream(manifestEntry);
				}
			} else {
				File file = new File(bundleLocation, JarFile.MANIFEST_NAME);
				if (file.exists())
					manifestStream = new FileInputStream(file);
			}
		} catch (IOException e) {
		}
		if (manifestStream == null)
			return null;
		try {
			Manifest m = new Manifest(manifestStream);
			return manifestToProperties(m.getMainAttributes());
		} finally {
			try {
				manifestStream.close();
			} catch (IOException e1) {
			}
			try {
				if (jarFile != null)
					jarFile.close();
			} catch (IOException e2) {
			}
		}
	}

	public static Properties manifestToProperties(Attributes d) {
		Iterator iter = d.keySet().iterator();
		Properties result = new Properties();
		while (iter.hasNext()) {
			Attributes.Name key = (Attributes.Name) iter.next();
			result.put(key.toString(), d.get(key));
		}
		return result;
	}

	public static PluginConverter acquirePluginConverter(BundleContext bundleContext) {
		ServiceTracker tracker = new ServiceTracker(bundleContext, PluginConverter.class.getName(), null);
		tracker.open();
		PluginConverter converter = (PluginConverter) tracker.getService();
		tracker.close();
		return converter;
	}
}
