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
/* $Id: buffer.c,v 1.6 2011/06/12 18:22:01 bediger Exp $ */

/*
 * Self-resizing buffer.
 * The usual life-cycle:
 * b = new_buffer(some_size);
 * buffer_append(b, ptr, strlen(ptr));
 * buffer_append(b, "/", 1);
 * buffer_append(b, ptr2, strlen(ptr2));
 * char *string = b->buffer;
 * ... use string
 * delete_buffer(b);
 */

#include <stdio.h>    /* NULL manifest constant */
#include <stdlib.h>   /* malloc(), free(), realloc() */
#include <string.h>   /* memcpy() */

#include <buffer.h>

struct buffer *
new_buffer(int desired_size)
{
	struct buffer *r = malloc(sizeof *r);

	r->size = 0;
	r->buffer = malloc(desired_size);
	r->size = desired_size;
	r->offset = 0;

	return r;
}

void
delete_buffer(struct buffer *b)
{

	if (b->buffer)
	{
		free(b->buffer);
		b->buffer = NULL;
	}

	b->offset = b->size = 0;
	free(b);
	b = NULL;
}

void
resize_buffer(struct buffer *b, int increment)
{
	char *reallocated_buffer;
	int new_size = b->size + increment;

	reallocated_buffer = realloc(b->buffer, new_size);

	b->buffer = reallocated_buffer;
	b->size   = new_size;
}

void
buffer_append(struct buffer *b, const char *bytes, int length)
{
	if (length >= (b->size - b->offset))
		resize_buffer(b, length);

	/* XXX - assumes resize_buffer() always succeeds. */
	memcpy(&b->buffer[b->offset], bytes, length);
	b->offset += length;
}
