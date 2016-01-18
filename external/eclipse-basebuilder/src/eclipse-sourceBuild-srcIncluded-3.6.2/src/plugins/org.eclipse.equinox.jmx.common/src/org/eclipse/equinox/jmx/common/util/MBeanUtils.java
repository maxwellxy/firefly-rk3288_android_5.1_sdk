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

import java.math.BigDecimal;
import java.math.BigInteger;
import javax.management.MBeanParameterInfo;

public class MBeanUtils {
	public static Object[] getParameters(String[] textParams, MBeanParameterInfo[] params) throws ClassNotFoundException {
		if (textParams == null || params == null) {
			return null;
		}
		Object[] ret = new Object[textParams.length];
		for (int i = 0; i < ret.length && i < params.length; i++) {
			MBeanParameterInfo param = params[i];
			String texti = textParams[i];
			if (texti.length() == 0) {
				ret[i] = null;
			} else if (param.getType().equals("byte")) { //$NON-NLS-1$
				ret[i] = new Byte(texti);
			} else if (param.getType().equals("short")) {//$NON-NLS-1$
				ret[i] = new Short(texti);
			} else if (param.getType().equals("java.lang.Short")) {//$NON-NLS-1$
				ret[i] = new Short(texti);
			} else if (param.getType().equals("int")) {//$NON-NLS-1$
				ret[i] = new Integer(texti);
			} else if (param.getType().equals("java.lang.Integer")) {//$NON-NLS-1$
				ret[i] = new Integer(texti);
			} else if (param.getType().equals("long")) {//$NON-NLS-1$
				ret[i] = new Long(texti);
			} else if (param.getType().equals("java.lang.Long")) {//$NON-NLS-1$
				ret[i] = new Long(texti);
			} else if (param.getType().equals("float")) {//$NON-NLS-1$
				ret[i] = new Float(texti);
			} else if (param.getType().equals("java.lang.Float")) {//$NON-NLS-1$
				ret[i] = new Float(texti);
			} else if (param.getType().equals("double")) {//$NON-NLS-1$
				ret[i] = new Double(texti);
			} else if (param.getType().equals("java.lang.Double")) {//$NON-NLS-1$
				ret[i] = new Double(texti);
			} else if (param.getType().equals("char")) {//$NON-NLS-1$
				ret[i] = new Character(texti.charAt(0));
			} else if (param.getType().equals("boolean")) {//$NON-NLS-1$
				ret[i] = new Boolean(texti);
			} else if (MBeanUtils.class.getClassLoader().loadClass("java.lang.Number").isAssignableFrom(MBeanUtils.class.getClassLoader().loadClass(param.getType()))) {//$NON-NLS-1$
				ret[i] = createNumber(texti);
			} else {
				ret[i] = texti;
			}
		}
		return ret;
	}

	public static Number createNumber(String val) {
		try {
			return new Byte(val);
		} catch (NumberFormatException e) {
		}
		try {
			return new BigDecimal(val);
		} catch (NumberFormatException e) {
		}
		try {
			return new BigInteger(val);
		} catch (NumberFormatException e) {
		}
		return null;
	}
}
