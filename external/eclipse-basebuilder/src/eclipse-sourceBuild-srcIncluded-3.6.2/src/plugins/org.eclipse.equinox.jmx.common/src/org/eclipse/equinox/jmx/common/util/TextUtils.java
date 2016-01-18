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

public class TextUtils {

	static final String QUOTE = "&quot;"; //$NON-NLS-1$
	static final String LESS_THAN = "&lt;"; //$NON-NLS-1$
	static final String GREATER_THAN = "&gt;"; //$NON-NLS-1$
	static final String AMPERSAND = "&amp;"; //$NON-NLS-1$

	public static String escapeForMarkup(String strin) {
		return escapeForMarkup(new StringBuffer(strin));
	}

	/**
	 * Return a valid UTF8 xml encoded string from the input string buffer 
	 * provided.
	 * 
	 * @param buffer The <code>StringBuffer</code> to encode.
	 * @return The encoded string.
	 */
	public static String escapeForMarkup(StringBuffer buffer) {
		if (buffer == null) {
			return null;
		}
		final StringBuffer result = new StringBuffer();
		char c;
		int len = buffer.length();
		for (int i = 0; i < len; i++) {
			c = buffer.charAt(i);
			switch (c) {
				case '"' :
					result.append(QUOTE);
					break;
				case '<' :
					result.append(LESS_THAN);
					break;
				case '>' :
					result.append(GREATER_THAN);
					break;
				case '&' :
					result.append(AMPERSAND);
					break;
				case '\r' :
					result.append(c);
					break;
				case '\n' :
					result.append(c);
					break;
				case '\t' :
					result.append(c);
					break;
				default :
					if (c > 0x20 && c < 0x7f) {
						result.append(c);
					}
			}
		}
		return result.toString();
	}
}