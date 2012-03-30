Synopsis
========

The treexpr library is a Free Software library that applies regular expression concepts to tree structures. It it written in C and has a JNI binding to Java.

Primer
======

Syntax
------

    Expr   ::= Term | Expr "|" Term
    Term   ::= Factor | Term Factor
    Factor ::= Symbol | Symbol "*" | "~" | "(" Expr ")" | "(" Expr ")" "*"
               | Symbol "->" Expr | Symbol ":" String | Symbol Attrs | Symbol Attrs "->" Expr
    Attrs  ::= "<" Attr* ">"
    Attr   ::= Symbol | Symbol "=" String
    Symbol ::= any alphanumeric string with '_' and '-'
    String ::= quoted string (skips over \")

Semantics
---------

A tree can be thought of as a hierarchical list, each element of the list can have a list of
children. Consider the following tree:

        html
        /   \
     head   body
      /      / \
    title   h1  p 

    root list: html
    list of html's children: head body
    list of head's children: title
    list of body's children: h1 p
    list of h1's chlidren: ~ (where ~ denotes the empty list)
    list of p's children: ~

By the same token, a tree expression can be thought of as a hierarchical regular expression,
each symbol in the expression can have a child expression. The `->` operator binds a child
expression to a symbol in the parent expression. If a symbol has no child expression it is
equivalent to having a child expression that matches everything. In other words the children
of a symbol are ignored if there is no child expression. It can also be said that `->` adds
a restriction to the symbol on it's left-hand side: the symbol will only be matched if the
symbol's children match the child expression. Other restriction operators are discussed later to
improve matching of HTML. Consider the following expressions:

These match the tree above:

    html                        - matches the root list and ignores html's children
    html -> head body           - also matches html's children and ignores head and body's children
    html -> (head -> title) body        - also matches head's children and ignores title's children
    html -> (head -> title -> ~) body   - also matches title's children
    html -> (head -> title -> ~) (body -> h1 p)                 - also matches body's children
    html -> (head -> title -> ~) (body -> (h1 -> ~) (p -> ~))   - matches the tree exactly

These do not match the tree above:

    body                - does not match root list
    html -> body        - matches root list but does not match html's children (must match all)
    html -> body head   - matches root list but html's children are out of order

Note the grouping around `->`. In the second expression `->` binds the symbol `html` to the
expression `head body`. In the third expression we want the symbol `head` to be bound to the
expression `title` and not `title body` so we must group them together using parenthesis.

Tree expressions use the alternation `|` and closure `*` operators from reqular expressions.
Consider the following expressions:

    html*       - matches the root list because it consists of zero or more "html" symbols
    html | xml  - matches the root list because it consists of "html" or "xml"
    html -> (head -> title | title meta) body   - matches

In addition to the `~` special symbol there is `.` which matches any symbol:

    html -> .*          - matches the root list and html's children because html has zero or more
    html -> .* (body -> .* p .*) .*             - matches a tree that has "html" at the root, html has
                                                  atleast one child named "body", and body has atleast
                                                  one child named "p".

Extra matching operators for HTML
---------------------------------

The `:` operator binds a regular expression to a symbol, the symbol matches only if its
contents matches the regular expression. In HTML the only tags that have contents are `text`
and `comment`. So most of the time it will be used like:

    p -> text:"ab*c"    - matches the HTML '<p>abbbbbc</p>'

The `<`, `=`, and `>` operators are used together to bind a symbol to an attribute list,
the symbol matches only if every attribute matches an attribute of the HTML tag. An attribute
restriction is a list of name and value pairs. A name without a value is matched only if it
appears in the HTML without a value. Consider the following:

    table <bgcolor="blue">      - matches the HTML '<table bgcolor="blue">'
                                  and also the HTML '<table bgcolor="blue" border="1">'
    foo <bar>                   - matches the HTML '<foo bar>' but not '<foo bar="">' or '<foo bar="baz">'
    foo <bar> | foo <bar=".*">  - matches all three

It is possible to combine the attribute restriction operators with the `->` operator (for example
`option <selected> -> text:"blue"`), but it is not possible to combine the content restriction
operator with the `->` operator (for example `text:"blue" -> br`) because `text` and `comment`
symbols never have children.

Extracting values from an HTML page
-----------------------------------

This section has been up for debate because it can be very confusing. The way we extract strings
from the tree expressions is from the contents and attribute rescrictions. These restrictions
contain regular expressions that match against some of the data contained in an HTML page. Most
regular expression libraries allow you to use something called back references where each
parenthasized sub-expression is given a number and the string matching that sub-expression can be
referenced later. For example the expression "foo" has no sub-expressions, the expression `fo(o*)`
has one sub-expression, namely `o*`. When `fo(o*)` is matched against "fooo", the fist
sub-expression references the string "oo". The sub-expressions are numbered by the position of
their open parenthesis. For example `(b(a)(r))` has three sub-expressions, 1. `b(a)(r)` 2. `a`
3. `r`. If a sub-expression matches multiple strings then the last match is what is referenced.
For example `(ab|ac)*` matches "ababac" and the string referenced will be "ac".

A tree expression can have several regular expressions embedded in it. In order to find a specific
sub-expression they are numbered in the order they appear in the tree expression. For example:

        form -> input<value="([0-9]+)"> text:"."
                input<value="([0-9]+)"> text:"."
                input<value="([0-9]+)"> text:"."
                input<value="([0-9]+)"> input

has four sub-expressions. This tree expression would match against an HTML form that might be used
for changing an IP address:

    <form method="POST" action="/cgi-bin/setip">
        <input type="text" name="first" value="192"> .
        <input type="text" name="second" value="168"> .
        <input type="text" name="third" value="1"> .
        <input type="text" name="fourth" value="42"> <input type="submit" value="Set IP">
    </form>

In this case the extracted values will be:

1. "192"
2. "168"
3. "1"
4. "42"

It's simple to specify a method for combining the strings together to form a usefull output.
For example something like `\1.\2.\3.\4` would produce "192.168.1.42".

Example Program
---------------

    /* Example program to output all 1st headings in an HTML document */
    /* NB: please tell me if this works, I haven't tested it */
    #include <stdio.h>
    #include <treexpr.h>
    #include <libxml/HTMLtree.h>
    #include <libxml/HTMLparser.h>
    
    int main( int argc, char **argv )
    {
        const char *expr = "h1 -> text:\"(.*)\""; /* Expression to match */
        struct machine *m; /* Pointer to compiled expression */
        htmlDocPtr doc; /* Pointer to HTML document */
        const char *end; /* end of expression parsed */
        int error = 0; /* return code of main */
        struct match *z; /* matches returned by document_process */
    
        /* Check command line options */
        if( argc != 2 )
        {
                printf( "USAGE: %s <url>\n", argv[0] );
                return -1;
        }
    
        /* Parse expression, a pointer to the compiled expression is stored at m */
        end = parse_treexpr( expr, &m );
        if( end == NULL )
        {
                printf( "Error parsing expression near: %s\n", m->buf );
                printf( "Details: %s\n", m->error );
                error = -1;
                goto free_m;
        }
    
        /* Load HTML document from a URL */
        doc = htmlReadFile( arg[1], NULL, PARSER_OPTIONS );
        if( doc == NULL )
        {
                printf( "Error reading document: %s\n", arg[1] );
                error = -1;
                goto free_m;
        }
    
        /* Search for any matches of the expression */
        z = document_process( m, doc );
    
        /* Loop through linked list of matches */
        while( z != NULL )
        {
                struct regex_match *re = z->re; /* First subexpression */
                int i;
    
                puts( "Heading: " );
                for( i = re->match.rm_so; i < re->match.rm_eo; i++ )
                        putchar( re->str[i] );
                putchar( '\n' );
                z = z->next; /* Consider next match in list */
        }
    
        /* Free memory allocated by document_process */
        free_matches( z );
    
        /* Free memory and return */
        xmlFreeDoc( doc );
    free_m:
        free_machine( m );
        return error;
    }

TODO
====

* Autotoolize build
* Include a better C example program, possibly a make test
* Write lots of example expressions, maybe even a CGI interface for people to play with
* Clean up namespace
* Use SourceForge tasks instead of this list
* Rearrange code, make it usefull for more than just HTML documents
* Add Python bindings
