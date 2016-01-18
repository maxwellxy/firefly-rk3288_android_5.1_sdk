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

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import javax.management.*;

/**
 * Utility class for creating <code>MBeanInfo</code> objects
 * from a specified class.
 */
public class MBeanInfoWrapper {

	public static MBeanInfo createMBeanInfo(Class cls, String description, MBeanAttributeInfo[] attributes, MBeanNotificationInfo[] notifications) {
		return new MBeanInfo(cls.getName(), description, attributes, createMBeanConstructorInfoAry(cls), createMBeanOperationInfoAry(cls), notifications);
	}

	public static MBeanConstructorInfo[] createMBeanConstructorInfoAry(Class cls) {
		MBeanConstructorInfo[] ret = new MBeanConstructorInfo[cls.getConstructors().length];
		Constructor[] constructors = cls.getConstructors();
		for (int i = 0; i < ret.length; i++) {
			Constructor c = constructors[i];
			ret[i] = createMBeanConstructorInfo(c);
		}
		return ret;
	}

	public static MBeanConstructorInfo createMBeanConstructorInfo(Constructor c) {
		return new MBeanConstructorInfo("", c); //$NON-NLS-1$ //TODO request description
	}

	public static MBeanOperationInfo[] createMBeanOperationInfoAry(Class cls) {
		MBeanOperationInfo[] ret = new MBeanOperationInfo[cls.getMethods().length];
		Method[] methods = cls.getMethods();
		for (int i = 0; i < ret.length; i++) {
			Method m = methods[i];
			ret[i] = createMBeanOperationInfo(m);
		}
		return ret;
	}

	public static MBeanOperationInfo createMBeanOperationInfo(Method method) {
		return new MBeanOperationInfo("", method); //$NON-NLS-1$ //TODO request description
	}
}
