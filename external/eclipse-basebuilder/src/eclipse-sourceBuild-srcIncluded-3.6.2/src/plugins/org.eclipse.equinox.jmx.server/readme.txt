Server Readme
-------------------

By default the server starts on port 8118 and uses RMI as its transport. Both of
these values can be changed via the following system properties:
	org.eclipse.equinox.jmx.server.protocol
	org.eclipse.equinox.jmx.server.port

Currently you can only have one server running at a time, either RMI or XMLRPC.
