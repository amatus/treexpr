/* treexpr.h - Tree expression language header
 + Copyright (C) 2005 David Barksdale
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

#ifndef _TREEXPR_H_
#define _TREEXPR_H_

#include <libxml/tree.h>
#include <sys/types.h>
#include "regex.h"

/*
 * State machines
 */

struct state;

struct epsilon
{
	struct epsilon *next;
	struct state *st;
};

#define RESUBR	( 10 )

struct attribute
{
	struct attribute *next;
	char *name; // name of attribute to match
	regex_t re; // compiled regular expression to match
	regmatch_t match[RESUBR]; // matches
	char *str; // string containing matches
};

struct trans
{
	struct state *st;	// state we transition to upon match
	// stuff to match
	char *name;			// name of tag to match against
	regex_t re;			// compiled regular expression to match against contents
	regmatch_t match[RESUBR]; // matches
	char *str;			// string containing matches
	struct attribute *attrs; // attributes to match against
	struct machine *ptr; // machine to match children
};

struct state
{
	struct trans *tr; // optional transition
	struct epsilon *ep;	// list of epsilon transitions
	int num; // state number (generated at run time)
	// graph traversal
	struct state *next; // internal list of states for a machine
};

struct machine
{
	struct state *start; // start state
	struct state *final; // final state (yes, only one)
	// parse errors
	const char *error, *buf;
	// execution
	int states; // number of states
	int **E; // arrays of bit masks for E function
	// I moved these here to make repeated execution a little faster
	// we alloc these buffers on the first execution and then reuse them
	int *cur_state; 
	int *next_state;
};

/* Matches */

struct regex_match
{
	struct regex_match *next;
	regmatch_t match; // match
	char *str; // string containing match
};

struct match
{
	struct match *next;
	xmlNodePtr node; // root node of tree match
	struct regex_match *re; // list of regular expression matches
};

/* Public functions */

const char *parse_treexpr( const char *expr, struct machine **m );
void free_machine( struct machine *m );
struct match *document_process( struct machine *m, xmlDocPtr doc );
void free_matches( struct match *z );

#endif
