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

/* $Id: spine_stack.c,v 1.4 2010/08/10 20:50:39 bediger Exp $ */

#include <stdlib.h>
#include <stdio.h>

#include <spine_stack.h>
#include <node.h>

/* Only a single spine stack instance gets created
 * during any run of the interpreter.  That single
 * instance gets reused to evaluate the second and
 * later expressions evaluated. */
struct spine_stack *old_spine_stack = NULL;
static int spine_stack_resizes = 0;

extern int interpreter_interrupted;

struct spine_stack *
new_spine_stack(int sz)
{
	struct spine_stack *r;

	if (old_spine_stack)
	{
		r = old_spine_stack;
		r->top   = 0;
		/* Don't NULL out old_spine_stack: if someone control-c's
		 * the interpreter during a reduction, it might miss putting
		 * the spine stack back in place. */
	} else {
		r = malloc(sizeof(*r));
		r->stack = malloc(sz * sizeof(struct spine_stack_element));
		r->size = sz;
		r->maxdepth = 0;
		r->top   = 0;
	}

	return r;
}

void
delete_spine_stack(struct spine_stack *ss)
{
	old_spine_stack = ss;
}


/*
 * Push a struct node pointer onto the spine stack,
 * re-sizing the stack if necessary, since the "stack"
 * part of struct spine_stack is an array.
 *
 * The "re-sizing" part actually doubles the usuable
 * stack each time it gets executed.  May cause trouble
 * for reduction of pathological CL expressions.
 *
 * The "mark" argument allows us to push the RHS of
 * an application on the stack, and use the corresponding
 * depth element of struct spine_stack_element as the
 * "depth" of the RHS of the graph.
 */
void
pushnode(struct spine_stack *ss, struct node *n, int mark)
{
	ss->stack[ss->top].node = n;

	ss->stack[ss->top].depth = (mark? mark: ss->stack[ss->top-1].depth + 1);

	++ss->top;

	if (ss->top > ss->maxdepth) ++ss->maxdepth;

	if (ss->top >= ss->size)
	{
		/* resize the allocation pointed to by stack */
		struct spine_stack_element *old_stack = ss->stack;
		size_t new_size = ss->size * 2;  /* XXX !!! */
		++spine_stack_resizes;
		ss->stack = realloc(old_stack, sizeof(struct spine_stack_element)*new_size);
		if (!ss->stack)
			ss->stack = old_stack;  /* realloc failed */
		else
			ss->size = new_size;
	}
}

void
free_all_spine_stacks(int memory_info_flag)
{
	if (memory_info_flag)
	{
		fprintf(stderr, "Resized spine stack %d times\n", spine_stack_resizes);
		if (old_spine_stack)
		{
			fprintf(stderr, "Spine stack size %d elements\n", old_spine_stack->size);
			fprintf(stderr, "Spine stack reached max depth of %d\n", old_spine_stack->maxdepth);
		}
	}

	if (old_spine_stack)
	{
		free(old_spine_stack->stack);
		old_spine_stack->stack = NULL;
		old_spine_stack->top = 0;
		old_spine_stack->size = 0;
		free(old_spine_stack);
		old_spine_stack = NULL;
	}
}
