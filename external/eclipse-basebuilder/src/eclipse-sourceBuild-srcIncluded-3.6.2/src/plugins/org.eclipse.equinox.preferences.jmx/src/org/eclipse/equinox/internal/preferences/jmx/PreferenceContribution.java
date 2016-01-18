/*******************************************************************************
 * Copyright (c) 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials 
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 * 	Jeff Mesnil - Bug 151266 - [monitoring] Browsing our server in jconsole is really unfriendly
 *     IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.equinox.internal.preferences.jmx;

import java.net.URL;
import java.util.*;
import javax.management.*;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.equinox.jmx.common.JMXConstants;
import org.eclipse.equinox.jmx.server.Contribution;
import org.osgi.service.prefs.BackingStoreException;
import org.osgi.service.prefs.Preferences;

/**
 * @since 1.0
 */
public class PreferenceContribution extends Contribution {

	// path to the icon
	private static final String IMAGE_PATH = "icons/node.gif"; //$NON-NLS-1$
	private static final String EMPTY_STRING = ""; //$NON-NLS-1$

	private static final String METHOD_ADD_CHILD = "addChild"; //$NON-NLS-1$
	private static final String METHOD_PUT = "put"; //$NON-NLS-1$
	private static final String METHOD_REMOVE_NODE = "removeNode"; //$NON-NLS-1$

	private static final String PARM_NAME = "name"; //$NON-NLS-1$
	private static final String PARM_KEY = "key"; //$NON-NLS-1$
	private static final String PARM_VALUE = "value"; //$NON-NLS-1$

	private static final String TYPE_STRING = "java.lang.String"; //$NON-NLS-1$

	/*
	 * Constructor for the class. Create a contribution for the given preference node.
	 */
	public PreferenceContribution(Preferences delegate) {
		super(delegate.absolutePath());
	}

    /* (non-Javadoc)
     * @see org.eclipse.equinox.jmx.server.Contribution#getObjectName()
     */
    protected ObjectName getObjectName() {
        try {
            return new ObjectName(JMXConstants.DEFAULT_DOMAIN + ":type=Preferences,name=" + contributionDelegate); //$NON-NLS-1$
        } catch (Exception e) {
            return super.getObjectName();
        }
    }

	/*
	 * Return the delegate associated with this contribution.
	 */
	private Preferences getDeletgate() {
		return Activator.getPreferenceService().getRootNode().node((String) contributionDelegate);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getName()
	 */
	protected String getName() {
		return getDeletgate().name();
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getChildren()
	 */
	protected Object[] getChildren() {
		String[] children = new String[0];
		try {
			children = getDeletgate().childrenNames();
		} catch (BackingStoreException e) {
			// ignore and try and add the key/value pairs
		}

		String[] keys = new String[0];
		try {
			keys = getDeletgate().keys();
		} catch (BackingStoreException e) {
			// ignore
		}
		Object[] result = new Object[children.length + keys.length];
		for (int i = 0; i < children.length; i++)
			result[i] = getDeletgate().node(children[i]);
		for (int i = children.length; i < keys.length; i++)
			result[i] = new KVP(getDeletgate(), keys[i], getDeletgate().get(keys[i], EMPTY_STRING));
		return result;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getProperties()
	 */
	protected Set getProperties() {
		return null;
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#getMBeanInfo(java.lang.Object)
	 */
	protected MBeanInfo getMBeanInfo(Object delegate) {
		return new MBeanInfo(getClass().getName(), null, null, null, createOperations(), null);
	}

	/*
	 * Return an array of operations which are applicable to this contribution.
	 */
	private MBeanOperationInfo[] createOperations() {
		List result = new ArrayList();
		result.add(createAddChildOperation());
		result.add(createRemoveNodeOperation());
		result.add(createPutOperation());
		return (MBeanOperationInfo[]) result.toArray(new MBeanOperationInfo[result.size()]);
	}

	/*
	 * Create and return an MBean operation that adds a key/value pair
	 * to the current preference node.
	 */
	private MBeanOperationInfo createPutOperation() {
		MBeanParameterInfo[] parms = new MBeanParameterInfo[] { //
		new MBeanParameterInfo(PARM_KEY, TYPE_STRING, Messages.parm_key_desc), //
				new MBeanParameterInfo(PARM_VALUE, TYPE_STRING, Messages.parm_value_desc) //
		};
		return new MBeanOperationInfo(METHOD_PUT, Messages.operation_put, parms, Void.TYPE.getName(), 0);
	}

	/*
	 * Create and return an MBean operation that will remove the current preference node.
	 */
	private MBeanOperationInfo createRemoveNodeOperation() {
		return new MBeanOperationInfo(METHOD_REMOVE_NODE, Messages.operation_removeNode, null, Void.TYPE.getName(), 0);
	}

	/*
	 * Create and return an MBean operation that will add a child to the current preference node.
	 */
	private MBeanOperationInfo createAddChildOperation() {
		MBeanParameterInfo[] parms = new MBeanParameterInfo[] { //
		new MBeanParameterInfo(PARM_NAME, TYPE_STRING, Messages.parm_childName_desc) //
		};
		return new MBeanOperationInfo(METHOD_ADD_CHILD, Messages.operation_addChild, parms, Void.TYPE.getName(), 0);
	}

	/* (non-Javadoc)
	 * @see com.jmx.server.contrib.Contribution#invokeOperation(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	protected Object invokeOperation(String operationName, Object[] args, String[] argTypes) {
		if (METHOD_ADD_CHILD.equals(operationName)) {
			handleAddChildOperation(args);
		} else if (METHOD_REMOVE_NODE.equals(operationName)) {
			handleRemoveNodeOperation();
		} else if (METHOD_PUT.equals(operationName)) {
			handlePutOperation(args);
		}
		return null;
	}

	/*
	 * Add the given key/value pair to the current preference node.
	 */
	private void handlePutOperation(Object[] args) {
		if (args == null || args.length != 2)
			return;
		if (!(args[0] instanceof String) || !(args[1] instanceof String))
			return;
		getDeletgate().put((String) args[0], (String) args[1]);
	}

	/*
	 * Add a child with the given name to the current preference node.
	 */
	private void handleAddChildOperation(Object[] args) {
		if (args == null || args.length != 1)
			return;
		if (!(args[0] instanceof String))
			return;
		getDeletgate().node((String) args[0]);
	}

	/*
	 * Remove this preference node.
	 */
	private void handleRemoveNodeOperation() {
		try {
			getDeletgate().removeNode();
		} catch (BackingStoreException e) {
			Activator.log(e.getMessage(), e);
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttribute(java.lang.String)
	 */
	public Object getAttribute(String attribute) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttribute(javax.management.Attribute)
	 */
	public void setAttribute(Attribute attribute) {
		// do nothing
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#getAttributes(java.lang.String[])
	 */
	public AttributeList getAttributes(String[] attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.DynamicMBean#setAttributes(javax.management.AttributeList)
	 */
	public AttributeList setAttributes(AttributeList attributes) {
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.server.Contribution#getImageLocation()
	 */
	protected URL getImageLocation() {
		return FileLocator.find(Activator.getContext().getBundle(), new Path(IMAGE_PATH), null);
	}
}
