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
/* $Id: cycle_detector.c,v 1.6 2011/06/12 18:22:01 bediger Exp $ */

/*
 * Code to detect a reduction cycle that occurs
 * during evaluation of CL expressions like
 * C W (C W) (C W)
 *
 * It just converts a graph (struct node * binary tree)
 * to a string via canonicalize_graph(), and keeps the
 * strings around for comparison to any later graphs.
 */

#include <stdio.h>
#include <setjmp.h>  /* sigjmp_buf, siglongjmp() */
#include <stdlib.h>  /* realloc() */
#include <string.h>  /* strcmp() */

#include <node.h>
#include <buffer.h>
#include <graph.h>
#include <cycle_detector.h>

extern sigjmp_buf in_reduce_graph;

static char **cycle_stack = NULL;
static int    cycle_stack_depth = 0;
static int    cycle_stack_size = 0;

void canonicalize(struct node *node, struct buffer *b);

void
free_detection(void)
{
	reset_detection();
	if (cycle_stack) free(cycle_stack);
	cycle_stack = NULL;
}

void
reset_detection(void)
{
	while (cycle_stack_depth)
		free(cycle_stack[--cycle_stack_depth]);
}

int
cycle_detector(struct node *root, int max_redex_count)
{
	char *graph = NULL;
	int i;
	int detected_cycle = 0;

	graph = canonicalize_graph(root->left);

	for (i = cycle_stack_depth; i > 0; --i)
	{
		if (!strcmp(graph, cycle_stack[i-1]))
		{
			/* Gotta say that this is a *very* low level routine in which
			 * to perform output.  Unfortunately, unless I pass a whole
			 * bunch of flags back up the call-chain, I can't communicate
			 * all the information I'd like to.  So despite the ugliness,
			 * the output stays here. */
			printf("Found a %scycle of length %d, %d terms evaluated, ends with \"%s\"\n",
				(max_redex_count == 1)? "pure ": "", 
				(cycle_stack_depth - i + 1),
				cycle_stack_depth,
				graph
			);
			free(graph);
			graph = NULL;
			reset_detection();
			detected_cycle = 1;
			break;
		}
	}

	if (i == 0 && !detected_cycle )
	{
		if (cycle_stack_depth == cycle_stack_size)
		{
			int new_size = cycle_stack_size + 10;
			char **new_array = realloc(cycle_stack, new_size*sizeof(cycle_stack[0]));

			cycle_stack = new_array;
			cycle_stack_size = new_size;
		}

		cycle_stack[cycle_stack_depth++] = graph;
	}

	return detected_cycle;
}

/* Convert a graph ("parse tree") into a canonical
 * string.  Each application appears as a '.', and
 * a space (' ') appears between combinator identifiers.
 */
char *
canonicalize_graph(struct node *node)
{
	struct buffer *b;
	char *s;

	b = new_buffer(256);

	canonicalize(node, b);

	s = b->buffer;
	s[b->offset] = '\0';
	b->buffer = NULL;

	delete_buffer(b);

	return s;
}

/* The actual work of canonical representation. */
void
canonicalize(struct node *node, struct buffer *b)
{
	switch (node->typ)
	{
	case APPLICATION:
		buffer_append(b, ".", 1);
		canonicalize(node->left, b);
		if (ATOM == node->right->typ)
			buffer_append(b, " ", 1);
		canonicalize(node->right, b);
		break;
	case ATOM:
		buffer_append(b, node->name, strlen(node->name));
		break;
	}
}
