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
/* $Id: cb.h,v 1.3 2011/06/12 18:19:11 bediger Exp $ */

struct queue *queueinit(void);
void queuedestroy(struct queue *);
void enqueue(struct queue *, int);
int  dequeue(struct queue *);
int  queueempty(struct queue *);
void empty_error(struct queue *);
