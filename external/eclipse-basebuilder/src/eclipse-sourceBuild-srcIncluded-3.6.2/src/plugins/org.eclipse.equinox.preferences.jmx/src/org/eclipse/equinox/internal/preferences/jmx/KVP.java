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
package org.eclipse.equinox.internal.preferences.jmx;

import org.osgi.service.prefs.Preferences;

/**
 * This class represents a key/value pair in the preferences.
 * 
 * @since 1.0
 */
public class KVP {

	private Preferences node;
	private String key;
	private String value;

	/*
	 * Constructor. Make a new key/value pair with the given values.
	 */
	public KVP(Preferences node, String key, String value) {
		super();
		this.node = node;
		this.key = key;
		this.value = value;
	}

	/*
	 * Return the key.
	 */
	public String getKey() {
		return this.key;
	}

	/*
	 * Return the value.
	 */
	public String getValue() {
		return this.value;
	}

	/*
	 * Return the preference node which is associated with this key/value pair.
	 */
	public Preferences getNode() {
		return this.node;
	}
}
