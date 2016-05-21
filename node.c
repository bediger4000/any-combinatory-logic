/*
	Copyright (C) 2010-2011, Bruce Ediger

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

/* $Id: node.c,v 1.13 2011/06/12 18:22:01 bediger Exp $ */

/*
 * struct node: main data structure of this interpreter.
 * The parser (in file grammar.y) builds up a binary tree of structs node,
 * calling it a "parse tree".  Since we write CL in a left-associative
 * way, each application instance of struct node has 2 child structs node,
 * a function (left) and an argument (right).  It's a big ol' binary tree.
 *
 * The parse tree also gets used in "graph reduction", where the collected
 * structure of structs node gets referred to as a "graph". Function
 * reduce_graph() destructively rearranges its input parse tree into the
 * normal form of that expression.
 *
 * This module uses code in arena.c to do "slab" allocation of memory for
 * structs node, but it also keeps a free list, and does reference counting
 * on structs node passed to free_node().  At the end of a read-eval-print
 * loop, all arenas get set to "zero allocated", but during a reduction,
 * the free list (struct node *node_free_list) gets used to avoid having
 * to fill out the updateable, left_addr and right_addr fields of struct
 * node on every allocation.
 *
 * Includes similar data struct, struct abs_node, created in parsing of
 * user-input bracket abstractin rules.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <node.h>
#include <arena.h>
#include <hashtable.h>
#include <atom.h>

extern int elaborate_output;
extern int debug_reduction;

static struct memory_arena *arena = NULL;

/* sn_counter - give a serial number (sn field) to
 * all nodes, so as to distinguish them in elaborate output.
 * Note that 0 constitutes a special value. */
static int sn_counter = 0;
static int reused_node_count = 0;
static int allocated_node_count = 0;  /* Not total. In a particular arena. */
static int new_node_cnt;

extern int interpreter_interrupted;
extern int reduction_interrupted;

static struct node *node_free_list = NULL;

/* actual centralized allocation, used by new_term(),
 * new_application(). */
struct node *new_node(void);

struct node *
new_application(struct node *left_child, struct node *right_child)
{
	struct node *r = new_node();

	r->typ = APPLICATION;
	r->name = Atom_string("@");  /* algorithm_d() uses this value */
	r->right = right_child;
	r->left  = left_child;

	if (r->right)
		++r->right->refcnt;
	if (r->left)
		++r->left->refcnt;

	return r;
}

struct node *
new_term(const char *name)
{
	struct node *r = new_node();

	r->typ = ATOM;
	r->name = name;

	return r;
}

void
print_tree(struct node *node, int reduction_node_sn, int current_node_sn)
{
	switch (node->typ)
	{
	case APPLICATION:

		if (!node->left && !node->right) return;

		print_tree(node->left, reduction_node_sn, current_node_sn);
		
		if (elaborate_output)
		{
			printf(" {%d}", node->sn);
			if (node->sn == current_node_sn)
				printf("+ ");
			else
				putc(' ', stdout);
		} else {
			if (node->sn == current_node_sn)
				printf(" + ");
			else
				putc(' ', stdout);
		}

		if (node->right)
		{
			int print_right_paren = 0;
			if (APPLICATION == node->right->typ)
			{
				putc('(', stdout);
				print_right_paren = 1;
			}
			print_tree(node->right, reduction_node_sn, current_node_sn);
			if (print_right_paren)
				putc(')', stdout);
		} else if (elaborate_output)
			printf(" {%d}", node->sn);

		break;
	case ATOM:
		if (elaborate_output)
			printf("%s{%d}", node->name, node->sn);
		else
			printf(
				(node->sn != reduction_node_sn)? "%s": "%s*",
				node->name
			);
		if (node->sn == current_node_sn)
			putc('+', stdout);
		break;
	}
}

struct node *
new_node(void)
{
	struct node *r = NULL;

	++new_node_cnt;

	if (node_free_list)
	{
		r = node_free_list;
		node_free_list = node_free_list->right;
		++reused_node_count;
	} else {
		r = arena_alloc(arena, sizeof(*r));
		++sn_counter;
		++allocated_node_count;
		r->sn = sn_counter;
		r->right_addr = &(r->right);
		r->left_addr = &(r->left);
	}

	/* r->sn stays unchanged throughout */
	r->right = r->left = NULL;
	r->name = NULL;
	r->updateable = NULL;
	r->refcnt = 0;
	r->tree_size = 0;

	return r;
}

void
free_all_nodes(void)
{
	deallocate_arena(arena);
}

void
init_node_allocation(void)
{
	arena = new_arena();
}

void
reset_node_allocation(void)
{
	if (!reduction_interrupted)
	{
		int free_list_cnt = 0;
		struct node *p = node_free_list;

		while (p)
		{
			++free_list_cnt;
			if (debug_reduction)
				fprintf(stderr, "Node %d, ref cnt %d on free list\n",
					p->sn, p->refcnt);
			p = p->right;
			if (free_list_cnt > allocated_node_count) break;
		}

		if (free_list_cnt != allocated_node_count)
			fprintf(stderr, "Allocated %d nodes, but found %s %d on free list\n",
				allocated_node_count,
				free_list_cnt >allocated_node_count? "at least": "only",
				free_list_cnt);
	}

	node_free_list = 0;
	allocated_node_count = 0;

	free_arena_contents(arena);
}

struct node *
arena_copy_graph(struct node *p)
{
	struct node *r = NULL;

	if (!p)
		return r;

	r = new_node();

	r->typ = p->typ;
	r->name = p->name;
	r->rule = p->rule;

	if (p->typ == APPLICATION)
	{
		r->left = arena_copy_graph(p->left);
		++r->left->refcnt;
		r->right = arena_copy_graph(p->right);
		++r->right->refcnt;
	}
	return r;
}

void
free_node(struct node *node)
{
	if (NULL == node) return;  /* dummy root nodes have NULL right field */

	if (debug_reduction)
		fprintf(stderr, "Freeing node %d, ref cnt %d\n",
			node->sn, node->refcnt);

	--node->refcnt;

	if (node->refcnt == 0)
	{
		if (APPLICATION == node->typ)
		{
			free_node(node->left);
			free_node(node->right);
		}
		node->right = node_free_list;
		node_free_list = node;
	} else if (0 > node->refcnt)
		fprintf(stderr, "Freeing node %d, negative ref cnt %d\n",
			node->sn, node->refcnt);
}

void
print_abs_node(struct abs_node *n)
{
	int print_right_paren = 0;
	switch (n->typ)
	{
	case abs_APPLICATION:
		if (n->abstracted)
		{
			putc('(', stdout);
			++print_right_paren;
			printf("[_] ");
		}
		print_abs_node(n->left);
		putc(' ', stdout);
		if (abs_APPLICATION == n->right->typ)
		{
			putc('(', stdout);
			++print_right_paren;
		}
		print_abs_node(n->right);
		while (print_right_paren--)
			putc(')', stdout);

		break;
	case abs_LEAF:
		if (n->abstracted)
			printf("([_] ");
		printf("%s", n->label);
		if (n->abstracted)
			printf(")");
		break;
	}
}

struct abs_node *
new_abs_node(const char *label)
{
	struct abs_node *r = malloc(sizeof(*r));
	r->left = r->right = NULL;
	r->typ = abs_LEAF;
	r->label = label;
	r->abstracted = 0;
	return r;
}

struct abs_node *
new_abs_application(struct abs_node *lft, struct abs_node *rght)
{
	struct abs_node *r = new_abs_node(Atom_string("@"));
	r->typ = abs_APPLICATION;
	r->left = lft;
	r->right = rght;
	return r;
}


/* Return true if a specific (var_name) variable occurs
 * in this node or any under it. */
int
var_in_tree(struct node *node, const char *var_name)
{
	int r = 0;

	switch (node->typ)
	{
	case APPLICATION:
		r = var_in_tree(node->left, var_name)
			|| var_in_tree(node->right, var_name);
		break;
	case ATOM:
		r = (node->name == var_name)? 1: 0;
		break;
	}

	return r;
}

/* Return true if any variable (non-primitive, as specified by rule:
 * interpreter command) appears in this node or any underneath it. */
int
any_var_in_tree(struct node *node)
{
	int r = 0;

	switch (node->typ)
	{
	case APPLICATION:
		r = any_var_in_tree(node->left)
			|| any_var_in_tree(node->right);
		break;
	case ATOM:
		/* Just like in graph.c, if node->rule contains non-NULL,
		 * this node counts as a primitive. */
		r = !node->rule;
		break;
	}

	return r;
}

void
free_abs_node(struct abs_node *tree)
{
	if (tree)
	{
		free_abs_node(tree->left);
		free_abs_node(tree->right);
		free(tree);
	}
}

void
preallocate_nodes(int pre_node_count)
{
	size_t sz = pre_node_count * sizeof(struct node);
	struct node *node_ary = arena_alloc(arena, sz);
	int i;

	allocated_node_count += pre_node_count;

	for (i = 0; i < pre_node_count; ++i)
	{
		struct node *n = &node_ary[i];
		n->sn = ++sn_counter;
		n->right_addr = &(n->right);
		n->left_addr = &(n->left);
		n->right = &node_ary[i+1];
		n->tree_size = 0;
	}
	node_ary[pre_node_count - 1].right = node_free_list;
	node_free_list = &node_ary[0];
}
