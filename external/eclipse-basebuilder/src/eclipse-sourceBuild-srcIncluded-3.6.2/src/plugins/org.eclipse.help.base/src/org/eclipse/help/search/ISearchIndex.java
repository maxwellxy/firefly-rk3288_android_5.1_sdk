/*******************************************************************************
 * Copyright (c) 2005, 2010 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation - initial API and implementation
 *******************************************************************************/
package org.eclipse.help.search;

import java.net.URL;

import org.apache.lucene.document.Document;
import org.eclipse.core.runtime.IStatus;

/**
 * Represents a Lucene index for one locale. The interface is used
 * to allow participants to delegate indexing of documents outside
 * of the TOC using the same algorithms as those in TOC.
 * @deprecated 
 * This interface is deprecated because it exposes Lucene classes, 
 * which are not binary compatible between major release. This
 * interface was used by clients which implemented the extension
 * point org.eclipse.help.bace.luceneSearchParticipants. The 
 * extension point org.eclipse.help.base.searchParticipant 
 * and the interface IHelpSearchIndex should be used instead.
 */

public interface ISearchIndex {

	/**
	 * Adds a document to the search index by parsing it using one of the file-based search
	 * participants, or the default HTML search participant. Use this method when encountering
	 * documents outside of TOC that are nevertheless of the known format and help system knows how
	 * to handle.
	 * 
	 * @param pluginId
	 *            the id of the contributing plug-in
	 * @param name
	 *            the name of the document
	 * @param url
	 *            the URL of the document using format '/pluginId/href'
	 * @param id
	 *            the unique id of this document as defined in the participant
	 * @param doc
	 *            the Lucene document
	 * @return the status of the operation
	 */
	IStatus addDocument(String pluginId, String name, URL url, String id, Document doc);
	
	/**
	 * A search index is created for each locale.
	 * @return the locale associated with this index.
	 */
	String getLocale();
}
