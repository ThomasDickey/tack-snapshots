/*
** Copyright (C) 2017 Free Software Foundation, Inc.
**
** This file is part of TACK.
**
** TACK is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2, or (at your option)
** any later version.
**
** TACK is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with TACK; see the file COPYING.  If not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301, USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <term.h>

/* $Id: tackgen.c,v 1.2 2017/07/21 23:46:18 tom Exp $ */

typedef struct {
    int name_index;
    const char *short_name;
    const char *type_name;
    const char *long_name;
} DATA;

static int
compare_data(const void *a, const void *b)
{
    const DATA *p = (const DATA *) a;
    const DATA *q = (const DATA *) b;
    return strcmp(p->short_name, q->short_name);
}

static char *
tabbed(const char *value, int extra)
{
    int len = (int) strlen(value) + extra;
    return (len < 8) ? "\t\t" : "\t";
}

static void
show_count(int num, const char *tag)
{
    printf("#ifndef %sCOUNT\n", tag);
    printf("#define %sCOUNT %d\n", tag, num);
    printf("#endif\n");
}

int
main(void)
{
    size_t s, d;
    size_t num_b = 0;
    size_t num_n = 0;
    size_t num_s = 0;
    size_t num_values;
    DATA *values = 0;

    while (boolnames[num_b] != 0)
	++num_b;
    while (numnames[num_n] != 0)
	++num_n;
    while (strnames[num_s] != 0)
	++num_s;

    show_count(num_b,"BOOL");
    show_count(num_n,"NUM");
    show_count(num_s,"STR");

    printf("#ifdef NAME_ENTRY_DATA\n");

    num_values = num_b + num_n + num_s;

    values = calloc(num_values, sizeof(DATA));

    d = 0;
    for (s = 0; boolnames[s] != 0; ++s) {
	values[d].name_index = s;
	values[d].short_name = boolnames[s];
	values[d].long_name = boolfnames[s];
	values[d].type_name = "BOOLEAN";
	++d;
    }
    for (s = 0; numnames[s] != 0; ++s) {
	values[d].name_index = s;
	values[d].short_name = numnames[s];
	values[d].long_name = numfnames[s];
	values[d].type_name = "NUMBER";
	++d;
    }
    for (s = 0; strnames[s] != 0; ++s) {
	values[d].name_index = s;
	values[d].short_name = strnames[s];
	values[d].long_name = strfnames[s];
	values[d].type_name = "STRING";
	++d;
    }
    qsort(values, num_values, sizeof(DATA), compare_data);
    for (s = 0; s < num_values; ++s) {
	printf("DATA(\t%3d,\t\"%s\",%s%s%s),\t/* %s */\n",
	       values[s].name_index,
	       values[s].short_name, tabbed(values[s].short_name, 3),
	       values[s].type_name, tabbed(values[s].type_name, 0),
	       values[s].long_name);
    }

    printf("#endif /* NAME_ENTRY_DATA */\n");

    return 0;
}
