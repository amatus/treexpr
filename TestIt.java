class TestIt
{	
	public static void main( String[] args )
	{
		GrokHtml foo = new GrokHtml();
		long m1 = 0, m2 = 0, doc1 = 0, doc2 = 0;
		String expr;

		/* Parse an easy expression */
		expr = "tr -> td -> text:\"foo = (.*)\"";
		try
		{
			m1 = foo.ParseExpression( expr );
			System.out.println( "Expression 1 parsed correctly" );
		}
		catch( java.text.ParseException e )
		{
			System.out.println( "Error parsing expression 1: " + e.getMessage( ) + "\n" + expr );
			int i, off = e.getErrorOffset( );
			for( i = 0; i < off; i++ )
				System.out.print( " " );
			System.out.println( "^" );
		}

		/* Parse another easy expression */
		expr = "tr -> td -> text:\"bar = (.*)\"";
		try
		{
			m2 = foo.ParseExpression( expr );
			System.out.println( "Expression 2 parsed correctly" );
		}
		catch( java.text.ParseException e )
		{
			System.out.println( "Error parsing expression 2: " + e.getMessage( ) + "\n" + expr );
			int i, off = e.getErrorOffset( );
			for( i = 0; i < off; i++ )
				System.out.print( " " );
			System.out.println( "^" );
		}

		/* Parse a broken expression */
		expr = "table<border=\"0\" foo=>";
		try
		{
			long m = foo.ParseExpression( expr );
			System.out.println( "This expression should not parse correctly" );
		}
		catch( java.text.ParseException e )
		{
			System.out.println( "Error parsing broken expression: " + e.getMessage( ) + "\n" + expr );
			int i, off = e.getErrorOffset( );
			for( i = 0; i < off; i++ )
				System.out.print( " " );
			System.out.println( "^" );
		}

		/* Open a document from a byte[] */
		try
		{
			String testdoc = "<h1>Foo!</h1><h2>Bar!</h2><table><tr><td>foo = baz</td></tr></table>";
			doc1 = foo.OpenDocumentFromBytes( testdoc.getBytes( "US-ASCII" ));
			System.out.println( "Document opened successfully from a string" );
		}
		catch( java.lang.RuntimeException e )
		{
			System.out.println( e.getMessage( ));
		}
		catch( java.io.UnsupportedEncodingException e )
		{
			System.out.println( e.getMessage( ));
		}

		/* Open a document from a URI */
		try
		{
			doc2 = foo.OpenDocumentFromURI( "test.html" );
			System.out.println( "Document opened successfully from a URI" );
		}
		catch( java.lang.RuntimeException e )
		{
			System.out.println( e.getMessage( ));
		}

		/* Try both machines on doc 1 */
		if( doc1 != 0 )
		{
			if( m1 != 0 )
			{
				try
				{
					String result = foo.SearchDocument( doc1, "foo is \\1", m1 );
					System.out.println( "Expression 1 on doc 1: " + result );
				}
				catch( java.lang.RuntimeException e )
				{
					System.out.println( e.getMessage( ));
				}
				catch( java.lang.OutOfMemoryError e )
				{
					System.out.println( e.getMessage( ));
				}
			}

			if( m2 != 0 )
			{
				try
				{
					String result = foo.SearchDocument( doc1, "bar is \\1", m2 );
					System.out.println( "Expression 2 on doc 1: " + result );
				}
				catch( java.lang.RuntimeException e )
				{
					System.out.println( "This error is ok: " + e.getMessage( ));
				}
				catch( java.lang.OutOfMemoryError e )
				{
					System.out.println( e.getMessage( ));
				}
			}

			/* Free it now that we're finished */
			foo.FreeDocument( doc1 );
		}

		/* Try both machines on doc 2 */
		if( doc2 != 0 )
		{
			if( m1 != 0 )
			{
				try
				{
					String result = foo.SearchDocument( doc2, "foo is \\1", m1 );
					System.out.println( "Expression 1 on doc 2: " + result );
				}
				catch( java.lang.RuntimeException e )
				{
					System.out.println( e.getMessage( ));
				}
				catch( java.lang.OutOfMemoryError e )
				{
					System.out.println( e.getMessage( ));
				}
			}

			if( m2 != 0 )
			{
				try
				{
					String result = foo.SearchDocument( doc2, "bar is \\1", m2 );
					System.out.println( "Expression 2 on doc 2: " + result );
				}
				catch( java.lang.RuntimeException e )
				{
					System.out.println( e.getMessage( ));
				}
				catch( java.lang.OutOfMemoryError e )
				{
					System.out.println( e.getMessage( ));
				}
			}

			/* Free it now that we're finished */
			foo.FreeDocument( doc2 );
		}

		/* Free the machines */
		if( m1 != 0 )
			foo.FreeMachine( m1 );
		if( m2 != 0 )
			foo.FreeMachine( m2 );
	}	
}