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

public class RingBuffer {

	private Object[] buffer;
	private int next = 0;
	private int size;

	public RingBuffer(int size) {
		buffer = new Object[this.size = Math.max(1, size)];
	}

	public synchronized int add(Object obj) {
		next %= size;
		buffer[next] = obj;
		return next++;
	}

	public synchronized Object[] subset(int start, int end) {
		start = start >= 0 ? start : 0;
		end = end <= next ? end : next;
		if (start >= end) {
			return null;
		}
		Object[] result = new Object[end - start];
		for (int i = 0; i < result.length; i++) {
			result[i] = buffer[start++];
		}
		return result;
	}

	public Object[] subset(int start) {
		return subset(start, next);
	}

	public synchronized Object getObject(int idx) {
		if (idx < 0 || next == 0) {
			return null;
		}
		return buffer[idx < next ? idx : next - 1];
	}

	public int getNextPosition() {
		return next;
	}

	public int getSize() {
		return size;
	}
}
