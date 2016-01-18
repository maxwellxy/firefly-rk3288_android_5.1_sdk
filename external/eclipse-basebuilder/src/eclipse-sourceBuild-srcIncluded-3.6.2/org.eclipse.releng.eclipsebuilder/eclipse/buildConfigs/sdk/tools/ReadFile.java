/*******************************************************************************
 * Copyright (c) 2004, 2006 IBM Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation - initial API and implementation
 *******************************************************************************/

/*
 * Created on Feb 23, 2004
 *
 * To change the template for this generated file go to
 * Window - Preferences - Java - Code Generation - Code and Comments
 */
/**
 * @author kmoir
 *
 * A custom ant task that will generate pdf documents for all the Eclipse documentation
 * 
 * The caller must set the directory, the regular expression to search for in the the directory,
 * the name of the script to generate pdf, the program and the parameters to run the html to pdf 
 * conversion.
 * 
 */


import java.io.*;
import java.util.*;
import org.apache.tools.ant.Task;


public class ReadFile extends Task {
		 /**
		  * 
		  */
	 
     	 ArrayList listLinks = new ArrayList();
     	 ArrayList tempList = new ArrayList();
 		
		 String xmlFileName = "toc.xml";
		 String tempFileName = xmlFileName +".2";		
		 	
		 String initialPath = "";
		 String htmlFileName = "toc.html";
		 String htmlFile = initialPath + htmlFileName;	 
		 String docDir;
		 String scriptName = "test.sh";
		 String scriptParam = "htmldoc --book -f";
		 String styleSheet = ""; 
		 String docName = "";
		 	
	
	 public ReadFile() {
		 super();
		
		 
		 
		 // TODO Auto-generated constructor stub
}
	
	    	     	
						 
		 public String getInitialPath() {		 	
		 	return initialPath;
		 }		 
		 
		 public String getHtmlFileName() {
		 	return htmlFileName;
		 }
		 
		 public String getTempFileName() {
		 	return tempFileName;
		 }
		 
		 public String getScriptName() {
		 	return scriptName;
		 }
		 
		 public String getScriptParam() {
		 	return scriptParam;
		 }
		 /**
			 * @return Returns the styleSheet.
			 */
			public String getStyleSheet() {
				return styleSheet;
			}
		
			/**
			 * @return Returns the docDir.
			 */
			public String getDocDir() {
				return docDir;
			}
			/**
			 * @param docDir The docDir to set.
			 */
			public void setDocDir(String docDir) {
				this.docDir = docDir;
			}
		 
			/**
			 * @param htmlFile The htmlFile to set.
			 */
			public void setHtmlFile(String htmlFile) {
				this.htmlFile = htmlFile;
			}
			/**
			 * @param htmlFileName The htmlFileName to set.
			 */
			public void setHtmlFileName(String htmlFileName) {
				this.htmlFileName = htmlFileName;
			}
			/**
			 * @param listLinks The listLinks to set.
			 */
			public void setListLinks(ArrayList listLinks) {
				this.listLinks = listLinks;
			}
			/**
			 * @param tempList The tempList to set.
			 */
			public void settempList(ArrayList tempList) {
				this.tempList = tempList;
			}
			/**
			 * @param path The path to set.
			 */
			public void setInitialPath(String path) {
				this.initialPath = path;
			}
			/**
			 * @param scriptName The scriptName to set.
			 */
			public void setScriptName(String scriptName) {
				this.scriptName = scriptName;
			}
			/**
			 * @param scriptParam The scriptParam to set.
			 */
			public void setScriptParam(String scriptParam) {
				this.scriptParam = scriptParam;
			}
			/**
			 * @param tempFileName The tempFileName to set.
			 */
			public void setTempFileName(String tempFileName) {
				this.tempFileName = tempFileName;
			}
			/**
			 * @param xmlFileName The xmlFileName to set.
			 */
			public void setXmlFileName(String xmlFileName) {
				this.xmlFileName = xmlFileName;
			}
			/**
			 * @param styleSheet The styleSheet to set.
			 */
			public void setStyleSheet(String styleSheet) {
				this.styleSheet = styleSheet;
			}			
			
			public void setdocName(String docName) {
				this.docName = docName;
			}
			/**
			 * @param styleSheet The styleSheet to set.
			 */
						
			public String getdocName() {
			 	return docName;
			 }
		 
		 void verifySourceFiles(String path, String xmlFileName, String styleSheet, String htmlFileName) {
		 	
		// 	verify that the contents of the xml file are valid		 			 			 	
		 	 
		    ArrayList fileContents = new ArrayList();		 
		 	Properties convertFiles = new Properties();		 	 		 	 
		 	convertFiles.put(path+"/"+xmlFileName+".2",path+"/"+htmlFileName);
		 	 
		 	 try {
	 
	 		 	File file = new File(path+"/" + xmlFileName);
		 		 
		 		if (!file.exists()) {		 		 	 		 	
		 			 return;
		 		 }
		 		
		 		FileReader fileReader = new FileReader(file);
		 		BufferedReader reader = new BufferedReader(fileReader);		    	
        	   		 		 		 		 		 		 		 		 		 		 		 
		 		String line = null;		 
		 		 		 		 
		 		while ((line = reader.readLine()) != null) {
		 			 fileContents.add(line);		 			
		 		}
		 		reader.close();		 		 
		 	 } catch(Exception ex) {
        		 ex.printStackTrace();
		 	 }
       
		 	 String nextLine,newFileName ="";	 	 
		 	 
		 	  	 	 
		 	 try {
		 	 	BufferedWriter outfile = new BufferedWriter(new FileWriter(path+"/"+xmlFileName+".2"));
		 	 	
		 	 	for (Iterator i = fileContents.iterator(); i.hasNext();) {
		 	 		String currentLine = ((String)(i.next()));
                   if ((currentLine.matches("(?i).*topic label=.*")) && (! currentLine.matches("(?i).*href=\".*"))) {
		 	 			//if the xml file doesn't have the appropriate link
                   	
		 	 		    	nextLine =  ((String)(i.next()));
		 	 			
		 	 			
		 	 			if (! (( nextLine.matches("(?i).*topic label=.*")) &&  (nextLine.matches("(?i).*href=\".*")))) {		 	 				
		 	 			
		 	 				//contruct a new line from the two lines
		 	 				int indexEndCurrent = currentLine.lastIndexOf(">");
		 	 				boolean indexLastSlashNext = currentLine.lastIndexOf("/") > 0;
		 	 				int indexStartNext = nextLine.indexOf("=\"");
		 	 				int indexEndNext = nextLine.lastIndexOf("\"");
		 	 					 	 			                        
		 	 				currentLine = currentLine.substring(0,indexEndCurrent);
		 	 				newFileName =  nextLine.substring(indexStartNext+2,indexEndNext);
		 	 				currentLine = currentLine + newFileName;            
		 	 					 	 			
		 	 				//	If xml file, add to the list of files that need to be converted
		 	 				if (newFileName.matches("(?i).*xml")) {		 	 				
		 	 					//replace .xml with .html in the link
		 	 					int indexStartSuffix = newFileName.indexOf(".xml");
		 	 					htmlFileName = newFileName.substring(0,indexStartSuffix) + ".html";
		 	 					convertFiles.put(path +"/" + newFileName,path+"/"+htmlFileName);		 	 		
		 	 				}
		 	 		
		 	 				String closeString = "\"/>";
		 	 			
		 	 				if (! indexLastSlashNext) {
		 	 				closeString = "\">";
		 	 				} 	 			
		 	 				String newLink = " href=\"" + htmlFileName + closeString;
		 	 				String newLine = currentLine.substring(0,indexEndCurrent) + newLink;
                       
                                           
		 	 				outfile.write(newLine+"\n");
		 	 				} else {
		 	 					outfile.write(currentLine+"\n");
		 	 					outfile.write(nextLine+"\n");		 	 					
		 	 				}
                        
		 	 				//add the xml link to the list of files that needs to be constructed via xslt
                    
                } else {                   
                    outfile.write(currentLine+"\n");
                }
		 	 }
		 	outfile.write("\n");
		 	outfile.close();
		 	 
		  } catch(Exception ex) {
		  		ex.printStackTrace();
		  }
		  //iterate through the hash of xml and html files and transform to xml via xlst
		 for (Enumeration e = convertFiles.propertyNames(); e.hasMoreElements();) {
		 		  	   Object n = e.nextElement();		  	
		  		  	   String x = n.toString();
		  		  	   String h = convertFiles.getProperty(x);		  		  
		  		  	   tempList.add(h);	
		  		  	   
		  		  	   transformXML(x,path,styleSheet,h);
		 }
        } 


		 
		 boolean addLink(String url) {
		 		 		 		 
		 //If the string is legal related then add it automatically
		 //check if the link already exists in the array
		     if (! listLinks.contains(url)) {
		 		 listLinks.add(url);
		 		 return true;
		 	 } else {
		 	 	return false;
		 	 }	 			 		 		 
		 }
		 		 		 		  
		 
		 String findUrl(String line) {
		 		 
		 int indexStart = line.indexOf("href=\"");
		 String url="";
		 if (indexStart > 0) {
		     url = line.substring(indexStart+6,line.length());
		 }
		 int indexEnd = url.indexOf("\">");
		 if (indexEnd > 0) {
		     url = url.substring(0,indexEnd);   
		 } else {
		  	url = "";	 		 	
		 }
		    return(url);		 	    
		 }
		 
		 boolean verifyUrl(String url) {	
		 	
		 	  		 
		 //verify that the link is valid and return a boolean value
		 //verify that the link doesn't start with #
	 	 if (url.matches("(?i).*#.*")){		 		 
	 	     return false;		 		 
	 	 }  
		 //verify that the link ends in .htm
		 if (!(url.matches("(?i).*.htm.*"))) {		 		 	
		     return false;
		 }
		 if (!(url.matches("(?i).*/*"))) {		 		 	
	 		  return false;
	 	 } 
		 //verify that the link doesn't start with ../..
		 if (url.startsWith("../..")){
		 	 return false;		 		 
		 }  
		 if (url.matches("../..")){		 		 	  
		 	 return false;		 		 
		 }  
		 //verify url doesn't have multiple ../.. strings
		 int indexS = url.indexOf("../");
		 int indexE = url.lastIndexOf("../");
		 if (indexE > indexS) {
		   	return(false);
		 }
		 if (listLinks.contains(url)) {
		     return(false);
		 }
		 				 	
		 //verify that the url doesn't refer to the api reference
		 if (url.matches("(?i).*/reference/api/.*")) {                        
		 	 return false;
		 } 
         if (url.matches("(?i).*/reference/.*/../api/.*")) {                        
            return false;
         } 
		 if (url.matches("(?i).*platform_whatsnew.html.*")) {
		 	 return false;
		 } 
		 //verify that the url doesn't refer to the api reference	
		 if	 (url.matches("(?i).*http://.*")) {
		     return false;
		 }                               
  		     		     return true;
	 }
		 
	
			 
		 public void parseLinks(String fileName) {
		 	
		 try {
		     File myFile = new File(fileName);
		 	 if (!myFile.exists()) {
		 		 return;
		 	 }
		 	 if	 (fileName.matches("(?i).*hglegal.*")) {
		 	 		 return;
		 	} 

		 	FileReader fileReader = new FileReader(myFile);
		 	BufferedReader reader = new BufferedReader(fileReader);		 		 		 		 		 
		 		 		 		 
		 	String line = null;
		 	ArrayList fileContents = new ArrayList();
		 	
		 	//put file contents into array
		 	while ((line = reader.readLine()) != null) {
		 		fileContents.add(line);
		 	}
		 	reader.close();
		 	    for (Iterator i = fileContents.iterator(); i.hasNext();) {
		 	 	// parse the file for lines with links
		 		    String l = (String)(i.next());
		 		    if (l.matches("(?).*href=.*")) {
		 		 			 		 		 		 		 
		 		        int indexPathStart = fileName.indexOf("/");
		 		        int indexPathEnd = fileName.lastIndexOf("/");
		 			    String path = fileName.substring(indexPathStart,indexPathEnd);
		 	   	        String nextUrl;
		 	   	        String url = findUrl(l);		 			
		 			 
		 			if (verifyUrl(path+"/"+url)) {		 			
		 				boolean upLevel = url.startsWith("../");
		 		 	 	if (upLevel) {		 		 			     
		 		 	 	 	int indexLastDirPath = path.lastIndexOf('/');		 		 		 		 		 		 
		 		 		 	String shorterPath = path.substring(0,indexLastDirPath);
		 		 		 	url = url.replaceFirst("..","");
		 		 		 	nextUrl = shorterPath + url;		 		 		 		 		 
		 		 	 	 } else {
		 		 			     nextUrl = path + "/" + url;
		 		 		 }
		 		 	 		if (addLink(nextUrl)) {
		 		 	 			if (nextUrl.matches("(?i).*samples.*")) {		 		 		 	 //  
		 		 	 			}
		 		 		 	   parseLinks(nextUrl);	
		 		 	 		}
		 		 		 }
		 		 	 }		 		
		 		  }		 		 
		  		 } catch (Exception ex) {
		 		 		   ex.printStackTrace();
		 		 }
		 	}		 
		 
		 	String findFiles(String path, final String re) {
		 		 		 		 		 		 
		 	File dir = new File(path); 		 		 
		 	File[] files = dir.listFiles();		  
		 		 
		 	//Filter the list of returned files so they only match re
		         FilenameFilter filter = new FilenameFilter() {
		         public boolean accept(File dir, String name) {
		         	    return name.matches("(?i).*"+re+".*");
		         	   //
		         }
		     };
		     
		     files = dir.listFiles(filter);
		     
		     //convert array to a string
		     String docfiles = ""; 
		     for(int i=0; i<files.length; i++ ){		     		 
		     		 docfiles = docfiles + " " + files[i];
		     }		   
		     return docfiles;       
		 }
		 	
		 String findDocName(String path) {

 		 //construct the name of the doc from the path
 		 int startDoc = path.lastIndexOf("/");
	 	 int endDoc = path.lastIndexOf("_");
		 		 
		 String docFileName = path + "/" + path.substring(startDoc+1,endDoc+6) + ".pdf";
		
		 	 return(docFileName);
		 }
		 
		 void transformXML(String fileName, String path, String styleSheet, String outputFile) {
		 			
		     String[] command;
		     command = new String[6];			 			
		 		
		     command[0] = "-in";
		     command[1] = fileName;
		     command[2] = "-xsl";
		     command[3] = styleSheet; 
		     command[4] = "-out";
		     command[5] = outputFile;
				 		
 		 	try {
 		 	    org.apache.xalan.xslt.Process.main(command);	 	      
		 	} catch (Exception ex) {		 		 	    
 		 	    	ex.printStackTrace();
 		 	}

		 	
		 }
		 
		  void writeDocGenScript(String path, String docName, String scriptName, String docParam) {
		 		 		 		 
		      String docGenScript = path + "/" + scriptName;	    	 		 
		 		 
		      try {
		          BufferedWriter outfile = new BufferedWriter(new FileWriter(docGenScript));
                  outfile.write("#!/bin/sh\n");
		          outfile.write(docParam + " " + docName+" ");
		          for (Iterator i = listLinks.iterator(); i.hasNext();) {
		              outfile.write((String)(i.next())+" ");		 		 		     
		 		 }
		         outfile.write("\n");
		         outfile.close();    
		     } catch (IOException ex) {
		     		 ex.printStackTrace();
		     }  
		     
		     try {
		         String command = "sh " + docGenScript;
		         Process child = Runtime.getRuntime().exec(command);
		    } catch (IOException ex) {
		    	 ex.printStackTrace();
		    }
		 }
		 
		 
		 		 
		 public void execute() {
             
		 String path = docDir;
		 listLinks = new ArrayList();
		 //Update xml if required
		 htmlFile = path + "/" + htmlFileName;		 	
		 //fix xml and generate html as required
		 verifySourceFiles(path,xmlFileName,styleSheet,htmlFileName);	    	 			 		 	 			
		 //add the first link to the array		 		 		 
		 listLinks.add(htmlFile);		 		 		
		 parseLinks(htmlFile);		 		 	
		 writeDocGenScript(path,docName,scriptName,scriptParam);	
		 //Clean up temporary files
		 for (Iterator f = tempList.iterator(); f.hasNext();) {		 			
		 		boolean success = new File (f.next().toString()).delete(); 
		     }	 		
		 }
		 
		 public static void main(String args[]) {		 			 	
		 		
		 	ReadFile instance = new ReadFile();
		 	instance.execute();	
		 }			
	
}


