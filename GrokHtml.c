/* GrokHtml.c - Tree expression language JNI interface source file
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

#include <ctype.h>
#include <string.h>
#include <jni.h>
#include <libxml/HTMLtree.h>
#include <libxml/HTMLparser.h>
#include "treexpr.h"
#include "GrokHtml.h"

#ifdef HTML_PARSE_RECOVER
#define PARSER_OPTIONS	( HTML_PARSE_RECOVER | HTML_PARSE_NOERROR \
			| HTML_PARSE_NOWARNING )
#else
#define PARSER_OPTIONS	( HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING )
#endif

#if 0
/* 64-bit machine */
#define JLONG_TO_POINTER( x )	((void *)( x ))
#define POINTER_TO_JLONG( x )	((jlong)( x ))
#else
/* 32-bit machine */
#define JLONG_TO_POINTER( x )	((void *)(int)( x ))
#define POINTER_TO_JLONG( x )	((jlong)(int)( x ))
#endif

/*
 * Class:     GrokHtml
 * Method:    FreeMachine
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_GrokHtml_FreeMachine( JNIEnv *env, jobject obj,
	jlong Machine )
{
	free_machine((struct machine *)JLONG_TO_POINTER( Machine ));
}

/*
 * Class:     GrokHtml
 * Method:    ParseExpression
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_GrokHtml_ParseExpression( JNIEnv *env, jobject obj,
	jstring Expression )
{
	const char *expr, *residue;
	struct machine *m;

	expr = (char *)(*env)->GetStringUTFChars( env, Expression, JNI_FALSE );
	residue = parse_treexpr( expr, &m );
	if( residue == NULL )
	{
		jclass exc;
		jmethodID id;
		jstring message;
		jobject ex;

		/*
		 * There was an error parsing, throw Java exception
		 */

		/* First lookup the java.text.ParseException class */
		exc = (*env)->FindClass( env, "java/text/ParseException" );
		if( exc == NULL )
			return 0;

		/*
		 * Grab the method ID for the constructor: ParseException(
		 * String s, int errorOffset )
		 */
		id = (*env)->GetMethodID( env, exc, "<init>",
			"(Ljava/lang/String;I)V" );
		if( id == 0 )
			return 0;

		/*
		 * Convert the error message from the parser to a java string
		 * object
		 */
		message = (*env)->NewStringUTF( env, m->error );

		/* Create the exception object */
		/* NB: m->buf is a pointer into the origional string where the
		 * error occured, so we take the difference from expr to find
		 * errorOffset, this will not be accurate if the string
		 * contained any non-ASCII characters
		 */
		ex = (*env)->NewObject( env, exc, id, message,
			(jint)( m->buf - expr ));

		/* Throw it */
		(*env)->Throw( env, ex );

		/* Free the left-over memory used by the parser */
		free_machine( m );
		(*env)->ReleaseStringUTFChars( env, Expression, expr );
		return 0;
	}

	/*
	 * If residue is an empty string here then the entire thing was read as
	 * an expression. It might be considered an error if there is some stuff
	 * left over, but it also might be usefull to ignore it to be a little
	 * more robust. For now it's ignored.
	 */

	/* Clean up and return the pointer as a long */
	(*env)->ReleaseStringUTFChars( env, Expression, expr );
	return POINTER_TO_JLONG( m );
}

/*
 * Class:     GrokHtml
 * Method:    FreeDocument
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_GrokHtml_FreeDocument( JNIEnv *env, jobject obj,
	jlong Document )
{
	xmlFreeDoc((htmlDocPtr)JLONG_TO_POINTER( Document ));
}

/*
 * Class:     GrokHtml
 * Method:    OpenDocumentFromURI
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_GrokHtml_OpenDocumentFromURI( JNIEnv *env,
	jobject obj, jstring URI )
{
	const char *uri;
	htmlDocPtr doc;

	uri = (char *)(*env)->GetStringUTFChars( env, URI, JNI_FALSE );
	doc = htmlReadFile( uri, NULL, PARSER_OPTIONS );
	(*env)->ReleaseStringUTFChars( env, URI, uri );
	if( doc == NULL )
	{
		jclass exc;

		/* Throw a runtime exception */
		exc = (*env)->FindClass( env, "java/lang/RuntimeException" );
		if( exc == NULL )
			return 0;

		(*env)->ThrowNew( env, exc, "Error opening HTML document" );
		return 0;
	}
	return POINTER_TO_JLONG( doc );
}

/*
 * Class:     GrokHtml
 * Method:    OpenDocumentFromBytes
 * Signature: ([B)J
 */
JNIEXPORT jlong JNICALL Java_GrokHtml_OpenDocumentFromBytes( JNIEnv *env,
	jobject obj, jbyteArray Bytes )
{
	jsize len;
	jbyte *bytes;
	htmlDocPtr doc;

	len = (*env)->GetArrayLength( env, Bytes );
	bytes = (*env)->GetByteArrayElements( env, Bytes, NULL );
	doc = htmlReadMemory((char *)bytes, len, NULL,	NULL, PARSER_OPTIONS );
	(*env)->ReleaseByteArrayElements( env, Bytes, bytes, JNI_ABORT );
	if( doc == NULL )
	{
		jclass exc;

		/* Throw a runtime exception */
		exc = (*env)->FindClass( env, "java/lang/RuntimeException" );
		if( exc == NULL )
			return 0;

		(*env)->ThrowNew( env, exc, "Error opening HTML document" );
		return 0;
	}
	return POINTER_TO_JLONG( doc );

}

/*
 * Class:     GrokHtml
 * Method:    OpenDocumentFromString
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_GrokHtml_OpenDocumentFromString( JNIEnv *env,
	jobject obj, jstring Document )
{
	const char *document;
	htmlDocPtr doc;

	document = (char *)(*env)->GetStringUTFChars( env, Document,
		JNI_FALSE );
	doc = htmlReadMemory( document, strlen( document ), NULL,
		NULL, PARSER_OPTIONS );
	(*env)->ReleaseStringUTFChars( env, Document, document );
	if( doc == NULL )
	{
		jclass exc;

		/* Throw a runtime exception */
		exc = (*env)->FindClass( env, "java/lang/RuntimeException" );
		if( exc == NULL )
			return 0;

		(*env)->ThrowNew( env, exc, "Error opening HTML document" );
		return 0;
	}
	return POINTER_TO_JLONG( doc );
}

/*
 * Class:     GrokHtml
 * Method:    SearchDocument
 * Signature: (JLjava/lang/String;J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_GrokHtml_SearchDocument( JNIEnv *env,
	jobject obj, jlong Document, jstring Pattern, jlong Machine )
{
	const char *pattern;
	struct match *z;
	struct regex_match **re, *cur;
	char *buf;
	jstring retval;
	int i, j, k, sub, len;

	/* Search the document */
	z = document_process((struct machine *)JLONG_TO_POINTER( Machine ),
		(htmlDocPtr)JLONG_TO_POINTER( Document ));
	if( z == NULL )
	{
		jclass exc;

		/* Throw a runtime exception */
		exc = (*env)->FindClass( env, "java/lang/RuntimeException" );
		if( exc == NULL )
			return NULL;

		(*env)->ThrowNew( env, exc, "No match found" );

		/* Clean up and return */
		free_matches( z );
		return NULL;
	}

	/* Count subexpressions */
	pattern = (char *)(*env)->GetStringUTFChars( env, Pattern, JNI_FALSE );
	sub = 0;
	for( i = 0; pattern[i] != 0; i++ )
		if( pattern[i] == '\\' && isdigit( pattern[i + 1] )
			&& sub < pattern[i + 1] - '0' )
			sub = pattern[i + 1] - '0';

	/*
	 * Allocate an array for matches, only enough to hold what we're going
	 * to use
	 */
	re = malloc( sub * sizeof( struct regex_match * ));
	if( re == NULL )
	{
		jclass exc;

		/* Throw an out of memory error */
		exc = (*env)->FindClass( env, "java/lang/OutOfMemoryError" );
		if( exc == NULL )
			return NULL;

		(*env)->ThrowNew( env, exc, "Unable to allocate memory" );

		/* Clean up and return */
		(*env)->ReleaseStringUTFChars( env, Pattern, pattern );
		free_matches( z );
		return NULL;
	}

	/* Fill in array */
	for( i = 0, cur = z->re; cur != NULL && i < sub; cur = cur->next, i++ )
		re[i] = cur;
	if( i < sub )
	{
		jclass exc;

		/* Throw an index out of bounds exception */
		exc = (*env)->FindClass( env,
			"java/lang/IndexOutOfBoundsException" );
		if( exc == NULL )
			return NULL;

		(*env)->ThrowNew( env, exc,
			"Not enough matches to satisfy pattern" );

		/* Clean up and return */
		free( re );
		(*env)->ReleaseStringUTFChars( env, Pattern, pattern );
		free_matches( z );
		return NULL;
	}

	/* Calculate size of output buffer */
	len = 1;
	for( i = j = 0; pattern[i] != 0; i++ )
		if( pattern[i] == '\\' && isdigit( pattern[i + 1] ))
		{
			sub = pattern[++i] - '1';
			for( k = re[sub]->match.rm_so; k < re[sub]->match.rm_eo;
				k++ )
				len++;
		}
		else
			len++;

	/* Allocate output buffer */
	buf = malloc( len );
	if( buf == NULL )
	{
		jclass exc;

		/* Throw an out of memory error */
		exc = (*env)->FindClass( env, "java/lang/OutOfMemoryError" );
		if( exc == NULL )
			return NULL;

		(*env)->ThrowNew( env, exc, "Unable to allocate memory" );

		/* Clean up and return */
		free( re );
		(*env)->ReleaseStringUTFChars( env, Pattern, pattern );
		free_matches( z );
		return NULL;
	}

	/* Fill in output buffer */
	for( i = j = 0; pattern[i] != 0; i++ )
		if( pattern[i] == '\\' && isdigit( pattern[i + 1] ))
		{
			sub = pattern[++i] - '1';
			for( k = re[sub]->match.rm_so; k < re[sub]->match.rm_eo;
				k++ )
				buf[j++] = re[sub]->str[k];
		}
		else
			buf[j++] = pattern[i];
	buf[j++] = 0;

	/* Clean up and return the string */
	free( re );
	(*env)->ReleaseStringUTFChars( env, Pattern, pattern );
	free_matches( z );
	retval = (*env)->NewStringUTF( env, buf );
	free( buf );
	return retval;
}
