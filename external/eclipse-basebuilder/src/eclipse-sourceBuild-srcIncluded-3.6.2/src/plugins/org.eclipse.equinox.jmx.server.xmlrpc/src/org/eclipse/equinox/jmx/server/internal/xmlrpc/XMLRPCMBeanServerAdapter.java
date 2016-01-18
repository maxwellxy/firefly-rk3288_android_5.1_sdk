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
package org.eclipse.equinox.jmx.server.internal.xmlrpc;

import java.io.IOException;
import java.io.ObjectInputStream;
import java.lang.reflect.Method;
import java.util.*;
import javax.management.*;
import javax.management.loading.ClassLoaderRepository;
import javax.management.remote.MBeanServerForwarder;
import javax.servlet.*;
import org.apache.xmlrpc.*;
import org.apache.xmlrpc.server.XmlRpcHandlerMapping;
import org.apache.xmlrpc.server.XmlRpcNoSuchHandlerException;
import org.apache.xmlrpc.webserver.XmlRpcServlet;
import org.eclipse.equinox.jmx.common.NamedNotification;
import org.eclipse.equinox.jmx.common.util.RingBuffer;
import org.mortbay.http.*;
import org.mortbay.jetty.servlet.ServletHandler;
import org.mortbay.jetty.servlet.ServletHolder;

public class XMLRPCMBeanServerAdapter implements MBeanServerForwarder, NotificationListener {

	private static final int NOTIFICATIONS_BUFFER_SIZE = 100;
	private static final String INTERNAL_CONTEXT_CLASSLOADER = "internal.ContextClassLoader"; //$NON-NLS-1$
	private static final String INTERNAL_MAPPING = "internal.Mapping"; //$NON-NLS-1$

	final Map notificationBroadcasters = new HashMap();
	private final RingBuffer notificationsBuffer = new RingBuffer(NOTIFICATIONS_BUFFER_SIZE);
	private final HttpServer webServer;
	static XmlRpcHandlerMappingImpl mapping;
	private MBeanServer mbs;
	private boolean started;

	/**
	 * XML-RPC Web server adapter that allows for dynamic adding of handlers.  It
	 * is important to note that any handler mapping registered with the server
	 * is replaced with our custom handler mapping implementation.
	 * 
	 * @param port The port for the web server to listen on.
	 * @param mbs The <code>MBeanServer</code> which stores the registered mbeans.
	 */
	public XMLRPCMBeanServerAdapter(int port, MBeanServer mbs) {
		this.webServer = new HttpServer();
		this.mbs = mbs;
		mapping = new XmlRpcHandlerMappingImpl();
		// support retrieval of notifications from clients
		mapping.addNameHandler("retrieveNotifications"); //$NON-NLS-1$
		SocketListener httpListener = new SocketListener();
		httpListener.setPort(port);
		if (httpListener != null)
			webServer.addListener(httpListener);

		ServletHandler servlets = new ServletHandler();
		servlets.setAutoInitializeServlets(true);

		ServletHolder holder = servlets.addServlet("/", InternalHttpServiceServlet.class.getName()); //$NON-NLS-1$
		holder.setInitOrder(0);
		holder.setInitParameter("enabledForExtensions", "true"); //$NON-NLS-1$ //$NON-NLS-2$

		HttpContext httpContext = new HttpContext();
		httpContext.setAttribute(INTERNAL_CONTEXT_CLASSLOADER, Thread.currentThread().getContextClassLoader());
		httpContext.setAttribute(INTERNAL_MAPPING, mapping);
		httpContext.setClassLoader(this.getClass().getClassLoader());
		httpContext.setContextPath("/"); //$NON-NLS-1$
		httpContext.addHandler(servlets);

		webServer.addContext(httpContext);
	}

	public void start() throws IOException {
		if (!started) {
			try {
				webServer.start();
			} catch (IOException e) {
				throw e;
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}

	public void stop() {
		if (webServer.isStarted()) {
			try {
				webServer.stop();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	public boolean isActive() {
		return webServer.isStarted();
	}

	public static class InternalHttpServiceServlet extends XmlRpcServlet {
		private static final long serialVersionUID = 6297772804215794345L;
		private ClassLoader contextLoader;

		public void init(ServletConfig config) throws ServletException {
			ServletContext context = config.getServletContext();
			contextLoader = (ClassLoader) context.getAttribute(INTERNAL_CONTEXT_CLASSLOADER);
			mapping = (XmlRpcHandlerMappingImpl) context.getAttribute(INTERNAL_MAPPING);

			Thread thread = Thread.currentThread();
			ClassLoader current = thread.getContextClassLoader();
			thread.setContextClassLoader(contextLoader);
			try {
				super.init(config);
			} finally {
				thread.setContextClassLoader(current);
			}
		}

		public void destroy() {
			Thread thread = Thread.currentThread();
			ClassLoader current = thread.getContextClassLoader();
			thread.setContextClassLoader(contextLoader);
			try {
				super.destroy();
			} finally {
				thread.setContextClassLoader(current);
			}
			contextLoader = null;
		}

		public void service(ServletRequest req, ServletResponse res) throws ServletException, IOException {
			Thread thread = Thread.currentThread();
			ClassLoader current = thread.getContextClassLoader();
			thread.setContextClassLoader(contextLoader);
			try {
				super.service(req, res);
			} finally {
				thread.setContextClassLoader(current);
			}
		}
		
		protected XmlRpcHandlerMapping newXmlRpcHandlerMapping() {
			return mapping;
		}
		
	}
	
	protected class XmlRpcHandlerMappingImpl implements XmlRpcHandlerMapping {

		// use hashtable as synchronization is required
		private final Map handlerMap = new Hashtable();
		private final Handler handler = new Handler();

		public void addNameHandler(String name) {
			handlerMap.put(name, handler);
		}

		public void addObjectHandler(ObjectName name, Object obj) {
			handlerMap.put(name.toString(), handler);
			// if we are to handle the object, we must attempt to register with it 
			// to receive notifications so we are able to forward them to connected clients.
			if (obj instanceof NotificationBroadcaster) {
				((NotificationBroadcaster) obj).addNotificationListener(XMLRPCMBeanServerAdapter.this, null, null);
				// cache the broadcaster and its associated object name 
				notificationBroadcasters.put(obj, name);
			}
		}

		/* (non-Javadoc)
		 * @see org.apache.xmlrpc.server.XmlRpcHandlerMapping#getHandler(java.lang.String)
		 */
		public XmlRpcHandler getHandler(String handlerName) throws XmlRpcNoSuchHandlerException, XmlRpcException {
			XmlRpcHandler result = (XmlRpcHandler) handlerMap.get(parseRequest(handlerName)[0]);
			return result;
		}

		private String[] parseRequest(String requestName) {
			int delimIdx = requestName.indexOf('|');
			String[] result = null;
			if (delimIdx != -1) {
				result = new String[2];
				result[0] = requestName.substring(0, delimIdx);
				result[1] = requestName.substring(delimIdx + 1);
			} else {
				result = new String[] {requestName};
			}
			return result;
		}

		private class Handler implements XmlRpcHandler {

			public Object execute(XmlRpcRequest pRequest) throws XmlRpcException {
				String request[] = parseRequest(pRequest.getMethodName());
				String methodName = null;
				if (request.length == 1) {
					// mbean server request
					methodName = request[0];
				} else if (request.length == 2) {
					// mbean request
					methodName = request[1];
				} else {
					// unsupported request format
					return null;
				}
				int nParams = pRequest.getParameterCount();
				Class paramTypes[] = new Class[nParams];
				Object params[] = new Object[nParams];
				String signature[] = new String[nParams];
				for (int i = 0; i < nParams; i++) {
					Object iParam = pRequest.getParameter(i);
					paramTypes[i] = iParam.getClass();
					params[i] = iParam;
					signature[i] = iParam.getClass().getName();
				}
				try {
					if (request.length == 1) {
						Method method = XMLRPCMBeanServerAdapter.this.getClass().getMethod(methodName, paramTypes);
						return method.invoke(XMLRPCMBeanServerAdapter.this, params);
					}
					// invoke operation on object instance
					return invoke(ObjectName.getInstance(request[0]), methodName, params, signature);
				} catch (Exception e) {
					throw new XmlRpcException(e.getMessage(), e);
				}
			}
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.MBeanServerForwarder#getMBeanServer()
	 */
	public MBeanServer getMBeanServer() {
		return this;
	}

	/* (non-Javadoc)
	 * @see javax.management.remote.MBeanServerForwarder#setMBeanServer(javax.management.MBeanServer)
	 */
	public void setMBeanServer(MBeanServer mbs) {
		if (this.mbs == null) {
			this.mbs = mbs;
		}
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#createMBean(java.lang.String, javax.management.ObjectName)
	 */
	public ObjectInstance createMBean(String className, ObjectName name) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException {
		return mbs.createMBean(className, name);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#createMBean(java.lang.String, javax.management.ObjectName, javax.management.ObjectName)
	 */
	public ObjectInstance createMBean(String className, ObjectName name, ObjectName loaderName) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException, InstanceNotFoundException {
		return mbs.createMBean(className, name, loaderName);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#createMBean(java.lang.String, javax.management.ObjectName, java.lang.Object[], java.lang.String[])
	 */
	public ObjectInstance createMBean(String className, ObjectName name, Object[] params, String[] signature) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException {
		return mbs.createMBean(className, name, params, signature);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#createMBean(java.lang.String, javax.management.ObjectName, javax.management.ObjectName, java.lang.Object[], java.lang.String[])
	 */
	public ObjectInstance createMBean(String className, ObjectName name, ObjectName loaderName, Object[] params, String[] signature) throws ReflectionException, InstanceAlreadyExistsException, MBeanRegistrationException, MBeanException, NotCompliantMBeanException, InstanceNotFoundException {
		return mbs.createMBean(className, name, loaderName, params, signature);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#registerMBean(java.lang.Object, javax.management.ObjectName)
	 */
	public ObjectInstance registerMBean(Object object, ObjectName name) throws InstanceAlreadyExistsException, MBeanRegistrationException, NotCompliantMBeanException {
		mapping.addObjectHandler(name, object);
		return mbs.registerMBean(object, name);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#unregisterMBean(javax.management.ObjectName)
	 */
	public void unregisterMBean(ObjectName name) throws InstanceNotFoundException, MBeanRegistrationException {
		mbs.unregisterMBean(name);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getObjectInstance(javax.management.ObjectName)
	 */
	public ObjectInstance getObjectInstance(ObjectName name) throws InstanceNotFoundException {
		return mbs.getObjectInstance(name);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#queryMBeans(javax.management.ObjectName, javax.management.QueryExp)
	 */
	public Set queryMBeans(ObjectName name, QueryExp query) {
		return mbs.queryMBeans(name, query);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#queryNames(javax.management.ObjectName, javax.management.QueryExp)
	 */
	public Set queryNames(ObjectName name, QueryExp query) {
		return mbs.queryNames(name, query);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#isRegistered(javax.management.ObjectName)
	 */
	public boolean isRegistered(ObjectName name) {
		return mbs.isRegistered(name);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getMBeanCount()
	 */
	public Integer getMBeanCount() {
		return mbs.getMBeanCount();
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getAttribute(javax.management.ObjectName, java.lang.String)
	 */
	public Object getAttribute(ObjectName name, String attribute) throws MBeanException, AttributeNotFoundException, InstanceNotFoundException, ReflectionException {
		return mbs.getAttribute(name, attribute);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getAttributes(javax.management.ObjectName, java.lang.String[])
	 */
	public AttributeList getAttributes(ObjectName name, String[] attributes) throws InstanceNotFoundException, ReflectionException {
		return mbs.getAttributes(name, attributes);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#setAttribute(javax.management.ObjectName, javax.management.Attribute)
	 */
	public void setAttribute(ObjectName name, Attribute attribute) throws InstanceNotFoundException, AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException {
		mbs.setAttribute(name, attribute);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#setAttributes(javax.management.ObjectName, javax.management.AttributeList)
	 */
	public AttributeList setAttributes(ObjectName name, AttributeList attributes) throws InstanceNotFoundException, ReflectionException {
		return mbs.setAttributes(name, attributes);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#invoke(javax.management.ObjectName, java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	public Object invoke(ObjectName name, String operationName, Object[] params, String[] signature) throws InstanceNotFoundException, MBeanException, ReflectionException {
		return mbs.invoke(name, operationName, params, signature);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getDefaultDomain()
	 */
	public String getDefaultDomain() {
		return mbs.getDefaultDomain();
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getDomains()
	 */
	public String[] getDomains() {
		return mbs.getDomains();
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#addNotificationListener(javax.management.ObjectName, javax.management.NotificationListener, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void addNotificationListener(ObjectName name, NotificationListener listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException {
		mbs.addNotificationListener(name, listener, filter, handback);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#addNotificationListener(javax.management.ObjectName, javax.management.ObjectName, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void addNotificationListener(ObjectName name, ObjectName listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException {
		mbs.addNotificationListener(name, listener, filter, handback);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#removeNotificationListener(javax.management.ObjectName, javax.management.ObjectName)
	 */
	public void removeNotificationListener(ObjectName name, ObjectName listener) throws InstanceNotFoundException, ListenerNotFoundException {
		mbs.removeNotificationListener(name, listener);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#removeNotificationListener(javax.management.ObjectName, javax.management.ObjectName, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void removeNotificationListener(ObjectName name, ObjectName listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException, ListenerNotFoundException {
		mbs.removeNotificationListener(name, listener, filter, handback);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#removeNotificationListener(javax.management.ObjectName, javax.management.NotificationListener)
	 */
	public void removeNotificationListener(ObjectName name, NotificationListener listener) throws InstanceNotFoundException, ListenerNotFoundException {
		mbs.removeNotificationListener(name, listener);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#removeNotificationListener(javax.management.ObjectName, javax.management.NotificationListener, javax.management.NotificationFilter, java.lang.Object)
	 */
	public void removeNotificationListener(ObjectName name, NotificationListener listener, NotificationFilter filter, Object handback) throws InstanceNotFoundException, ListenerNotFoundException {
		mbs.removeNotificationListener(name, listener, filter, handback);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getMBeanInfo(javax.management.ObjectName)
	 */
	public MBeanInfo getMBeanInfo(ObjectName name) throws InstanceNotFoundException, IntrospectionException, ReflectionException {
		return mbs.getMBeanInfo(name);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#isInstanceOf(javax.management.ObjectName, java.lang.String)
	 */
	public boolean isInstanceOf(ObjectName name, String className) throws InstanceNotFoundException {
		return mbs.isInstanceOf(name, className);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#instantiate(java.lang.String)
	 */
	public Object instantiate(String className) throws ReflectionException, MBeanException {
		return mbs.instantiate(className);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#instantiate(java.lang.String, javax.management.ObjectName)
	 */
	public Object instantiate(String className, ObjectName loaderName) throws ReflectionException, MBeanException, InstanceNotFoundException {
		return mbs.instantiate(className, loaderName);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#instantiate(java.lang.String, java.lang.Object[], java.lang.String[])
	 */
	public Object instantiate(String className, Object[] params, String[] signature) throws ReflectionException, MBeanException {
		return mbs.instantiate(className, params, signature);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#instantiate(java.lang.String, javax.management.ObjectName, java.lang.Object[], java.lang.String[])
	 */
	public Object instantiate(String className, ObjectName loaderName, Object[] params, String[] signature) throws ReflectionException, MBeanException, InstanceNotFoundException {
		return mbs.instantiate(className, loaderName, params, signature);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#deserialize(javax.management.ObjectName, byte[])
	 */
	public ObjectInputStream deserialize(ObjectName name, byte[] data) throws InstanceNotFoundException, OperationsException {
		return mbs.deserialize(name, data);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#deserialize(java.lang.String, byte[])
	 */
	public ObjectInputStream deserialize(String className, byte[] data) throws OperationsException, ReflectionException {
		return mbs.deserialize(className, data);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#deserialize(java.lang.String, javax.management.ObjectName, byte[])
	 */
	public ObjectInputStream deserialize(String className, ObjectName loaderName, byte[] data) throws InstanceNotFoundException, OperationsException, ReflectionException {
		return mbs.deserialize(className, loaderName, data);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getClassLoaderFor(javax.management.ObjectName)
	 */
	public ClassLoader getClassLoaderFor(ObjectName mbeanName) throws InstanceNotFoundException {
		return mbs.getClassLoaderFor(mbeanName);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getClassLoader(javax.management.ObjectName)
	 */
	public ClassLoader getClassLoader(ObjectName loaderName) throws InstanceNotFoundException {
		return mbs.getClassLoader(loaderName);
	}

	/* (non-Javadoc)
	 * @see javax.management.MBeanServer#getClassLoaderRepository()
	 */
	public ClassLoaderRepository getClassLoaderRepository() {
		return mbs.getClassLoaderRepository();
	}

	public synchronized NamedNotification[] retrieveNotifications(Long startId) {
		int nextPos = notificationsBuffer.getNextPosition();
		if (nextPos == 0) {
			return new NamedNotification[0];
		}
		NamedNotification lastNotification = (NamedNotification) notificationsBuffer.getObject(nextPos - 1);
		long pStartId = startId.longValue();
		if (lastNotification.getNotificationId() < pStartId) {
			return new NamedNotification[0];
		}
		pStartId %= notificationsBuffer.getSize();
		List result = new ArrayList(1);
		for (; pStartId != nextPos; pStartId++) {
			NamedNotification nn = (NamedNotification) notificationsBuffer.getObject((int) pStartId);
			if (pStartId == notificationsBuffer.getSize()) {
				pStartId %= notificationsBuffer.getSize();
			}
			result.add(nn);
		}
		return (NamedNotification[]) result.toArray(new NamedNotification[result.size()]);
	}

	/* (non-Javadoc)
	 * @see javax.management.NotificationListener#handleNotification(javax.management.Notification, java.lang.Object)
	 */
	public synchronized void handleNotification(Notification notification, Object handback) {
		// if the source of the notification is registered as a known broadcaster
		// we store this notification and its object name for requesting clients
		Object source = notification.getSource();
		ObjectName broadcasterName = null;
		if (source != null && (broadcasterName = (ObjectName) notificationBroadcasters.get(source)) != null) {
			notificationsBuffer.add(new NamedNotification(broadcasterName, notification));
		}
	}
}
