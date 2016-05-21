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
/* $Id: aho_corasick.c,v 1.8 2011/06/12 18:22:00 bediger Exp $ */

/*
 *  This code based on:
 * Pattern Matching in Trees
 * Christoph M. Hoffmann and Michael J. O'Donnell
 * Journal of the Association for Computing Machinery
 * Vol 29, No 1, January 1982
 * pp 68-95
 * http://ftp.cs.purdue.edu/research/technical_reports/1973/TR%2073-091.pdf
 *  and on
 * Efficient string matching: an aid to bibliographic search
 * Alfred V. Aho, Margaret J. Corasick    
 * Communications of the ACM
 * Volume 18, Issue 6 (June 1975)
 * Pages: 333 - 340
 *
 * I can't say that I truly understood the latter paper, except during
 * the throes of development, so commentary on the Aho-Corasick part of
 * the code is sparse.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <node.h>
#include <aho_corasick.h>
#include <cb.h>
#include <hashtable.h>
#include <atom.h>

/* From "Pattern Matching in Trees". */
struct stack_elem {
	struct node *n; /* 1 */
	int state_at_n; /* 2 */
	int visited;    /* 3 */
	int node_number;
};

void set_output_length(struct gto *p, int state, int node_count);
int tabulate(struct gto *g, struct stack_elem *stack, int top, int state, int pat_leaf_count, int *count);

const char *abstr_meta_var;


struct gto *
init_goto()
{
	int i;
	struct gto *g = NULL;

	abstr_meta_var = Atom_string("_");

	g = malloc(sizeof(*g));

	g->ary = malloc(sizeof(int *));

	g->ary[0] = malloc(128*sizeof(int));

	g->ary_len = 1;

	for (i = 0; i < 128; ++i)
		g->ary[0][i] = FAIL;

	g->output = malloc(sizeof(*g->output));
	g->output_len = 1;

	g->output->len = g->output->max = 0;
	g->output->out = NULL;

	g->max_node_count = 0;

	return g;
}

void
construct_goto(const char *keywords[], int k, struct gto *g)
{
	int newstate = 0;
	int i;

	for (i = 0; i < k; ++i)
	{
		int state, j, p;

		/* procedure enter() */

		state = 0;
		j = 0;

		while (
			FAIL != (
				(state<g->ary_len) ?
					g->ary[state][(int)keywords[i][j]] :
					(assert(state>=g->ary_len),FAIL)
				)
			)
		{
			state = ((state<g->ary_len) ?
						g->ary[state][(int)keywords[i][j]] :
						(assert(state>=g->ary_len),FAIL)
					);
			++j;
		}

		for (p = j; '\0' != keywords[i][p]; ++p)
		{
			++newstate;
			add_state(g, state, keywords[i][p], newstate);
			state = newstate;
		}

		/* end procedure enter() */

		set_output(g, state, keywords[i]);
	}

	for (i = 0; i < 128; ++i)
	{
		if (FAIL == g->ary[0][i])
			g->ary[0][i] = 0;
	}
}

void
set_output(struct gto *p, int state, const char *keyword)
{
	size_t kwl = strlen(keyword);
	int i;
	int node_count = 0;

	/* Slightly weird: count the number of nodes in
	 * the "keyword", which is actually a string, from
	 * root to leaf of a path through a pattern tree. */
	for (i = 0; i < kwl; ++i)
	{
		/* Skip the characters '1' and '2': they're
		 * the characters in the string indicating left
		 * or right branch at an application node. */
		if ('1' != keyword[i] && '2' != keyword[i])
		{
			++node_count;

			/* If you hit a keyword that doesn't begin
			 * with '@', you've hit a leaf node.  We've
			 * counted it, so now we break out of the loop. */
			if (keyword[i] != '@')
				break;
		}
	}

	set_output_length(p, state, node_count);
}

void
set_output_length(struct gto *p, int state, int node_count)
{
	struct output_extent *oxt;

	/* value of node_count stored against the match */

	if (state >= p->output_len)
	{
		int n = state - p->output_len + 1;  /* how many more to add */
		int l = p->output_len + n;          /* how many total after add */
		int i;

		p->output = realloc(p->output, l*sizeof(struct output_extent));

		/* init only the new structs output_extent, leave previous ones alone */
		for (i = p->output_len; i < l; ++i)
		{
			p->output[i].len = 0;
			p->output[i].max = 0;
			p->output[i].out = NULL;
		}

		p->output_len += n;  /* bumped up the number of structs output_extent */
	}

	/* oxt comprises the lengths of paths in pattern matched in subject */
	oxt = &(p->output[state]);

	if (oxt->len >= oxt->max)
	{
		oxt->max += 4;
		if (oxt->out)
			oxt->out = realloc(oxt->out, oxt->max * sizeof(int));
		else
			oxt->out = malloc(oxt->max * sizeof(int));
	}

	oxt->out[oxt->len++] = node_count;

	/* state has oxt->len matches now */

	if (node_count > p->max_node_count)
		p->max_node_count = node_count;
}


void
add_state(struct gto *p, int state, char input, int new_state)
{
	if (state >= p->ary_len || new_state >= p->ary_len)
	{
		int i, n;

		/* by using something other than 1, would it jack up array length
		 * more than a single entry at a time? */
		n = 1 + ((new_state > state? new_state: state) - p->ary_len);

		p->ary = realloc(p->ary, sizeof(int *)*(p->ary_len + n));

		for (i = p->ary_len; i < p->ary_len + n; ++i)
		{
			int j;


			/* if this ends up being too costly, could increase array size
			 * by more than one each time, and malloc loads of rows at once */
			p->ary[i] = malloc(sizeof(int)*128);

			for (j = 0; j < 128; ++j)
				p->ary[i][j] = FAIL;
		}

		p->ary_len += n;
	}

	p->ary[state][(int)input] = new_state;
}

void
construct_failure(struct gto *g)
{
	int i;
	struct queue *q;

	g->failure = malloc(g->ary_len * sizeof(int));

	for (i = 0; i < g->ary_len; ++i)
		g->failure[i] = 0;

	q = queueinit();

	for (i = 0; i < 128; ++i)
	{
		int s = g->ary[0][i];

		if (0 != s)
		{
			enqueue(q, s);
			g->failure[s] = 0;
		}
	}

	while (!queueempty(q))
	{
		int a;
		int r = dequeue(q);

		for (a = 0; a < 128; ++a)
		{
			int s = g->ary[r][a];

			if (FAIL != s)
			{
				int state;
				struct output_extent *p;

				enqueue(q, s);

				state = g->failure[r];
				while (FAIL == g->ary[state][a])
					state = g->failure[state];

				g->failure[s] = g->ary[state][a];

				/* output(s) <- output(s) U output(f(s)) */
				p = &g->output[g->failure[s]];

				for (i = 0; i < p->len; ++i)
					set_output_length(g, s, p->out[i]);
			}
		}
	}

	queuedestroy(q);
}

void
destroy_goto(struct gto *p)
{
	int i;

	for (i = 0; i < p->ary_len; ++i)
		free(p->ary[i]);

	free(p->ary);

	for (i = 0; i < p->output_len; ++i)
		free(p->output[i].out);

	if (NULL != p->output) free(p->output);
	if (NULL != p->failure) free(p->failure);
	if (NULL != p->delta[0]) free(p->delta[0]);
	if (NULL != p->delta) free(p->delta);

	free(p);
}

void
construct_delta(struct gto *g)
{
	struct queue *q;
	int i, a;

	g->delta = malloc(sizeof(int *)*g->ary_len);

	g->delta[0] = malloc(sizeof(int)*g->ary_len*128);

	memset(g->delta[0], 0, sizeof(int)*g->ary_len*128);

	for (i = 0; i < g->ary_len; ++i)
		g->delta[i] = g->delta[0] + i*128;

	q = queueinit();

	for (a = 0; a < 128; ++a)
	{
		g->delta[0][a] = g->ary[0][a];

		if (0 != g->ary[0][a])
			enqueue(q, g->ary[0][a]);
	}

	while (!queueempty(q))
	{
		int r = dequeue(q);

		for (a = 0; a < 128; ++a)
		{
			int s = g->ary[r][a];

			if (FAIL != s)
			{
				enqueue(q, s);
				g->delta[r][a] = s;
			} else
				g->delta[r][a] = g->delta[g->failure[r]][a];
		}
	}

	queuedestroy(q);
}

/* This function implemented from: "Pattern Matching in Trees".
 * Each "pattern" (LHS of an abstraction rule input) gets converted
 * into strings.  Each string represents one root-to-leaf path
 * through the pattern.  The strings get put into a "goto" table
 * as per Aho & Corasick.  Hoffmann & O'Donnel's Algorithm D does a
 * breadth-first traverse of the graph of the expresson subject to
 * bracket abstraction, trying pattern at each node of the
 * subject.  If all the paths-through-a-pattern get found at a given
 * subject node, do the RHS, perform the replacement specified by the
 * RHS of the abstraction rule at that node.
 */

int
algorithm_d(struct gto *g, struct node *t, int subject_node_count, int pat_path_cnt, const char *abstr_var_name)
{
	int top = 1;
	int matched = 0;
	int breadth_counter = 0;
	int next_state;
	int i;
	int *count;
	struct stack_elem *stack;
	const char *p;


	count = malloc(subject_node_count * sizeof(int));
	stack = malloc((subject_node_count + 1) * sizeof(struct stack_elem));

	for (i = 0; i < subject_node_count; ++i)
		count[i] = 0;

	next_state = 0;
	p = (t->name != abstr_var_name)? t->name: abstr_meta_var;
	while (*p)
		next_state = g->delta[next_state][(int)*p++];

	stack[top].n = t;
	stack[top].state_at_n = next_state;
	stack[top].visited = 0;
	stack[top].node_number = breadth_counter++;

	matched += tabulate(g, stack, top, next_state, pat_path_cnt, count);

	if (!matched)
	{
		if (any_var_in_tree(t))
		{
		  	if (var_in_tree(t, abstr_var_name))
				next_state = g->delta[0][(int)'+'];
			else
				next_state = g->delta[0][(int)'-'];
			matched += tabulate(g, stack, top, next_state, pat_path_cnt, count);
		} else {
			next_state = g->delta[0][(int)'!'];
			matched += tabulate(g, stack, top, next_state, pat_path_cnt, count);
			if (!matched)
			{
				next_state = g->delta[0][(int)'-'];
				matched += tabulate(g, stack, top, next_state, pat_path_cnt, count);
			}
		}
	}

	while (!matched && top > 0)
	{
		struct node *next_node, *this_node = stack[top].n;
		int intstate, nxt_st, this_state = stack[top].state_at_n;
		int visited = stack[top].visited;

		if (visited == 2 || this_node->typ == ATOM || top > g->max_node_count)
			--top;
		else {
			++visited;
			stack[top].visited = visited;

			intstate = g->delta[this_state][visited == 1?'1':'2'];
			matched += tabulate(g, stack, top, intstate, pat_path_cnt, count);

			next_node = (visited == 1)? this_node->left: this_node->right;
			nxt_st = intstate;

			p = (next_node->name != abstr_var_name)? next_node->name: abstr_meta_var;
			while (*p)
				nxt_st = g->delta[nxt_st][(int)*p++];

			++top;
			stack[top].n = next_node;
			stack[top].state_at_n = nxt_st;
			stack[top].visited = 0;
			stack[top].node_number = breadth_counter++;

			if (top <= g->max_node_count)
			{
				matched += tabulate(g, stack, top, nxt_st, pat_path_cnt, count);

				if (any_var_in_tree(next_node))
				{
					if (var_in_tree(next_node, abstr_var_name))
						nxt_st = g->delta[intstate][(int)'+'];
					else
						nxt_st = g->delta[intstate][(int)'-'];
				} else {
					nxt_st = g->delta[intstate][(int)'!'];
					if (0 == nxt_st)
						nxt_st = g->delta[intstate][(int)'-'];
				}

				matched += tabulate(g, stack, top, nxt_st, pat_path_cnt, count);
			}
		}
	}

	free(stack);
	free(count);

	return matched;
}

/* Again, from "Pattern Matching in Trees".
 * If the branches under a given node match all the pattern's
 * paths from that node to leaves, then the node and its sub-tree
 * match the pattern.
 */
int
tabulate(struct gto *g, struct stack_elem *stack, int top, int state, int pat_leaf_count, int *count)
{
	int i;
	int r = 0;
	struct output_extent *oxt;

	if (state < 0)
		return 0;

	oxt = &(g->output[state]);

	for (i = 0; r == 0 && i < oxt->len; ++i)
	{
		int idx = top - oxt->out[i] + 1;

		++count[stack[idx].node_number];

		if (count[stack[idx].node_number] == pat_leaf_count)
			r = 1;
	}

	return r;
}
