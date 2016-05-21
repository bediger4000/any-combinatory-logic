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
/* $Id: brack.h,v 1.2 2010/07/19 17:55:40 bediger Exp $ */

struct node *perform_bracket_abstraction(const char *var, struct node *expr);
void set_abstraction_rule(struct abs_node *pattern, struct abs_node *replacement);
void print_abstractions(void);
void delete_abstraction_rules(void);
