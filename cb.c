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
/* $Id: cb.c,v 1.5 2011/06/12 18:22:01 bediger Exp $ */

/*
 * A First-in, Last-out Queue, implemented as a circular buffer.
 * I did this based on an articl in Dr Dobbs journal, about how
 * linked-list implementations of some conceptual data structures
 * caused a lot of cache-hits.  Can't find the article, had to have
 * appeared in the early-to-mid 90s, as I had the Aho-Corasick code
 * on-tap in 1999 for something.
 *
 * If the circular buffer overflows, any overflow goes into a standard
 * singly-linked list.
 */

#include <stdio.h>
#include <stdlib.h>

#include <cb.h>

/*
 * These two need to constitute 2 bit patterns:
 * CBUFSIZE  100...0
 * CBUFMASK   11...1
 *
 * Actual value of CBUFSIZE mostly constitutes a guess. I think that
 * the enqueue and dequeue operations will slow down somewhat when
 * the circular buffer, sized by CBUFSIZE, overflows.
 */
#define CBUFSIZE 0x20
#define CBUFMASK 0x1f

struct queue_node {
	int state;
	struct queue_node *next;
};

struct queue {
	int *cbuf;
	int  in;
	int  out;
	struct queue_node *ovfhead;
	struct queue_node *ovftail;
};

struct queue *
queueinit()
{
	struct queue *r;

	r = malloc(sizeof(*r));

	r->cbuf = malloc(CBUFSIZE*sizeof(int));

	r->in = 0;
	r->out = 0;
	r->ovfhead = r->ovftail = NULL;

	return r;
}

void
enqueue(struct queue *q, int state)
{
	struct queue_node *newnode;

	if (((q->in + 1)&CBUFMASK) != q->out)
	{
		q->cbuf[q->in] = state;
		q->in = (q->in+1)&CBUFMASK;

	} else {

		newnode = malloc(sizeof *newnode);
		newnode->state = state;
		newnode->next = NULL;

		if (NULL == q->ovftail)
		{
			q->ovfhead = q->ovftail = newnode;
		} else {
			q->ovftail->next = newnode;
			q->ovftail = newnode;
		}
	}
}

int
queueempty(struct queue *q)
{
	return (q->in == q->out);
}

int
dequeue(struct queue *q)
{
	int r;

	r = q->cbuf[q->out];

	q->out = (q->out + 1)&CBUFMASK;

	if (NULL != q->ovfhead)
	{
		struct queue_node *tmp;

		q->cbuf[q->in]= q->ovfhead->state;
		q->in = (q->in+1)&CBUFMASK;
		tmp = q->ovfhead;
		q->ovfhead = q->ovfhead->next;
		if (NULL == q->ovfhead) q->ovftail = NULL;
		free(tmp);
	}

	return r;
}

void
queuedestroy(struct queue *q)
{
	free(q->cbuf);

	/* I don't think this while-loop ever gets executed:
	 * the code in aho_corasick.c never destroys anything
	 * other than an empty queue.  An abstraction rule that
	 * overflows the circular buffer part of this queue should
	 * never have any "overflow" when it gets deleted. */
	while (NULL != q->ovfhead)
	{
		struct queue_node *t = q->ovfhead->next;

		free(q->ovfhead);

		q->ovfhead = t;
	}

	free(q);
}
