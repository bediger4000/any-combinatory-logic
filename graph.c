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
/* $Id: graph.c,v 1.12 2011/06/12 18:22:01 bediger Exp $ */

#include <stdio.h>
#include <stdlib.h>  /* malloc() and free() */
#include <assert.h>
#include <string.h>
#include <setjmp.h>   /* longjmp(), jmp_buf */

#include <node.h>
#include <buffer.h>
#include <graph.h>
#include <spine_stack.h>
#include <cycle_detector.h>
#include <reduction_rule.h>

int read_line(void);

extern int multiple_reduction_detection;
extern int cycle_detection;
extern int trace_reduction;
extern int debug_reduction;
extern int elaborate_output;
extern int single_step;

extern int max_reduction_count;

extern sigjmp_buf in_reduce_graph;

#define C if(cycle_detection)
#define D if(debug_reduction)
#define T if(trace_reduction)
#define NT if(debug_reduction && !trace_reduction)

/* can't do single_step && read_line() - compilers optimize it away */
#define SS if (single_step) read_line()


void
print_graph(struct node *node, int sn_to_reduce, int current_sn)
{
	print_tree(node, sn_to_reduce, current_sn);
	putc('\n', stdout);
}

/* Graph reduction function. Destructively modifies the graph passed in.
 */
enum graphReductionResult
reduce_graph(struct node *root)
{
	enum graphReductionResult r = UNKNOWN;

	struct spine_stack *stack = NULL;

	unsigned long reduction_counter = 0;
	int max_redex_count = 0;

	/* Used to decide what to do next:
	 * at an application (interior node of graph)
	 * you can take the left branch into subtree,
	 * take the right branch, or pop the node and
	 * go "up" the tree. */
	enum Direction { DIR_LEFT, DIR_RIGHT, DIR_UP };
	enum Direction dir = DIR_LEFT;

	stack = new_spine_stack(64);

	/* root constitutes the "dummy" root node */
	root->updateable = root->left_addr;
	pushnode(stack, root, 1);

	D print_graph(root, 0, TOPNODE(stack)->sn);

	while (STACK_NOT_EMPTY(stack))
	{
		int pop_stack_cnt = 1;
		int performed_reduction = 0;
		struct node *topnode = TOPNODE(stack);
		const char *atom_name = NULL;

		switch (topnode->typ)
		{
		case APPLICATION:
			switch (dir)
			{
			case DIR_LEFT:
				topnode->updateable = topnode->left_addr;
				pushnode(stack, topnode->left, 0);
				D printf("push left branch on stack, depth now %d\n", DEPTH(stack));
				pop_stack_cnt = 0;
				break;

			case DIR_RIGHT:
				topnode->updateable = topnode->right_addr;
				pushnode(stack, topnode->right, 2);
				D printf("push right branch on stack, depth now %d\n", DEPTH(stack));
				pop_stack_cnt = 0;
				break;

			case DIR_UP:
				break;
			}
			break;

		case ATOM:
			/* node->typ indicates a combinator, which can comprise a built-in,
			 * or it can comprise a mere variable. Let node->rule decide. */
			if (topnode->rule && DEPTH(stack) >= (topnode->rule->required_depth + 2))
			{
				D {
					atom_name = topnode->name;
					printf("%s reduction (sn %d), stack depth %d, before: ",
						topnode->name,
						topnode->sn,
					 	DEPTH(stack)
					);
					print_graph(root->left, topnode->sn, topnode->sn);
				}

				pop_stack_cnt = topnode->rule->required_depth + 1;

				perform_reduction(stack);

				performed_reduction = 1;

			} else D {
				printf("%s atom, stack depth %d, required depth %d.\n",
					topnode->name,
				 	DEPTH(stack),
					topnode->rule? topnode->rule->required_depth + 2: -1
				);
			}
			if (performed_reduction) SS;
			break;
		}

		D printf("pop stack %d items\n", pop_stack_cnt);
		POP(stack, pop_stack_cnt);
		D {
			printf("%sperformed reduction, popped %d, stack depth now %d: ",
				performed_reduction? "": "didn't ", pop_stack_cnt, DEPTH(stack)
			);
			print_graph(root, 0, TOPNODE(stack)->sn);
			printf("direction %s\n", dir == DIR_LEFT? "left": dir == DIR_RIGHT? "right": "up");
		}

		/* Decide what to do next. Note that top-of-stack is the
		 * node we just popped to get to.
		 */
		topnode = TOPNODE(stack);

		if (performed_reduction)
		{
			if (topnode->updateable == topnode->left_addr)
				dir = DIR_LEFT;
			else
				dir = DIR_RIGHT;

		} else {
			if (pop_stack_cnt)
			{
				dir = DIR_UP;
				/* Ugly special case: popped up to root of tree */
				if (topnode == root)
					POP(stack, 1); /* empty out the stack */
				else {
					if (topnode->updateable == topnode->left_addr)
						dir = DIR_RIGHT;
					else if (topnode->updateable == topnode->right_addr)
						dir = DIR_UP;
					/* This assumes that topnode->updateable always has
					 * topnode->left_addr or topnode->right_addr in it. */
				}
			} else
				dir = DIR_LEFT;
		}

		D printf("direction now %s\n", dir == DIR_LEFT? "left": dir == DIR_RIGHT? "right": "up");

		if (performed_reduction)
		{
			++reduction_counter;

			D {
				printf("%s reduction, after: ", atom_name);
				print_graph(root->left, 0, topnode->sn);
			}

			if (multiple_reduction_detection)
			{
				if (trace_reduction)
				{
					struct buffer *b = new_buffer(256);
					int ignore, redex_count = reduction_count(root->left, 0, &ignore, b);  /* root: a dummy node */
					if (redex_count > max_redex_count) max_redex_count = redex_count;
					b->buffer[b->offset] = '\0';
					printf("[%d] %s\n", redex_count, b->buffer);
					delete_buffer(b);
				}
			} else
				T print_graph(root->left, 0, 0);

			if (cycle_detection && cycle_detector(root, max_redex_count))
			{
				r = CYCLE_DETECTED;
				goto exceptional_exit;
			}

			if (max_reduction_count > 0
				&& reduction_counter > max_reduction_count)
			{
				C reset_detection();
				r = REDUCTION_LIMIT;
				goto exceptional_exit;
			}
		}
	}

	r = NORMAL_FORM;

	/* reaching reduction limit or finding a cycle */
	exceptional_exit:

	delete_spine_stack(stack);

	C reset_detection();

	return r;
}

/* Control can longjmp() back to reduce_tree()
 * in grammar.y for certain input(s). */
int
read_line(void)
{
	char buf[64];
	*buf = 'A';
	do {
		printf("continue? ");
		fflush(stdout);
		fgets(buf, sizeof(buf), stdin);
		switch (*buf)
		{
		case 'x': case 'e':
			exit(0);
			break;
		case 'n': case 'q':
			C reset_detection();
			siglongjmp(in_reduce_graph, 3);
			break;
		case 'c':
			single_step = 0;
			break;
		case '?':
			fprintf(stderr,
				"e, x -> exit now\n"
				"n, q -> terminate current reduction, return to top level\n"
				"c -> continue current reduction without further stops\n"
			);
			break;
		default:
			break;
		}
	} while ('?' == *buf);
	return single_step;
}

int
reduction_count(struct node *node, int stack_depth, int *child_redex, struct buffer *b)
{
	int reductions = 0;
	int left_child_redex = 0, right_child_redex = 0;
	int print_right_paren = 0;

	if (node)
	{
		switch (node->typ)
		{
		case APPLICATION:

			if (!node->left && !node->right) return 0;

			reductions += reduction_count(node->left, stack_depth + 1, &left_child_redex, b);
			if (left_child_redex) buffer_append(b, "*", 1);

			buffer_append(b, " ", 1);

			if (APPLICATION == node->right->typ)
			{
				buffer_append(b, "(", 1);
				print_right_paren = 1;
			}
			reductions += reduction_count(node->right, 0, &right_child_redex, b);
			if (right_child_redex) buffer_append(b, "*", 1);
			if (print_right_paren) buffer_append(b, ")", 1);

			break;
		case ATOM:
			buffer_append(b, node->name, strlen(node->name));
			if (node->rule)
			{
				if (stack_depth >= node->rule->required_depth)
				{
					reductions = 1;
					*child_redex = 1;
				}
			}
			break;
		}
	}

	return reductions;
}

/* when total_count evaluates to true (non-zero),
 * this counts interior and leaf nodes.  Otherwise,
 * it just counts leaf nodes.
 */
int
node_count(struct node *node, int count_interior_nodes)
{
	int count = 0;

	switch (node->typ)
	{
	case APPLICATION:
		if (count_interior_nodes) ++count;
		count += node_count(node->left, count_interior_nodes);
		count += node_count(node->right, count_interior_nodes);
		break;
	case ATOM:
		count = 1;
		break;
	}

	return count;
}

/* return 1 if two graphs "equate", and 0 if they don't.
 * "Equate" means same tree structure (application-type nodes
 * in the same places, leaf (combinator) nodes in the same places),
 * and that combinator-type nodes in the same places have the same name.
 */
int
equivalent_graphs(struct node *g1, struct node *g2)
{
	int r = 0;

	if (g1->typ == g2->typ)
	{
		switch (g1->typ)
		{
		case APPLICATION:
			r = equivalent_graphs(g1->left, g2->left)
				&& equivalent_graphs(g1->right, g2->right);
			break;
		case ATOM:
			if (g1->name == g2->name)
				r = 1;
			break;
		}

	}

	return r;
}
