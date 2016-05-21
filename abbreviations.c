/*
	Copyright (C) 2010, Bruce Ediger

    This file is part of acl.

    acl is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    acl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with acl; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/


/* $Id: abbreviations.c,v 1.6 2010/08/10 20:50:39 bediger Exp $ */

/*
 * Abbreviations: an identifier can reference an entire parse tree
 * if the user has issued the "define" or "def" interpreter commands.
 *
 * During parse of user input, all identifiers get passed through
 * abbreviation_lookup().  Previously issued "def" or "define" commands
 * cause the single struct hashtable to keep a copy of a parse tree 
 * with the identifier as the key.
 *
 * Some pieces of this code assume that the definitions and the "atom"
 * strings used in the interpreter share a single struct hashtable for
 * storage.
 */


#include <stdio.h>
#include <stdlib.h>  /* malloc(), free() */

#include <node.h>
#include <hashtable.h>
#include <abbreviations.h>

struct hashtable *abbr_table = NULL;

void
setup_abbreviation_table(struct hashtable *h)
{
	abbr_table = h;
}

struct node *
abbreviation_lookup(const char *id)
{
	struct node *r = NULL;
	void *p = data_lookup(abbr_table, id);
	/* make an arena-allocated copy of the abbreviation, so that
	 * reduce_graph() can destructively reduce the resulting
	 * parse tree. */
	if (p) r = arena_copy_graph((struct node *)p);
	return r;
}

void
abbreviation_add(const char *id, struct node *expr)
{
	struct hashnode *n = NULL;
	unsigned int hv;

	/* By the time flow-of-control arrives here,
	 * the string (variable id) has already gotten
	 * in the hashtable. Only 1 struct hashtable exists,
	 * used for both unique-string-saving, and storage
	 * of "abbreviations". The lexer code guarantess
	 * it by calling Atom_string() on all input strings.
	 * Therefore, this code *always* finds string id
	 * in the abbr_table.
	 */
	n = node_lookup(abbr_table, id, &hv);

	free_graph((struct node *)n->data);

	/* Make a "permanent" copy of the parse tree, not arena.
	 * allocated.  At the end of a read-eval-print loop, the
	 * interpreter resets all arenas to "nothing allocated",
	 * so an arena-allocated copy would eventually get overwritten.
	 */
	n->data = (void *)copy_graph(expr);
}

/*
 * malloc() based parse tree ("graph") copy.
 * If a "reduce" interpreter command causes the input graph
 * to have multiply-referenced structs node, this function
 * ignores that.  Sub-trees that are "shared" get un-shared.
 */
struct node *
copy_graph(struct node *p)
{
	struct node *r = NULL;

	r = malloc(sizeof(*r));
	r->typ = p->typ;
	r->name = p->name;
	r->sn = -666;

	switch (p->typ)
	{
	case APPLICATION:
		r->left = copy_graph(p->left);
		r->right = copy_graph(p->right);
		break;
	case ATOM:
		r->rule = p->rule;
		r->left = r->right = NULL;
		break;
	}
	return r;
}

/*
 * free() based parse tree ("graph") deallocation.
 * Won't work on arena-based graphs generated from
 * user input.
 */
void
free_graph(struct node *p)
{
	if (!p) return;

	free_graph(p->left);
	free_graph(p->right);

	p->name = NULL;
	p->left = p->right = NULL;
	p->typ = -1;

	free(p);
}
