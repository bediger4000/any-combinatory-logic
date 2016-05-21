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
/* $Id: arena.c,v 1.2 2010/04/02 03:43:39 bediger Exp $ */

#include <stdio.h>
#include <unistd.h>  /* getpagesize() */
#include <stdlib.h>  /* malloc(), free() */
#include <assert.h>  /* malloc(), free() */

#include <arena.h>

#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

union combo {
	char c;
	short s;
	int i;
	long l;
	long long ll;
	void *vp;
	char *cp;
	short *sp;
	int  *ip;
	long *lp;
	float f;
	double d;
};

struct memory_arena {
	char *first_allocation;
	char *next_allocation;
	int sz;                     /* max free size */
	int remaining;              /* how many bytes remain in allocation */
	struct memory_arena *next;
	int usage_info;
	int sn;                     /* serial number: malloc order */
};

static int pagesize = 0;
static int headersize = 0;
static int combosize = 0;

/* Public way to get a new struct memory_arena
 * The first struct memory_arena in a chain constitutes a "dummy",
 * an empty head element of a FIFO (stack) of structs memory_arena.
 * The first allocation demanded from a memory arena will cause
 * the malloc() of one or more pages, and a 2nd element in the stack.
 */
struct memory_arena *
new_arena(int memory_info_flag)
{
	struct memory_arena *ra = NULL;

	if (!pagesize)
	{
		combosize = sizeof(union combo);
		pagesize = getpagesize();
		headersize = roundup(sizeof(struct memory_arena), combosize);
	}

	ra = malloc(sizeof(*ra));

	ra->sz = 0;
	ra->remaining = 0;
	ra->first_allocation = ra->next_allocation = NULL;
	ra->next = NULL;
	ra->usage_info = memory_info_flag;
	ra->sn = 0;

	return ra;
}

void
deallocate_arena(struct memory_arena *ma, int memory_info_flag)
{
	int free_arena_cnt = 0;
	int maxsz = 0;
	while (ma)
	{
		struct memory_arena *tmp = ma->next;

		if (ma->sz > maxsz) maxsz = ma->sz;

		ma->next = NULL;
		free(ma);

		ma = tmp;
		++free_arena_cnt;
	}

	if (memory_info_flag)
	{
		fprintf(stderr, "Page size %d (0x%x)\n", pagesize, pagesize);
		fprintf(stderr, "Header size %d (0x%x)\n", headersize, headersize);
		fprintf(stderr, "Allocated %d arenas\n", free_arena_cnt);
		fprintf(stderr, "Max arena size %d (0x%x)\n", maxsz, maxsz);
	}
}

void
free_arena_contents(struct memory_arena *ma)
{
	int last_sn = ma->sn;
	int max_sn = ma->sn;
	int arena_count = 0;

	ma = ma->next; /* dummy head node */
	while (ma)
	{
		++arena_count;

		if (ma->sn >= last_sn + 1)
		{
			fprintf(stderr, "Loop in chain of allocations: S/N %d, next S/N %d\n",
				last_sn, ma->sn);
			fprintf(stderr, "Allocated %d arenas\n", max_sn);
			break;
		} else
			last_sn = ma->sn;

		if (ma->sn > max_sn)
		{
			fprintf(stderr, "Chain of allocations out of order: max S/N %d, S/N %d\n",
				max_sn, ma->sn);
			fprintf(stderr, "Allocated %d arenas\n", max_sn);
			break;
		}

		if (arena_count > max_sn)
		{
			fprintf(stderr, "Found at least %d allocations on chain, should only have %d\n",
				arena_count, max_sn);
			break;
		}

		ma->remaining = ma->sz;
		ma->next_allocation = ma->first_allocation;
		ma = ma->next;
	}
}

void *
arena_alloc(struct memory_arena *ma, size_t size)
{
	void *r = NULL;
	struct memory_arena *ca;
	size_t nsize;

	assert(NULL != ma);

	/* What you actually have to allocate to get to
	 * a block "suitably aligned" for any use. */
	nsize = roundup(size, combosize);

	ca = ma->next;

	while (ca && nsize > ca->remaining)
		ca = ca->next;

	if (NULL == ca)
	{

		/* You could do something like moving all arenas with
		 * less than combosize remaining into a 2nd list - one
		 * that never gets checked, as combosize is the minimum
		 * allocation possible. */

		/* create a new struct memory_arena of at least 1 page */
		size_t arena_size = roundup((nsize + headersize), pagesize);

		ca = malloc(arena_size);

		ca->first_allocation = ((char *)ca) + headersize;
		ca->next_allocation = ca->first_allocation;
		ca->sz = arena_size - headersize;
		ca->remaining = ca->sz;

		/* Each struct arena gets a unique serial number,
		 * and the dummy "head" node sn field has a count of
		 * how many structs arena reside in the list.
		 */
		ca->sn = ++ma->sn;

		/* since the next 2 lines make a FIFO stack of structs
		 * arena, the serial numbers must decrease by one 
		 * as you walk the stack. */
		ca->next = ma->next;
		ma->next = ca;
	}

	r = ca->next_allocation;
	ca->next_allocation += nsize; /* next_allocation stays aligned */
	ca->remaining -= nsize;

	return r;
}

