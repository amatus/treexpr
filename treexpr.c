/* treexpr.c - Tree expression language source file
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

-- Syntax --
 Expr   ::= Term | Expr "|" Term
 Term   ::= Factor | Term Factor
 Factor ::= Symbol | Symbol "*" | "~" | "(" Expr ")" | "(" Expr ")" "*"
			| Symbol "->" Expr | Symbol ":" String | Symbol Attrs | Symbol Attrs "->" Expr
 Attrs  ::= "<" Attr* ">"
 Attr   ::= Symbol | Symbol "=" String
 Symbol ::= any alphanumeric string with '_' and '-'
 String ::= quoted string (skips over \")

-- Semantics --
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
each symbol in the expression can have a child expression. The "->" operator binds a child
expression to a symbol in the parent expression. If a symbol has no child expression it is
equivalent to having a child expression that matches everything. In other words the children
of a symbol are ignored if there is no child expression. It can also be said that "->" adds
a restriction to the symbol on it's left-hand side: the symbol will only be matched if the
symbol's children match the child expression. Other restriction operators are discussed later to
improve matching of HTML. Consider the following expressions:

  These match the tree above:
	html					- matches the root list and ignores html's children
	html -> head body		- also matches html's children and ignores head and body's children
	html -> (head -> title) body	- also matches head's children and ignores title's children
	html -> (head -> title -> ~) body							- also matches title's children
	html -> (head -> title -> ~) (body -> h1 p)					- also matches body's children
	html -> (head -> title -> ~) (body -> (h1 -> ~) (p -> ~))	- matches the tree exactly

  These do not match the tree above:
	body				- does not match root list
	html -> body		- matches root list but does not match html's children (must match all)
	html -> body head	- matches root list but html's children are out of order

Note the grouping around "->". In the second expression "->" binds the symbol "html" to the
expression "head body". In the third expression we want the symbol "head" to be bound to the
expression "title" and not "title body" so we must group them together using parenthesis.

Tree expressions use the alternation "|" and closure "*" operators from reqular expressions.
Consider the following expressions:
	html*	- matches the root list because it consists of zero or more "html" symbols
	html | xml	- matches the root list because it consists of "html" or "xml"
	html -> (head -> title | title meta) body	- matches

In addition to the "~" special symbol there is "." which matches any symbol:
	html -> .*		- matches the root list and html's children because html has zero or more
	html -> .* (body -> .* p .*) .*		- matches a tree that has "html" at the root, html has
										  atleast one child named "body", and body has atleast
										  one child named "p".

-- Extra restriction operators for HTML matching --

The ":" operator binds a regular expression to a symbol, the symbol matches only if it's
contents matches the regular expression. In HTML the only tags that have contents are "text"
and "comment". So most of the time it will be used like:
	p -> text:"ab*c"	- matches the HTML '<p>abbbbbc</p>'

The "<", "=", and ">" operators are used together to bind a symbol to an attribute list,
the symbol matches only if every attribute matches an attribute of the HTML tag. An attribute
restriction is a list of name and value pairs. A name without a value is matched only if it
appears in the HTML without a value. Consider the following:
	table <bgcolor="blue">	- matches the HTML '<table bgcolor="blue">'
						  and also the HTML '<table bgcolor="blue" border="1">'
	foo <bar>			- matches the HTML '<foo bar>' but not '<foo bar="">' or '<foo bar="baz">'
	foo <bar> | foo <bar=".*">	- matches all three

It is possible to combine the attribute restriction operators with the "->" operator (for example
option <selected> -> text:"blue"), but it is not possible to combine the content restriction
operator with the "->" operator (for example text:"blue" -> br) because "text" and "comment"
symbols never have children.

-- Extracting values from an HTML page --
This section has been up for debate because it can be very confusing. The way we extract strings
from the tree expressions is from the contents and attribute rescrictions. These restrictions
contain regular expressions that match against some of the data contained in an HTML page. Most
regular expression libraries allow you to use something called back references where each
parenthasized sub-expression is given a number and the string matching that sub-expression can be
referenced later. For example the expression "foo" has no sub-expressions, the expression "fo(o*)"
has one sub-expression, namely "o*". When "fo(o*)" is matched against "fooo", the fist
sub-expression references the string "oo". The sub-expressions are numbered by the position of
their open parenthesis. For example "(b(a)(r))" has three sub-expressions, 1. "b(a)(r)" 2. "a"
3. "r". If a sub-expression matches multiple strings then the last match is what is referenced.
For example "(ab|ac)*" matches "ababac" and the string referenced will be "ac".

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
1: "192"
2: "168"
3: "1"
4: "42"
It's simple to specify a method for combining the strings together to form a usefull output.
For example something like "\1.\2.\3.\4" would produce "192.168.1.42".
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libxml/tree.h>
#include <sys/types.h>
#include "regex.h"
#include "treexpr.h"

#ifdef _WINDOWS
#define strcasecmp stricmp
#endif

// allocator that never returns NULL, cheap hack to make code shorter
void *zalloc( size_t size )
{
	void *p;

	do { p = calloc( size, 1 );
	} while( p == NULL );
	return p;
}

// builds a machine that matches a symbol
struct machine *symbol( char *name )
{
	struct machine *m;
	struct state *start, *final;
	struct trans *tr;

	// alloc stuff
	m = zalloc( sizeof( struct machine ));
	start = zalloc( sizeof( struct state ));
	final = zalloc( sizeof( struct state ));
	tr = zalloc( sizeof( struct trans ));

	// insert transition
	tr->name = name;
	tr->st = final;
	start->tr = tr;

	// maintain linked list
	start->next = final;

	// finish machine
	m->start = start;
	m->final = final;
	return m;
}

// builds a machine that matches the empty string
struct machine *epsilon( void )
{
	struct machine *m;
	struct state *start, *final;
	struct epsilon *cat;

	// alloc stuff
	m = zalloc( sizeof( struct machine ));
	start = zalloc( sizeof( struct state ));
	final = zalloc( sizeof( struct state ));
	cat = zalloc( sizeof( struct epsilon ));

	// insert epsilon transition
	cat->st = final;
	start->ep = cat;

	// maintain liked list
	start->next = final;

	// finish machine
	m->start = start;
	m->final = final;
	return m;
}

// builds a machine that matches nothing (not used at present, but here for completeness)
struct machine *null( void )
{
	struct machine *m;
	struct state *start, *final;

	// alloc stuff
	m = zalloc( sizeof( struct machine ));
	start = zalloc( sizeof( struct state ));
	final = zalloc( sizeof( struct state ));

	// maintain linked list
	start->next = final;

	// finish machine
	m->start = start;
	m->final = final;
	return m;
}

// concatanates two machines
struct machine *concat( struct machine *a, struct machine *b )
{
	struct epsilon *cat;

	if( a == NULL || b == NULL )
		return NULL;

	// alloc stuff
	cat = zalloc( sizeof( struct epsilon ));

	// insert epsilon transition
	cat->st = b->start;
	cat->next = a->final->ep;
	a->final->ep = cat;

	// maintain linked list
	a->final->next = b->start;

	// use machine 'a' as our new machine and free 'b'
	a->final = b->final;
	free( b );
	return a;
}

// alternates two machines (spike)
struct machine *alternate( struct machine *a, struct machine *b )
{
	struct state *start, *final;
	struct epsilon *s1, *s2, *f1, *f2;

	if( a == NULL || b == NULL )
		return NULL;

	// alloc stuff
	start = zalloc( sizeof( struct state ));
	final = zalloc( sizeof( struct state ));
	s1 = zalloc( sizeof( struct epsilon ));
	s2 = zalloc( sizeof( struct epsilon ));
	f1 = zalloc( sizeof( struct epsilon ));
	f2 = zalloc( sizeof( struct epsilon ));

	// insert epsilon transitions from our new start state
	// to the start states of the two machines
	s1->st = a->start;
	s2->st = b->start;
	s1->next = s2;
	start->ep = s1;

	// insert epsilon transitions from the final states of the
	// new machines to our new final state
	f1->st = final;
	f2->st = final;
	f1->next = a->final->ep;
	a->final->ep = f1;
	f2->next = b->final->ep;
	b->final->ep = f2;

	// maintain linked list
	start->next = a->start;
	a->final->next = b->start;
	b->final->next = final;

	// use machine 'a' as our new machine and free 'b'
	a->start = start;
	a->final = final;
	free( b );
	return a;
}

// creates the closure of a machine (splat)
struct machine *closure( struct machine *a )
{
	struct state *start, *final;
	struct epsilon *s1, *s2, *f1, *f2;

	if( a == NULL )
		return NULL;

	// alloc stuff
	start = zalloc( sizeof( struct state ));
	final = zalloc( sizeof( struct state ));
	s1 = zalloc( sizeof( struct epsilon ));
	s2 = zalloc( sizeof( struct epsilon ));
	f1 = zalloc( sizeof( struct epsilon ));
	f2 = zalloc( sizeof( struct epsilon ));

	// insert epsilon transitions from our new start state to
	// the machine's start state and to our new final state
	s1->st = final;
	s2->st = a->start;
	s1->next = s2;
	start->ep = s1;

	// insert epsilon transitions from the machine's final state
	// to the machine's start state and our new final state
	f1->st = a->start;
	f2->st = final;
	f1->next = f2;
	f2->next = a->final->ep;
	a->final->ep = f1;

	// maintain linked list
	start->next = a->start;
	a->final->next = final;

	// use 'a' as our new machine
	a->start = start;
	a->final = final;
	return a;
}

void free_machine( struct machine *m );

// free each state in a linked list
void free_states( struct state *sl )
{
	struct trans *tr;
	struct epsilon *ep, *epn;
	struct state *next;
	struct attribute *at, *atn;

	for( ; sl != NULL; sl = next )
	{
		next = sl->next;

		// free each transition off this state
		tr = sl->tr;
		if( tr != NULL )
		{
			// free all the junk that can hang off of a transition
			free_machine( tr->ptr );
			free( tr->name );
			if( tr->re.re_magic != 0 )
				notbuiltin_regfree( &tr->re );
			for( at = tr->attrs; at != NULL; at = atn )
			{
				atn = at->next;
				free( at->name );
				if( at->re.re_magic != 0 )
					notbuiltin_regfree( &at->re );
				free( at );
			}
			free( tr );
		}

		// free each epsilon transition (easy)
		for( ep = sl->ep; ep != NULL; ep = epn )
		{
			epn = ep->next;
			free( ep );
		}

		// finally free the state
		free( sl );
	}
}

void free_machine( struct machine *m )
{
	int i;

	if( m == NULL )
		return;

	// free every state in the machine
	free_states( m->start );

	// if we have an E function free it
	if( m->E != NULL )
	{
		for( i = 0; i < m->states; i++ )
			free( m->E[i] );
		free( m->E );
	}

	// free bitmasks
	if( m->cur_state != NULL )
		free( m->cur_state );
	if( m->next_state != NULL )
		free( m->next_state );

	// finally free the machine
	free( m );
}

/*
 * Tokenizer
 */
struct token
{
	int t;
	char *name;
};

#define T_EOL			( -2 ) // not really paid attention to
#define T_ERROR			( -1 ) // tokenizing error, look at token->error and token->buf
#define T_SYMBOL		(  0 ) // symbol
#define T_SQUIGGLE		(  1 ) // ~
#define T_WAX			(  2 ) // (
#define T_WANE			(  3 ) // )
#define T_SPIKE			(  4 ) // |
#define T_SPLAT			(  5 ) // *
#define T_PTR			(  6 ) // ->
#define T_TWOSPOT		(  7 ) // :
#define T_ANGLE			(  8 ) // <
#define T_RIGHTANGLE	(  9 ) // >
#define T_HALFMESH		( 10 ) // =
#define T_STRING		( 11 ) // quoted string

// parse the next token in a string and return a pointer into the string were we stoped
const char *get_tok( const char *str, struct token *tk )
{
	static char *name;
	const char *p;
	int i, len = 0;

	if( str == NULL )
	{
		tk->t = T_ERROR;
		return NULL;
	}

	// skip whitespace
	for( p = str; *p != 0 && isspace( *p ); p++ );

	switch( *p )
	{
	case 0:
		tk->t = T_EOL;
		return p;
	case '~':
		tk->t = T_SQUIGGLE;
		return p + 1;
	case '(':
		tk->t = T_WAX;
		return p + 1;
	case ')':
		tk->t = T_WANE;
		return p + 1;
	case '|':
		tk->t = T_SPIKE;
		return p + 1;
	case '*':
		tk->t = T_SPLAT;
		return p + 1;
	case '-':
		if( p[1] == '>' )
		{
			tk->t = T_PTR;
			return p + 2;
		}
		tk->t = T_ERROR;
		return p;
	case ':':
		tk->t = T_TWOSPOT;
		return p + 1;
	case '<':
		tk->t = T_ANGLE;
		return p + 1;
	case '>':
		tk->t = T_RIGHTANGLE;
		return p + 1;
	case '=':
		tk->t = T_HALFMESH;
		return p + 1;
	case '\"':
		p++;
		len = 16;
		name = zalloc( len );
		for( i = 0; p[i] != 0 && p[i] != '\"'; i++ )
		{
			if( i + 1 >= len )
				name = realloc( name, len *= 2 );
			if( p[i] == '\\' && p[i + 1] == '\"' && i < 254 )
			{
				name[i] = p[i];
				i++;
			}
			name[i] = p[i];
		}
		name[i] = 0;
		tk->t = p[i] == 0 ? T_ERROR : T_STRING;
		tk->name = name;
		return p + i + ( p[i] == '\"' ? 1 : 0 );
	case '.':
		name = zalloc( 2 );
		name[0] = '.';
		name[1] = 0;
		tk->t = T_SYMBOL;
		tk->name = name;
		return p + 1;
	default:
		len = 16;
		name = zalloc( len );
		for( i = 0; isalnum( p[i] ) || p[i] == '_' || p[i] == '-'; i++ )
		{
			if( i + 1 >= len )
				name = realloc( name, len *= 2 );
			name[i] = p[i];
		}
		name[i] = 0;
		tk->t = i > 0 ? T_SYMBOL : T_ERROR;
		tk->name = name;
		return p + i;
	}
}

const char *parse_treexpr( const char *expr, struct machine **m );

// parses an attribute construction: <foo="bar" baz="quux" quuux>
// the opening angle has already been consumed
const char *parse_attrs( const char *expr, struct attribute **a )
{
	struct token tk;
	struct attribute *prev = NULL, *attr;
	const char *next;

	// build a list of attributes in order
	for( next = get_tok( expr, &tk ); tk.t == T_SYMBOL; )
	{
		// fist we grab the name
		attr = zalloc( sizeof( struct attribute ));
		if( prev == NULL )
			prev = *a = attr;
		else
			prev = prev->next = attr;
		attr->name = tk.name;

		// check for optional =
		next = get_tok( next, &tk );
		if( tk.t != T_HALFMESH )
			continue;

		// grab regex and compile it
		next = get_tok( next, &tk );
		if( tk.t != T_STRING )
			return NULL;
		if( notbuiltin_regcomp( &attr->re, tk.name, REG_EXTENDED
			| REG_ICASE ) != 0 )
		{
			notbuiltin_regfree( &attr->re );
			attr->re.re_magic = 0;
			return NULL;
		}
		free( tk.name );
		next = get_tok( next, &tk );
	}
	if( tk.t == T_RIGHTANGLE )
		return next;
	return NULL;
}

// parse a tree expression "factor"
// a factor is what I'm calling the operands to concatenation
// so it's either a symbol (with optional restrictions), a parenthasized expression
// or a factor with a *
// this does not allow foolishness like "foo**" (equivalent to "foo*") or "~*" (equivalent to "~")
// even though they are technically valid
const char *parse_factor( const char *expr, struct machine **m )
{
	struct token tk;
	struct machine *r;
	const char *next, *cur;

	// this token should either be a symbol, a ~, or a (
	cur = get_tok( expr, &tk );
	if( tk.t == T_ERROR )
	{
		*m = zalloc( sizeof( struct machine ));
		( *m )->error = "Tokenizing error";
		( *m )->buf = expr;
		return NULL;
	}
	if( tk.t == T_SYMBOL )
	{
		// build a machine to match the symbol
		*m = symbol( tk.name );
		next = get_tok( cur, &tk );
		if( tk.t == T_ERROR )
		{
			( *m )->error = "Tokenizing error";
			( *m )->buf = cur;
			return NULL;
		}

		// if there's a * build the closure
		if( tk.t == T_SPLAT )
		{
			*m = closure( *m );
			return next;
		}

		// if there's a -> then parse the expression on the rhs and add it as a restriction
		if( tk.t == T_PTR )
		{
			next = parse_treexpr( next, &r );
			( *m )->start->tr->ptr = r;
			if( next == NULL )
			{
				( *m )->error = r->error;
				( *m )->buf = r->buf;
				return NULL;
			}
			return next;
		}

		// if there's a : then compile the regex and add it as a restriction
		if( tk.t == T_TWOSPOT )
		{
			cur = next;
			next = get_tok( cur, &tk );
			if( tk.t == T_ERROR )
			{
				( *m )->error = "Tokenizing error";
				( *m )->buf = cur;
				return NULL;
			}
			if( tk.t != T_STRING )
			{
				( *m )->error = "Expecting a \"-delimited string";
				( *m )->buf = cur;
				return NULL;
			}
			if( notbuiltin_regcomp( &( *m )->start->tr->re, tk.name,
				REG_EXTENDED | REG_ICASE ) != 0 )
			{
				notbuiltin_regfree( &( *m )->start->tr->re );
				( *m )->start->tr->re.re_magic = 0;
				( *m )->error = "Error parsing regular expression";
				( *m )->buf = cur;
			}
			free( tk.name );
			return next;
		}

		// if there's a < then parse attributes and add it as a restriction
		if( tk.t == T_ANGLE )
		{
			next = parse_attrs( next, &( *m )->start->tr->attrs );
			if( next == NULL )
			{
				( *m )->error = "Expecting attribute list, ie <name=\"value\" name2=\"value2\">";
				( *m )->buf = cur;
				return NULL;
			}
			cur = next;

			// we also allow a -> restriction in this case
			next = get_tok( cur, &tk );
			if( tk.t == T_PTR )
			{
				next = parse_treexpr( next, &r );
				( *m )->start->tr->ptr = r;
				if( next == NULL )
				{
					( *m )->error = r->error;
					( *m )->buf = r->buf;
					return NULL;
				}
				return next;
			}
			return cur;
		}
		return cur;
	}
	if( tk.t == T_SQUIGGLE )
	{
		// build epsilon machine
		*m = epsilon( );
		return cur;
	}
	if( tk.t == T_WAX )
	{
		// parse expression
		cur = parse_treexpr( cur, m );
		if( cur == NULL )
			return NULL;

		// make sure we end in )
		next = get_tok( cur, &tk );
		if( tk.t == T_ERROR )
		{
			( *m )->error = "Tokenizing error";
			( *m )->buf = cur;
			return NULL;
		}
		if( tk.t != T_WANE )
		{
			( *m )->error = "Expected ')'";
			( *m )->buf = cur;
			return NULL;
		}

		// check for * and build closure
		cur = next;
		next = get_tok( cur, &tk );
		if( tk.t == T_ERROR )
		{
			( *m )->error = "Tokenizing error";
			( *m )->buf = cur;
			return NULL;
		}
		if( tk.t == T_SPLAT )
		{
			*m = closure( *m );
			return next;
		}
		return cur;
	}

	// invalid symbol
	*m = zalloc( sizeof( struct machine ));
	( *m )->error = "Expected symbol or '~' or '('";
	( *m )->buf = expr;
	return NULL;
}

// parse a tree expression "term"
// a term is what I'm calling the operands to alternation
// a term is either a factor, or a list of factors concatenated together
const char *parse_term( const char *expr, struct machine **m )
{
	struct token tk;
	struct machine *r;
	const char *cur;

	// grab the first factor
	cur = parse_factor( expr, m );
	if( cur == NULL )
		return NULL;

	// grab zero or more factors
	for( get_tok( cur, &tk ); tk.t == T_SYMBOL || tk.t == T_WAX || tk.t == T_SQUIGGLE;
		get_tok( cur, &tk ))
	{
		cur = parse_factor( cur, &r );
		if( cur == NULL )
		{
			( *m )->error = r->error;
			( *m )->buf = r->buf;
			free_machine( r );
			return NULL;
		}

		// concatenate the factors
		*m = concat( *m, r );
	}
	if( tk.t == T_ERROR )
	{
		( *m )->error = "Tokenizing error";
		( *m )->buf = cur;
		return NULL;
	}
	return cur;
}

// parse a tree expression
// an expression is just a term or a list of terms seperated by |
// this looks almost exactly like parse_term()
const char *parse_treexpr( const char *expr, struct machine **m )
{
	struct token tk;
	const char *next, *cur;
	struct machine *r;

	// grab the first term
	cur = parse_term( expr, m );
	if( cur == NULL )
		return NULL;

	// grab zero or more terms
	for( next = get_tok( cur, &tk ); tk.t == T_SPIKE; next = get_tok( cur, &tk ))
	{
		cur = parse_term( next, &r );
		if( cur == NULL )
		{
			( *m )->error = r->error;
			( *m )->buf = r->buf;
			free_machine( r );
			return NULL;
		}

		// combine the terms using alternation
		*m = alternate( *m, r );
	}
	if( tk.t == T_ERROR )
	{
		( *m )->error = "Tokenizing error";
		( *m )->buf = cur;
		return NULL;
	}
	return cur;
}


// process a regex restriction (basically just executes the regex)
int regex_process( struct trans *tr, char *content )
{
	if( content == NULL )
		return 0;
	if( notbuiltin_regexec( &tr->re, content, RESUBR, tr->match, 0 ) == 0 )
	{
		tr->str = content;
		return 1;
	}
	tr->str = NULL;
	return 0;
}

// process an attribute restriction
// it's important that we don't save the matches for all regexes until we know all of them
// match. otherwise if you were trying to match <foo="bar" bar="baz"> and you had a list like
// <foo="bar" bar="baz">   (both regexes match)
// <foo="barr" bar="quux"> (the first one matches and overwrites the previous match for foo)
// then you would be left with foo="barr" bar="baz" as your matches
int attrs_process( struct trans *tr, struct _xmlAttr *properties )
{
	struct attribute *attr;
	struct _xmlAttr *cur;
	regmatch_t match[RESUBR];

	// first pass makes sure each attribute matches
	for( attr = tr->attrs; attr != NULL; attr = attr->next )
	{
		for( cur = properties; cur != NULL; cur = cur->next )
		{
			if( strcasecmp( attr->name, (char *)cur->name ) == 0 )
			{
				// if there's no value it will only match if we didn't specify a regex
				if( cur->children == NULL )
				{
					if( attr->re.re_magic == 0 )
						break;
					return 0;
				}
				// otherwise the regex has to match the value
				if( notbuiltin_regexec( &attr->re,
					(char *)cur->children->content, RESUBR,
					match, 0 ) == 0 )
					break;
				else
					attr->str = NULL;
				return 0;
			}
		}
		if( cur == NULL )
			return 0;
	}
	// second pass saves the matches
	for( attr = tr->attrs; attr != NULL; attr = attr->next )
	{
		for( cur = properties; cur != NULL; cur = cur->next )
		{
			if( strcasecmp( attr->name, (char *)cur->name ) == 0 )
			{
				// if there's no value it will only match if we didn't specify a regex
				if( cur->children == NULL )
				{
					if( attr->re.re_magic == 0 )
						break;
					return 0;
				}
				// otherwise the regex has to match the value
				if( notbuiltin_regexec( &attr->re,
					(char *)cur->children->content, RESUBR,
					attr->match, 0 ) == 0 )
				{
					attr->str = (char *)cur->children->content;
					break;
				}
				else
					attr->str = NULL;
				return 0;
			}
		}
		if( cur == NULL )
			return 0;
	}
	return 1;
}

// some handy macros:
// number of bits in type pointed to by x
#define B( x )				(sizeof(*(x))*8)

// number of elements in array pointed to by x in order to contain n bits
#define N( x, n )			(((n)+B(x)-1)/B(x))

// test bit b of array pointed to by x
#define TEST_BIT( x, b )	(((x)[(b)/B(x)]>>((b)%B(x)))&1)

// set bit b of array pointed to by x
#define SET_BIT( x, b )		((x)[(b)/B(x)]|=1<<((b)%B(x)))

// compute the disjunction of two bitfields of n bits
#define OR( x, y, n )		{unsigned int _i;for(_i=0;_i<N(x,n);(x)[_i]|=(y)[_i],_i++);}

// applies a machine to an xml tree
// returns true iff the machine accepts
// all matches to regexes are contained within the machine
int tree_process( struct machine *m, xmlNodePtr node )
{
	int *e, done;
	struct state *cur, *cur2;
	struct trans *tr;
	struct epsilon *ep;

	if( m == NULL )
		return 1;
	if( m->E == NULL )
	{
		// Generate E function
		// the E function is an array of bitmasks indexed by state number
		// the bitmask represents the states you can reach by epsilon transitions

		// number states
		m->states = 0;
		for( cur = m->start; cur != NULL; cur = cur->next )
			cur->num = m->states++;

		// fill E
		m->E = zalloc( sizeof( *m->E ) * m->states );
		for( cur = m->start; cur != NULL; cur = cur->next )
		{
			e = m->E[cur->num] = zalloc( N( *m->E, m->states ) * sizeof( **m->E ));

			// we can reach ourself
			SET_BIT( e, cur->num );

			// try to add new states until an iteration of this loop makes no progress
			done = 0;
			while( !done )
			{
				done = 1;
				// for each state that is already in the bitmask, add states you can reach
				// by epsilon transitions
				for( cur2 = m->start; cur2 != NULL; cur2 = cur2->next )
					if( TEST_BIT( e, cur2->num ))
						for( ep = cur2->ep; ep != NULL; ep = ep->next )
							if( !TEST_BIT( e, ep->st->num ))
							{
								SET_BIT( e, ep->st->num );
								done = 0;
							}
			}
		}
	}

	// allocate current state and next state bitmasks
	if( m->cur_state == NULL )
		m->cur_state = zalloc( N( m->cur_state, m->states ) * sizeof( *m->cur_state ));
	if( m->next_state == NULL )
		m->next_state = zalloc( N( m->next_state, m->states ) * sizeof( *m->next_state ));

	// our inital current state is E(start)
	OR( m->cur_state, m->E[m->start->num], m->states );

	// main loop, terminate when we run out of input or when there's no states in the cur_state bitmask
	while( node != NULL )
	{
		unsigned int i;
		int sum = 0;

		// are we still alive?
		for( i = 0; i < N( m->cur_state, m->states ); i++ )
			sum |= m->cur_state[i];
		if( sum == 0 ) break;

		// zero out next_state and start adding states we can reach by normal transitions
		memset( m->next_state, 0, N( m->next_state, m->states ) * sizeof( *m->next_state ));

		// for each state in the cur_state bitmask we try a transition
		for( cur = m->start; cur != NULL; cur = cur->next )
			if( TEST_BIT( m->cur_state, cur->num ))
			{
				tr = cur->tr;
				if( tr != NULL )
				{
					// first we must match the name and attributes
					if( strcmp( tr->name, "." ) != 0 &&
						( node->name == NULL || strcasecmp( tr->name, (char *)node->name ) != 0 ))
						continue;
					if( tr->attrs != NULL && !attrs_process( tr, node->properties ))
						continue;
					// second we can match a machine and regexp
					if( tr->ptr != NULL && !tree_process( tr->ptr, node->children ))
						continue;
					if( tr->re.re_magic != 0 && !regex_process( tr, (char *)node->content ))
						continue;
					// we have a winner! add E(st) to the next state bitmap
					OR( m->next_state, m->E[tr->st->num], m->states );
				}
			}
		// advance input
		node = node->next;

		// swap cur_state and next_state pointers, saves us a copy operation
		e = m->cur_state;
		m->cur_state = m->next_state;
		m->next_state = e;
	}

	// the machine accepts the input if we end up in the final state
	done = TEST_BIT( m->cur_state, m->final->num );
	return done;
}

// extracts regex matches from a machine after it has been run
struct regex_match *find_matches( struct state *s, struct regex_match *m )
{
	struct regex_match *cur, *head;
	struct trans *tr;
	struct attribute *attr;
	int i;

	if( s == NULL )
		return m;

	// recurse to next state first, so we can add the matches in the current state to the
	// front of the list
	m = find_matches( s->next, m );

	// grab matches in this state
	tr = s->tr;
	if( tr != NULL )
	{
		// the -> comes after <foo> in the expression, so search it first
		if( tr->ptr != NULL )
			m = find_matches( tr->ptr->start, m );

		// next search : "foo"
		if( tr->str != NULL )
		{
			// loop backwards so that list builds forwards
			for( i = RESUBR - 1; i > 0; i-- )
				if( tr->match[i].rm_eo != -1 )
				{
					cur = zalloc( sizeof( struct regex_match ));
					cur->match = tr->match[i];
					cur->str = tr->str;
					cur->next = m;
					m = cur;
				}
		}

		// last search <foo>
		// loop forwards because we have no choice, but build list backwards
		head = NULL;
		cur = NULL;
		for( attr = tr->attrs; attr != NULL; attr = attr->next )
		{
			if( attr->str == NULL )
				continue;
			for( i = 1; i < RESUBR; i++ )
				if( attr->match[i].rm_eo != -1 )
				{
					if( head == NULL )
						cur = head = zalloc( sizeof( struct regex_match ));
					else
						cur = cur->next = zalloc( sizeof( struct regex_match ));
					cur->match = attr->match[i];
					cur->str = attr->str;
					cur->next = NULL;
				}
		}
		if( head != NULL )
		{
			cur->next = m;
			m = head;
		}
	}
	return m;
}

// runs machine m on each xml node at this level, then recurses to it's children, building a
// list of matches
struct match *node_recurse( struct machine *m, xmlNodePtr node, struct match *n )
{
	xmlNodePtr cur, next;
	struct match *xml;

	if( node == NULL )
		return n;
	for( cur = node; cur != NULL; cur = cur->next )
	{
		// this will consider each node by itself (without siblings)
		next = cur->next;
		cur->next = NULL;
		if( tree_process( m, cur ))
		{
			xml = zalloc( sizeof( struct match ));
			xml->next = n;
			xml->node = cur;
			xml->re = find_matches( m->start, NULL );
			n = xml;
		}
		cur->next = next;
		// recurse to children
		n = node_recurse( m, cur->children, n );
	}
	return n;
}

// run a machine on each node in an xml document and return a list of matches
struct match *document_process( struct machine *m, xmlDocPtr doc )
{
	return node_recurse( m, doc->children->next, NULL );
}

// free matches returned by document_process
void free_matches( struct match *z )
{
	struct match *nextz;
	struct regex_match *nextre;

	for( ; z != NULL; z = nextz )
	{
		nextz = z->next;
		for( ; z->re != NULL; z->re = nextre )
		{
			nextre = z->re->next;
			free( z->re );
		}
		free( z );
	}
}
