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

struct reduction_rule_node {
	struct reduction_rule_node *parent;
	struct reduction_rule_node *func;
	struct reduction_rule_node *arg;
	int combinator_argument_number;
	int depth;
};

struct reduction_rule {
	const char *name;    /* name of the atomic combinator: S, K, B, C, etc */
	int required_depth;

	/* what the re-arraneged arguments to the combinator look like: */
	struct reduction_rule_node *result_tree;
};

void free_reduction_tree(struct reduction_rule_node *tree);
void print_rules(void);
void add_reduction_rule(struct reduction_rule *rule);
struct reduction_rule *get_reduction_rule(const char *identifier);
void perform_reduction(struct spine_stack *stack);
void free_rules(void);

void traverse_rule(struct reduction_rule *rule);
