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
/* $Id: aho_corasick.h,v 1.2 2010/07/19 17:55:40 bediger Exp $ */

struct output_extent {
	int   *out;   /* array of int, lengths of matched paths */
	int    len;   /* next array element to fill in */
	int    max;   /* number of elements in array */
};

struct gto {
	int **ary;                     /* transition table */
	int   ary_len;                 /* max state currently in table */
	int  *failure;                 /* failure states */
	int **delta;
	struct output_extent *output;  /* output for output states */
	int   output_len;              /* max state for output states */
	int max_node_count;
};

#define FAIL -1

void add_state(struct gto *p, int state, char input, int new_state);
void set_output(struct gto *p, int state, const char *keyword);
void construct_goto(const char *keywords[], int k, struct gto *g);
void construct_failure(struct gto *g);
void construct_delta(struct gto *g);
struct gto *init_goto(void);
void        destroy_goto(struct gto *);

int algorithm_d(struct gto *g, struct node *subject, int subject_node_count, int pat_path_cnt, const char *abstr_var_name);
