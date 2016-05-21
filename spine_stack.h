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

/* $Id: spine_stack.h,v 1.3 2010/07/19 17:55:40 bediger Exp $ */

struct spine_stack_element {
	struct node *node;
	int depth;
};

struct spine_stack {
	struct spine_stack_element *stack;
	int top;
	int size;
	int maxdepth;
};

struct spine_stack *new_spine_stack(int sz);
void pushnode(struct spine_stack *ss, struct node *n, int mark);
void delete_spine_stack(struct spine_stack *ss);
void free_all_spine_stacks(int memory_info_flag);

#define TOPNODE(ss) ((ss)->stack[(ss)->top - 1].node)
#define DEPTH(ss) ((ss)->stack[(ss)->top - 1].depth)
#define PARENTNODE(ss, N) ((ss)->stack[((ss)->top)-1-N].node)
#define POP(ss, N)  (((ss)->top)-=N)
#define STACK_NOT_EMPTY(ss) ((ss)->top > 0)
