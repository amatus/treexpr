/* GrokHtml.java - Tree expression language JNI interface source file
 + Copyright (C) 2005 Dell, Inc.
 + Authors: David Barksdale <amatus@ocgnet.org>
 +
 +  This library is free software; you can redistribute it and/or
 +  modify it under the terms of the GNU Lesser General Public
 +  License as published by the Free Software Foundation; either
 +  version 2.1 of the License, or (at your option) any later version.
 +
 +  This library is distributed in the hope that it will be useful,
 +  but WITHOUT ANY WARRANTY; without even the implied warranty of
 +  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 +  Lesser General Public License for more details.
 +
 +  You should have received a copy of the GNU Lesser General Public
 +  License along with this library; if not, write to the Free Software
 +  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

class GrokHtml {
	/*
	 * Free memory associated with a machine pointer
	 */
	public native void FreeMachine( long Machine );

	/*
	 * Compile a tree expression into a machine, we return a machine pointer for use
	 * with the SearchDocument function
	 */
	public native long ParseExpression( String Expression )
		throws java.text.ParseException;

	/*
	 * Free memory associated with an document pointer
	 */
	public native void FreeDocument( long Document );

	/*
	 * Open an HTML document from the specified URI, for example http://example.com/file.html
	 * we return a document pointer for use with the SearchDocument function
	 */
	public native long OpenDocumentFromURI( String URI )
		throws java.lang.RuntimeException;

	/*
	 * Parse a byte[] as an HTML document, we return a document pointer for use with
	 * the SearchDocument function
	 */
	public native long OpenDocumentFromBytes( byte[] Document )
		throws java.lang.RuntimeException;

	/*
	 * Parse a string as an HTML document, we return a document pointer for use with
	 * the SearchDocument function. You might run into some character set problems
	 * using this function; for best results use OpenDocumentFromBytes using the exact
	 * bytes returned from the web server
	 */
	public native long OpenDocumentFromString( String Document )
		throws java.lang.RuntimeException;

	/*
	 * Search the given Document using the given Machine, the resulting matches are
	 * are copied into a new string using the Pattern string as a template
	 */
	public native String SearchDocument( long Document, String Pattern, long Machine )
		throws java.lang.RuntimeException, java.lang.OutOfMemoryError,
		java.lang.IndexOutOfBoundsException;

	/*
	 * These native functions are defined in GrokHtml.dll (or GrokHtml.so)
	 */
	static { System.loadLibrary( "GrokHtml" ); }
}