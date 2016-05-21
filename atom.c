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
/* $Id: atom.c,v 1.4 2010/08/10 20:50:39 bediger Exp $ */

/*
 * A stripped-down "atom" datatype, a la "C Interfaces and Implementation".
 * The idea is that a program only keeps around one instance of a given
 * string, so the input mechanism (lex in this program's case) "interns"
 * every input string, and always gives the sole instance of that string
 * to the rest of the program.  You can get string identity with "="
 * operator, and you don't have to deallocate any string until the program
 * exits.
 */

#include <string.h>

#include <hashtable.h>
#include <atom.h>


static struct hashtable *atom_table = NULL;

void
setup_atom_table(struct hashtable *h)
{
	atom_table = h;
}

const char *
Atom_string(const char *str)
{
	return add_string(atom_table, str);
}
