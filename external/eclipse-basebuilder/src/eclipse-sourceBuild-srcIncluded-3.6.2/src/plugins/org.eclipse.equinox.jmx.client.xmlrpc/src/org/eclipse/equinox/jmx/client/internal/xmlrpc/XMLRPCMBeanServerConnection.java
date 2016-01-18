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
package org.eclipse.equinox.jmx.client.internal.xmlrpc;

import java.io.IOException;
import java.util.Set;
import javax.management.*;
import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.eclipse.equinox.jmx.client.remote.RemoteMBeanConnection;
import org.eclipse.equinox.jmx.client.remote.RemoteNotificationHandler;
import org.eclipse.equinox.jmx.common.NamedNotification;

/**
 * A <code>XMLRPCMBeanServerConnection</code> provides an implementation of 
 * <code>MBeanServerConnection</code> that utilizes xml-rpc as a transport
 * provider. 
 */
public class XMLRPCMBeanServerConnection implements MBeanServerConnection, RemoteMBeanConnection {

	private static final String CREATE_MBEAN = "createMBean"; //$NON-NLS-1$
	private XmlRpcClient clientConnection;
	private RemoteNotificationHandler notificationHandler;

	public XMLRPCMBeanServerConnection(XmlRpcClient clientConnection) {
		this.clientConnection = clientConnection;
		this.notificationHandler = new RemoteNotificationHandler(this);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#createMBean(java.lang.String, javax.management.ObjectName)
	 */
	public ObjectInstance createMBean(String className, ObjectName name) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException, IOException {
		try {
			return (ObjectInstance) clientConnection.execute(CREATE_MBEAN, new Object[] {className, name});
		} catch (XmlRpcException e) {
			throw new MBeanException(e);
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#createMBean(java.lang.String, javax.management.ObjectName, javax.management.ObjectName)
	 */
	public ObjectInstance createMBean(String className, ObjectName name, ObjectName loaderName) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException, InstanceNotFoundException, IOException {
		try {
			return (ObjectInstance) clientConnection.execute(CREATE_MBEAN, new Object[] {className, name, loaderName});
		} catch (XmlRpcException e) {
			throw new MBeanException(e);
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#createMBean(java.lang.String, javax.management.ObjectName, java.lang.Object[], java.lang.String[])
	 */
	public ObjectInstance createMBean(String className, ObjectName name, Object[] params, String[] signature) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException, IOException {
		try {
			return (ObjectInstance) clientConnection.execute(CREATE_MBEAN, new Object[] {className, name, params, signature});
		} catch (XmlRpcException e) {
			throw new MBeanException(e);
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#createMBean(java.lang.String, javax.management.ObjectName, javax.management.ObjectName, java.lang.Object[], java.lang.String[])
	 */
	public ObjectInstance createMBean(String className, ObjectName name, ObjectName loaderName, Object[] params, String[] signature) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException, InstanceNotFoundException, IOException {
		try {
			return (ObjectInstance) clientConnection.execute(CREATE_MBEAN, new Object[] {className, name, loaderName, params, signature});
		} catch (XmlRpcException e) {
			throw new MBeanException(e);
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#unregisterMBean(javax.management.ObjectName)
	 */
	public void unregisterMBean(ObjectName name) throws InstanceNotFoundException, MBeanRegistrationException, IOException {
		try {
			clientConnection.execute("unregisterMBean", new Object[] {name});
		} catch (XmlRpcException e) {
			throw new MBeanRegistrationException(e);
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getObjectInstance(javax.management.ObjectName)
	 */
	public ObjectInstance getObjectInstance(ObjectName name) throws InstanceNotFoundException, IOException {
		try {
			return (ObjectInstance) clientConnection.execute("getObjectInstance", new Object[] {name});
		} catch (XmlRpcException e) {
			throw new IOException(e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#queryMBeans(javax.management.ObjectName, javax.management.QueryExp)
	 */
	public Set queryMBeans(ObjectName name, QueryExp query) throws IOException {
		try {
			return (Set) clientConnection.execute("queryMBeans", new Object[] {name, query});
		} catch (XmlRpcException e) {
			throw new IOException(e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#queryNames(javax.management.ObjectName, javax.management.QueryExp)
	 */
	public Set queryNames(ObjectName name, QueryExp query) throws IOException {
		try {
			return (Set) clientConnection.execute("queryNames", new Object[] {name, query});
		} catch (XmlRpcException e) {
			throw new IOException(e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#isRegistered(javax.management.ObjectName)
	 */
	public boolean isRegistered(ObjectName name) throws IOException {
		return false;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getMBeanCount()
	 */
	public Integer getMBeanCount() throws IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getAttribute(javax.management.ObjectName, java.lang.String)
	 */
	public Object getAttribute(ObjectName name, String attribute) throws MBeanException, AttributeNotFoundException, InstanceNotFoundException, ReflectionException, IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getAttributes(javax.management.ObjectName, java.lang.String[])
	 */
	public AttributeList getAttributes(ObjectName name, String[] attributes) throws InstanceNotFoundException, ReflectionException, IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#setAttribute(javax.management.ObjectName, javax.management.Attribute)
	 */
	public void setAttribute(ObjectName name, Attribute attribute) throws InstanceNotFoundException, AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException, IOException {
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#setAttributes(javax.management.ObjectName, javax.management.AttributeList)
	 */
	public AttributeList setAttributes(ObjectName name, AttributeList attributes) throws InstanceNotFoundException, ReflectionException, IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#invoke(javax.management.ObjectName, java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	public Object invoke(ObjectName name, String operationName, Object[] params, String[] signature) throws InstanceNotFoundException, MBeanException, ReflectionException, IOException {
		try {
			// split class name from object name to prepend with method name
			return clientConnection.execute(name + "|" + operationName, params == null ? new Object[0] : params);
		} catch (Exception e) {
			throw new IOException(e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getDefaultDomain()
	 */
	public String getDefaultDomain() throws IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getDomains()
	 */
	public String[] getDomains() throws IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#addNotificationListener(javax.management.ObjectName, javax.management.NotificationListener, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void addNotificationListener(ObjectName name, NotificationListener listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException, IOException {
		notificationHandler.addNotificationListener(name, listener, filter, handback);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#addNotificationListener(javax.management.ObjectName, javax.management.ObjectName, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void addNotificationListener(ObjectName name, ObjectName listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException, IOException {
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#removeNotificationListener(javax.management.ObjectName, javax.management.ObjectName)
	 */
	public void removeNotificationListener(ObjectName name, ObjectName listener) throws InstanceNotFoundException, ListenerNotFoundException, IOException {
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#removeNotificationListener(javax.management.ObjectName, javax.management.ObjectName, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void removeNotificationListener(ObjectName name, ObjectName listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException, ListenerNotFoundException, IOException {
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#removeNotificationListener(javax.management.ObjectName, javax.management.NotificationListener)
	 */
	public void removeNotificationListener(ObjectName name, NotificationListener listener) throws InstanceNotFoundException, ListenerNotFoundException, IOException {
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#removeNotificationListener(javax.management.ObjectName, javax.management.NotificationListener, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void removeNotificationListener(ObjectName name, NotificationListener listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException, ListenerNotFoundException, IOException {
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#getMBeanInfo(javax.management.ObjectName)
	 */
	public MBeanInfo getMBeanInfo(ObjectName name) throws InstanceNotFoundException, IntrospectionException, ReflectionException, IOException {
		return null;
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServerConnection#isInstanceOf(javax.management.ObjectName, java.lang.String)
	 */
	public boolean isInstanceOf(ObjectName name, String className) throws InstanceNotFoundException, IOException {
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.equinox.jmx.client.remote.RemoteMBeanConnection#retrieveNotifications(long)
	 */
	public NamedNotification[] retrieveNotifications(long startId) {
		try {
			Object obj = clientConnection.execute("retrieveNotifications", new Object[] {new Long(startId)});
			if (obj instanceof Object[]) {
				Object[] objs = (Object[]) obj;
				NamedNotification[] result = new NamedNotification[objs.length];
				for (int i = 0; i < objs.length; i++) {
					if (objs[i] instanceof NamedNotification) {
						result[i] = (NamedNotification) objs[i];
					}
				}
				return result;

			}
		} catch (XmlRpcException e) {
		}
		return null;
	}
}
