/*******************************************************************************
 * Copyright (c) 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     Red Hat, Inc. and IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.equinox.internal.initializer;

import java.io.*;
import java.net.URL;
import java.util.*;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.*;
import org.eclipse.osgi.util.NLS;
import org.osgi.framework.Bundle;

/**
 * <p>
 * This application extracts files to their "localURL". The file patterns that you
 * want to extract should be specified in a file in a properties format like this:
 * 
 * <pre>
 * .* = *.so,about.html
 * org.eclipse.team.core.cvs = *.html
 * </pre>
 * 
 * This application should be run through the generic initializer mechanism and 
 * not run directly.
 * </p>
 * 
 * <p>
 * To run this application, do something like:
 * </p>
 * 
 * <p>
 * <code>
 * java -cp startup.jar org.eclipse.core.launcher.Main -application org.eclipse.equinox.initializer.configInitializer [-justThisArchOSWS] -fileInitializer &lt;fileWithFilePatternsToExtract&gt;
 * </code>
 * </p>
 * 
 * <p>
 * <ul>
 * <li> 
 * Note: Only the files that are extracted will be printed to the console. If you
 * want to debug things, use the -consolelog option in
 * org.eclipse.core.launcher.Main
 * </li>
 * </ul>
 * </p>
 * 
 * @see Initializer
 * @see Platform#asLocalURL(URL)
 * @since 3.1
 */
public class FileInitializer implements IPlatformRunnable {

	private LinkedList extractedFiles;
	private boolean justThisArchOSWS;

	public Object run(Object args) throws Exception {
		Properties bundleAndFilePatterns = getBundleAndFileNamePatterns((String[]) args);
		// don't do anything if bundleAndFilePatterns is empty
		if (bundleAndFilePatterns == null || bundleAndFilePatterns.size() == 0) {
			return IPlatformRunnable.EXIT_OK;
		}
		justThisArchOSWS = getJustThisArchOSWS((String[]) args);
		extractedFiles = new LinkedList();
		Bundle[] installedBundles = Activator.getContext().getBundles();
		Set bundlePatterns = bundleAndFilePatterns.keySet();
		// foreach(bundle pattern)
		for (Iterator bundlePatternIter = bundlePatterns.iterator(); bundlePatternIter.hasNext();) {
			Object bundlePattern = bundlePatternIter.next();
			Object fileNamePattern = bundleAndFilePatterns.get(bundlePattern);
			// foreach(filename pattern)
			String[] fileNamePatterns = ((String) fileNamePattern).split(","); //$NON-NLS-1$
			for (int i = 0; i < fileNamePatterns.length; i++) {
				fileNamePatterns[i] = fileNamePatterns[i].trim();
			}
			// foreach(bundle)
			for (int i = 0; i < installedBundles.length; i++) {
				String bundleName = installedBundles[i].getSymbolicName();
				// if(bundle matches pattern) check fileNamePattern(s)
				if (Pattern.matches((String) bundlePattern, bundleName)) {
					for (int j = 0; j < fileNamePatterns.length; j++) {
						extractMatchingFilesFromBundle(fileNamePatterns[j], installedBundles[i]);
					}
				}
			}
		} // end for
		return IPlatformRunnable.EXIT_OK;
	}

	private void extractMatchingFilesFromBundle(String fileNamePattern, Bundle bundle) throws IOException {
		Enumeration e = bundle.findEntries("/", fileNamePattern, true); //$NON-NLS-1$

		while (e != null && e.hasMoreElements()) {
			URL fileURL = (URL) e.nextElement();
			if (justThisArchOSWS) {
				Path filePath = new Path(fileURL.getPath());

				String[] path = filePath.segments();
				// check to see if the file should be extracted to the current os/arch
				if (path.length == 4 && path[0].equals("os") && !(path[1].equals(Platform.getOS()) && path[2].equals(Platform.getOSArch()))) { //$NON-NLS-1$
					continue;
				}
				// check to see if the file is should be extracted to the current ws
				if (path.length == 3 && path[0].equals("ws") && !path[1].equals(Platform.getWS())) { //$NON-NLS-1$
					continue;
				}
			}

			// the call to Platform.asLocalURL(URL) does the actual extraction
			URL localURL = FileLocator.toFileURL(fileURL);
			if (localURL != null) {
				String localURLPath = localURL.getPath();

				// only print the path if it hasn't been printed yet
				int index = Collections.binarySearch(extractedFiles, localURLPath);
				if (index < 0) {
					extractedFiles.add(-(index + 1), localURLPath);
					System.out.println("FileInitializer: " + localURLPath); //$NON-NLS-1$
				}
			}
		}
	}

	private final String ARG_FILE = "-fileInitializer"; //$NON-NLS-1$

	private Properties getBundleAndFileNamePatterns(String[] argsArray) {

		for (int i = 0; i < argsArray.length; i++) {
			if (argsArray[i].equalsIgnoreCase(ARG_FILE)) {

				// the "file patterns" file was not specified
				if (argsArray.length < i + 2) {
					String msg = NLS.bind(Messages.fileInitializer_missingFileName, this.getClass().getName());
					IStatus status = new Status(IStatus.ERROR, Platform.PI_RUNTIME, Platform.PLUGIN_ERROR, msg, null);
					Activator.log(status);
					return null;
				}

				try {
					FileInputStream fin = new FileInputStream(argsArray[i + 1]);
					Properties bundleAndFileNameProperties = new Properties();
					bundleAndFileNameProperties.load(fin);
					fin.close();
					return bundleAndFileNameProperties;
				} catch (FileNotFoundException e1) {
					String msg = NLS.bind(Messages.fileInitializer_fileNotFound, this.getClass().getName(), argsArray[i + 1]);
					IStatus status = new Status(IStatus.ERROR, Platform.PI_RUNTIME, Platform.PLUGIN_ERROR, msg, null);
					Activator.log(status);
					return null;
				} catch (IOException e) {
					String msg = NLS.bind(Messages.fileInitializer_IOError, this.getClass().getName(), argsArray[i + 1]);
					IStatus status = new Status(IStatus.ERROR, Platform.PI_RUNTIME, Platform.PLUGIN_ERROR, msg, e);
					Activator.log(status);
					return null;
				}
			}
		}

		// ARG_FILE wasn't found
		return null;
	}

	private final String ARG_JUSTTHISARCHOSWS = "-justThisArchOSWS"; //$NON-NLS-1$

	private boolean getJustThisArchOSWS(String[] argsArray) {
		for (int i = 0; i < argsArray.length; i++) {
			if (argsArray[i].equalsIgnoreCase(ARG_JUSTTHISARCHOSWS)) {
				return true;
			}
		}
		return false;
	}
}
