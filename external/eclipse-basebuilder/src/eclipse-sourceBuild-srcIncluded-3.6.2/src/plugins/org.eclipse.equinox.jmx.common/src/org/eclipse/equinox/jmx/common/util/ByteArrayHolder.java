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
package org.eclipse.equinox.jmx.common.util;

import java.io.Serializable;

/**
 * Holder for <code>bytes</code>s.
 */
public final class ByteArrayHolder implements Serializable {

	private static final long serialVersionUID = 7212161648975273292L;

	/**
	 * The <code>byte</code>s held by this holder.
	 */
	public byte[] value;

	/**
	 * Default constructor.
	 */
	public ByteArrayHolder() {
		this(null);
	}

	/**
	 * Allocate a new <code>ByteArrayHolder</code>.
	 * 
	 * @param value The <code>byte<code>s to hold, <code>null</code> if none.
	 */
	public ByteArrayHolder(byte[] value) {
		this.value = value;
	}
}
