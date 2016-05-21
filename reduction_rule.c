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
#include <stdio.h>
#include <stdlib.h>


#include <node.h>
#include <spine_stack.h>
#include <reduction_rule.h>

void print_reduction_rule(struct reduction_rule *rule);
void print_reduction_tree(struct reduction_rule_node *tree);

void free_reduction_rule(struct reduction_rule *rule);

struct node *reduce_rule(
	struct reduction_rule_node *rnode,
	struct spine_stack *stack
);

/* **rules constitutes a dynamically resized array
 * of structs reduction_rule.
 */
struct reduction_rule **rules = NULL;
int number_of_rules = 0;     /* How many elements of **rules in use. */
int max_number_of_rules = 0; /* How many elements **rules has total. */

void
free_rules(void)
{
	int i;
	for (i = 0; i < number_of_rules; ++i)
	{
		free_reduction_rule(rules[i]);
		rules[i] = NULL;
	}
	free(rules);
	rules = NULL;
}

void
free_reduction_rule(struct reduction_rule *rule)
{
	rule->name = NULL;
	rule->required_depth = 0;
	free_reduction_tree(rule->result_tree);
	rule->result_tree = NULL;
	free(rule);
	rule = NULL;
}

void
free_reduction_tree(struct reduction_rule_node *tree)
{
	if (tree)
	{
		tree->parent = NULL;
		tree->combinator_argument_number = 0;
		tree->depth = 0;
		free_reduction_tree(tree->func);
		tree->func = NULL;
		free_reduction_tree(tree->arg);
		tree->arg = NULL;
		free(tree);
		tree = NULL;
	}
}

void
print_reduction_rule(struct reduction_rule *rule)
{
	int i;
	printf("rule: %s", rule->name);
	for (i = 1; i <= rule->required_depth; ++i)
		printf(" %d", i);
	printf(" -> ");
	print_reduction_tree(rule->result_tree);
	printf("\n");
}

void
print_reduction_tree(struct reduction_rule_node *node)
{
	int print_match_paren = 0;
	if (node->func)
	{
		print_reduction_tree(node->func);
		printf(" ");
		if (!node->arg->combinator_argument_number)
		{
			printf("(");
			print_match_paren = 1;
		}
	}
	if (node->combinator_argument_number) printf("%d", node->combinator_argument_number);
	if (node->arg) print_reduction_tree(node->arg);
	if (print_match_paren) printf(")");
}

void
add_reduction_rule(struct reduction_rule *rule)
{
	if (number_of_rules >= max_number_of_rules)
	{
		rules = realloc(rules, (max_number_of_rules + 4)*sizeof(*rules));
		max_number_of_rules += 4;
	}

	if (number_of_rules < max_number_of_rules)
	{
		struct reduction_rule *prev = get_reduction_rule(rule->name);
		if (prev)
		{
			int i;
			/* Hey, this looks through rules[] twice, once in get_reduction_rule(),
			 * once right here.  Could trigger a bug if concurrent. */
			for (i = 0; i < number_of_rules; ++i)
			{
				if (rules[i]->name == rule->name)
				{
					rules[i] = rule;
					break;
				}
			}
			free_reduction_rule(prev);
			prev = NULL;
		} else
			rules[number_of_rules++] = rule;
	}
}

struct reduction_rule *
get_reduction_rule(const char *identifier)
{
	struct reduction_rule *r = NULL;
	int idx;
	for (idx = 0; idx < number_of_rules; ++idx)
	{
		if (identifier == rules[idx]->name)
		{
			r = rules[idx];
			break;
		}
	}
	return r;
}

struct node *
reduce_rule(
	struct reduction_rule_node *rnode,
	struct spine_stack *stack
)
{
	struct node *n = NULL;
	struct node *tmp = NULL;

	if (0 == rnode->combinator_argument_number)
	{
		/* Application node in reduced tree */
		n = new_application(
			reduce_rule(rnode->func, stack),
			reduce_rule(rnode->arg, stack)
		);
	} else {
		/* Atom node in reduced tree */
		tmp = PARENTNODE(stack, rnode->combinator_argument_number);
		/* Reference counting takes care of common subexpressions. */
		n = tmp->right;
	}

	return n;
}

/* Assumes that the top-of-stack struct node is
 * the atomic primitive to be reduced. */
void
perform_reduction(struct spine_stack *stack)
{
	struct node *topnode = TOPNODE(stack);
	struct node *m = NULL, *n = NULL, *tmp = NULL;

	tmp = PARENTNODE(stack, topnode->rule->required_depth);
	m = PARENTNODE(stack, topnode->rule->required_depth - 1);
	n = reduce_rule(topnode->rule->result_tree, stack);
	*(m->updateable) = n;
	++n->refcnt;
	free_node(tmp);
}

/* Called by "rules" interpreter command. */
void
print_rules(void)
{
	int i;

	for (i = 0; i < number_of_rules; ++i)
		print_reduction_rule(rules[i]);
}
