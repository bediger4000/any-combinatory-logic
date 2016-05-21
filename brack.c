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
/* $Id: brack.c,v 1.10 2011/06/12 18:22:00 bediger Exp $ */

/*
 * Generalized bracket abstraction.  Externally-visible unctions to:
 * 1. Add a new abstraction rule: set_abstraction_rule()
 * 2. Print out all abstraction rules: print_abstractions()
 * 3. De-allocate all memory used by abstraction rules: delete_abstraction_rules()
 * 4. Do bracket abstraction on an expression: perform_bracket_abstraction()
 */

#include <stdio.h>    /* NULL manifest constant */
#include <stdlib.h>   /* malloc(), free(), realloc() */
#include <string.h>   /* memcpy() */

#include <node.h>
#include <hashtable.h>
#include <atom.h>
#include <brack.h>
#include <aho_corasick.h>
#include <buffer.h>
#include <graph.h>

/*
 * Functions and variables to calculate all the root-to-leaves
 * paths through a pattern.
 */
int set_pattern_paths(struct abs_node *pattern);
const char **get_pat_paths(void);
void calculate_strings(struct abs_node *node, struct buffer *buf);
/* **paths holds an array of const char * (atom) strings, one
 * string for each path through a pattern from root-to-leaf. */
static const char **paths = NULL;
/* **paths has a size (value of path_cnt) and a numer of entries
 * currently filled in (value of paths_used).  Dynamically resizes
 * paths when the number of paths-through-patterns gets too big.
 * **paths and path_cnt reused every time set_abstraction_rule()
 * gets called. */
static int path_cnt = 0;
static int paths_used = 0;

extern int trace_reduction;

/* Internal representation of a bracket abstraction
 * rule, and the dynamically-resized array (**rules)
 * used to keep track of the rules. */
struct abstraction_rule {
	struct gto *g;
	int pat_path_cnt;
	struct abs_node *pattern;
	struct abs_node *replacement;
	int replaceable_leaves_cnt;
};

static struct abstraction_rule **rules;
static int rule_cnt = 0;

static const char *dummy_abstr_var = NULL;

/* Support functions called by perform_bracket_abstraction() */
int count_effective_leaves(struct abs_node *tree);
void fill_in_replacements(struct abs_node *pattern, struct node *expr, struct node **replacements, int *counter);
struct node *perform_replacement(
	struct abstraction_rule *matched_rule,
	const char *var_name,
	struct node **replacements,
	struct abs_node *template
);
void massage_replacements(struct abs_node *replacement);

/* Working function to print a single rule. */
void print_rule(struct abstraction_rule *abs_rule, struct node *tree);

/* Centralize output of pattern/replacement/subject.
 * Used in both the "abstractions" interpreter command's output,
 * and when "trace on" issued, and a bracket abstraction gets
 * performed.
 */
void
print_rule(struct abstraction_rule *abs_rule, struct node *tree)
{
	printf("[_] ");
	print_abs_node(abs_rule->pattern);
	printf(" -> ");
	print_abs_node(abs_rule->replacement);
	if (tree)
	{
		printf(" on: ");
		print_tree(tree, 0, 0);
	}
	printf("\n");
}

struct node *
perform_bracket_abstraction(const char *var, struct node *expr)
{
	struct node *r = NULL;
	int idx, subject_node_count = 0;

	subject_node_count = node_count(expr, 1);

	for (idx = 0; idx < rule_cnt; ++idx)
	{
		int repl_cnt = 0;
		int matched = algorithm_d(rules[idx]->g, expr,
			subject_node_count, rules[idx]->pat_path_cnt, var);

		if (matched)
		{
			struct node **replacements
				= malloc(rules[idx]->replaceable_leaves_cnt * (sizeof (struct node *)));

			if (trace_reduction) print_rule(rules[idx], expr);

			fill_in_replacements(rules[idx]->pattern, expr,
				replacements, &repl_cnt);

			r = perform_replacement(rules[idx], var, replacements, rules[idx]->replacement);

			free(replacements);

			break;  /* only do the first rule you find. */
		}
	}

	return r;
}

void
delete_abstraction_rules(void)
{
	int idx;

	for (idx = 0; idx < rule_cnt; ++idx)
	{
		struct abstraction_rule *p = rules[idx];

		rules[idx] = NULL;

		destroy_goto(p->g);
		p->g = NULL;

		free_abs_node(p->pattern);
		free_abs_node(p->replacement);

		p->pattern = p->replacement = NULL;

		free(p);

		p = NULL;
	}

	free(rules);
	rules = NULL;

	if (paths)
		free(paths);
	paths = NULL;
}

void
set_abstraction_rule(struct abs_node *pattern, struct abs_node *replacement)
{
	int n;
	const char **path_ary;

	if (!dummy_abstr_var)
		dummy_abstr_var = Atom_string("_");

	n = set_pattern_paths(pattern);

	path_ary = get_pat_paths();

	rules = realloc(rules, sizeof(struct abstraction_rule) * (rule_cnt + 1));

	rules[rule_cnt] = malloc(sizeof(struct abstraction_rule));

	rules[rule_cnt]->g = init_goto();

	construct_goto(path_ary, n, rules[rule_cnt]->g);
	/* Does nothing with path_ary: the array itself sticks around
	 * for the next call to set_abstraction_rule(), while the
	 * array elements (strings) have type Atom, and get
	 * deallocated when the Atom hashtable gets deallocated. */

	construct_failure(rules[rule_cnt]->g);
	construct_delta(rules[rule_cnt]->g);
	rules[rule_cnt]->pat_path_cnt = n;

	rules[rule_cnt]->pattern = pattern;
	rules[rule_cnt]->replacement = replacement;
	massage_replacements(rules[rule_cnt]->replacement);

	rules[rule_cnt]->replaceable_leaves_cnt
		= count_effective_leaves(rules[rule_cnt]->pattern);

	++rule_cnt;
}

/* Called from the interpreter command "abstractions". */
void
print_abstractions(void)
{
	int i;

	printf("# %d abstraction rules\n", rule_cnt);

	for (i = 0; i < rule_cnt; ++i)
	{
		printf("abstraction: ");
		print_rule(rules[i], NULL);
		printf("# Path count: %d, Max depth: %d\n",
			rules[i]->pat_path_cnt, rules[i]->g->max_node_count);
	}
}

/*
 * A tree of structs abs_node has leaf nodes (typ == abs_LEAF)
 * which actually can represnt a subtree of the subject trees,
 * which have type of struct node.  "Effective" leaf nodes
 * can have labels like "*", "*+", "*-" which match subtrees.
 */
int
count_effective_leaves(struct abs_node *pattern)
{
	int n = 0;
	if (pattern->typ == abs_LEAF)
		n = 1;
	else {
		n = count_effective_leaves(pattern->left);
		n += count_effective_leaves(pattern->right);
	}
	return n;
}

/* Fills in the array of "replacements" used by
 * perform_replacement().  The structs node put into
 * the array at indexes that match numbers used in the
 * LHS (template) part of an abstraction rule.
 */
void
fill_in_replacements(struct abs_node *pattern, struct node *expr,
				struct node **replacements, int *counter)
{
	switch (pattern->typ)
	{
	case abs_LEAF:
		replacements[*counter] = expr;
		++*counter;
		break;
	case abs_APPLICATION:
		fill_in_replacements(pattern->left, expr->left,
			replacements, counter);
		fill_in_replacements(pattern->right, expr->right,
			replacements, counter);
		break;
	}
}

/* Actually compose a struct node * with the graph
 * resulting from abstracting out a variable. */
struct node *
perform_replacement(
	struct abstraction_rule *rule,
	const char *var,
	struct node **replacements,
	struct abs_node *template
)
{
	struct node *r = NULL;

	switch (template->typ)
	{
	case abs_LEAF:
		if (template->number < 0)
		{
			r = (template->label == dummy_abstr_var)
				? new_term(var)
				: new_term(template->label);
			r->rule = template->rule;
		} else
			r = template->abstracted
				? replacements[template->number]
				: arena_copy_graph(replacements[template->number]);
		break;

	case abs_APPLICATION:
		r = new_application(
			perform_replacement(rule, var, replacements, template->left),
			perform_replacement(rule, var, replacements, template->right)
		);
		break;
	}

	/* This part implements those abstraction rules that
	 * compose an expression, and then abstract the variable
	 * from that newly composed expression.
	 *
	 * Don't need to worry about leaking the value of r
	 * here: if this template node indicates an abstraction,
	 * r got set to an element of replacements[] above. */
	if (template->abstracted)
	{
		struct node *tmp = r;
		r = perform_bracket_abstraction(var, tmp);
		++tmp->refcnt;
		free_node(tmp);
	}

	return r;
}

/* Traverse the tree representing the replacement template,
 * and set the struct abs_node field "number", based on how
 * the field "label" translates to an int.
 * The value of field "number" will get used during the course
 * of building up the replacement tree of structs node, to find
 * the part of the pattern referenced by the replacement template.
 */
void
massage_replacements(struct abs_node *replacement)
{
	int x;

	switch (replacement->typ)
	{
	case abs_LEAF:
		x = atoi(replacement->label);
		if (x > 0)
			replacement->number = x - 1; /* index into rules[i].replacmenets */
		else
			replacement->number = -1;
		break;
	case abs_APPLICATION:
		massage_replacements(replacement->left);
		massage_replacements(replacement->right);
		break;
	}
}


/* Allow set_abstraction_rule() to pull all the "paths" through
 * a pattern out of this module. */
const char **
get_pat_paths(void)
{
	/* XXX - just reuses the paths array, not strings it contains.
	 * The strings that **paths contains are "atoms", not-to-be-overwritten
	 * ASCII-Nul-terminated strings held in a struct hashtable for use
	 * and re-use.  Don't need to deallocate them here. */
	paths_used = 0;
	return paths;
}

int
set_pattern_paths(struct abs_node *pattern)
{
	struct buffer *buf = new_buffer(512);
	calculate_strings(pattern, buf);
	delete_buffer(buf);
	return paths_used;
}

/*
 * Calculate all the root-to-leaves paths through
 * a pattern.  For example, "S K *" would end up as
 * 3 strings, each string representing a path:
 * "@1@1S"
 * "@1@2K"
 * "@2*"
 *
 * These strings end up as "atoms" (const char *), elements
 * of const char **paths.
 *
 * set_abstraction_rule() constructs a struct gto using those 3 strings,
 * then algorithm_d() uses the struct gto to find matching
 * sub-trees in the subject of a bracket abstraction.
 */
void
calculate_strings(struct abs_node *node, struct buffer *b)
{
	int curr_offset, orig_offset = b->offset;
	char *buf;
	char *pattern_string;

	switch (node->typ)
	{
	case abs_APPLICATION:
		buffer_append(b, node->label, strlen(node->label));
		curr_offset = b->offset;

		buffer_append(b, "1", 1);
		calculate_strings(node->left, b);

		/* Resetting b->offset effectively erases the suffix
		 * put on b by calculate_strings(node->left, b).  That
		 * lets us put on a suffix representing the right-hand-side
		 * sub-tree of struct node *node. */
		b->offset = curr_offset;
		buffer_append(b, "2", 1);

		calculate_strings(node->right, b);

		b->offset = orig_offset;
		break;

	case abs_LEAF:
		buf = b->buffer;
		buf[b->offset] = '\0';
		pattern_string = malloc(b->offset + 1 + 1);

		if ('*' != node->label[0])
			sprintf(pattern_string, "%s%s", buf, node->label);
		else {
			/* Special case leaf-node labels: "*", "*-", "*+".
			 * Somewhat confusing, as a struct abs_node with typ == abs_LEAF
			 * can represent a LEAF or an application node in the subject tree.
			 * 
			 */
			switch (node->label[1])
			{
			case '\0':
				/* "*" label */
				sprintf(pattern_string, "%s", buf);
				break;
			case '+':
			case '-':
			case '!':
				sprintf(pattern_string, "%s%c", buf, node->label[1]);
				break;
			}
		}

		if (paths_used >= path_cnt)
		{
			const char **tmp;
			int alloc_bytes = (sizeof(char *))*(path_cnt + 4);

			if (paths)
				tmp = realloc(paths, alloc_bytes);
			else
				tmp = malloc(alloc_bytes);

			paths = tmp;
			path_cnt += 4;
		}

		/* XXX - If a realloc() fails, this could overwrite paths[] */
		paths[paths_used++] = Atom_string(pattern_string);
		free(pattern_string);

		break;
	}
}
