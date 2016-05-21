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
/* $Id: node.h,v 1.8 2010/08/10 20:50:39 bediger Exp $ */

enum nodeType { APPLICATION, ATOM };

struct node {
	int sn;
	enum nodeType typ;
	const char *name;
	struct node *left;
	struct node *right;
	struct node **updateable;
	struct node **left_addr;
	struct node **right_addr;
	int refcnt;
	struct reduction_rule *rule;
	int node_number;
};

/* struct abs_node: similar data structure created
 * when user-input abstraction rules get parsed. */

enum abs_NodeType { abs_APPLICATION, abs_LEAF };

struct abs_node {
	enum   abs_NodeType typ;
	const char *label;
	int    number;
	int    node_number;
	int    abstracted;
	struct abs_node *left;
	struct abs_node *right;
	struct reduction_rule *rule;
};

struct node *new_application(struct node *left_child, struct node *right_child);
struct node *new_term(const char *name);

void init_node_allocation(int memory_info_flag);
void reset_node_allocation(void);
void print_tree(struct node *root, int reduction_node_sn, int current_node_sn);
void free_all_nodes(int memory_info_flag);
void free_node(struct node *root);

struct node *arena_copy_graph(struct node *root);

int var_in_tree(struct node *tree, const char *var_name);
void renumber(struct node *node, int *n);

struct abs_node *new_abs_node(const char *label);
struct abs_node *new_abs_application(struct abs_node *lft, struct abs_node *rght);
void free_abs_node(struct abs_node *tree);
void print_node_abs(struct abs_node *root);
void print_abs_node(struct abs_node *n);

