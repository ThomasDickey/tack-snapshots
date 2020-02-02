/*
** Copyright 2017,2020 Thomas E. Dickey
** Copyright 1997-2012,2013 Free Software Foundation, Inc.
**
** This file is part of TACK.
**
** TACK is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, version 2.
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
/* scan mode keyboard support */

#include <tack.h>

MODULE_ID("$Id: scan.c,v 1.17 2020/02/02 15:30:33 tom Exp $")

size_t scan_max;		/* length of longest scan code */
char **scan_up, **scan_down, **scan_name;
size_t *scan_tested, *scan_length;
static unsigned *scan_value;

static unsigned shift_state;
static int debug_char_count;

#define SHIFT_KEY   0x100
#define CONTROL_KEY 0x200
#define META_KEY    0x400
#define CAPS_LOCK   0x800

int
scan_key(void)
{				/* read a key and translate scan mode to
				   ASCII */
    unsigned i;
    char buf[64];

    for (i = 1;; i++) {
	int j;
	int ch = getchar();

	if (ch == EOF)
	    return EOF;
	if (debug_fp) {
	    fprintf(debug_fp, "%02X ", ch);
	    debug_char_count += 3;
	    if (debug_char_count > 72) {
		fprintf(debug_fp, "\n");
		debug_char_count = 0;
	    }
	}
	buf[i - 1] = (char) ch;
	buf[i] = '\0';
	if (buf[0] & 0x80) {	/* scan up */
	    for (j = 0; scan_up[j]; j++) {
		if (i == scan_length[j] &&
		    !strcmp(buf, scan_up[j])) {
		    i = 0;
		    shift_state &= ~scan_value[j];
		    break;
		}
	    }
	    continue;
	}
	for (j = 0; scan_down[j]; j++) {
	    if (i == scan_length[j] && !strcmp(buf, scan_down[j])) {
		i = 0;
		shift_state |= scan_value[j];
		ch = (int) scan_value[j];
		if (ch == CAPS_LOCK)
		    shift_state ^= SHIFT_KEY;
		if (ch >= 256)
		    break;
		if (shift_state & SHIFT_KEY) {
		    if (ch >= 0x60)
			ch -= 0x20;
		    else if (ch >= 0x30 && ch <= 0x3f)
			ch -= 0x10;
		}
		if (shift_state & CONTROL_KEY) {
		    if ((ch | 0x20) >= 0x60 &&
			(ch | 0x20) <= 0x7f)
			ch = (ch | 0x20) - 0x60;
		}
		if (shift_state & META_KEY)
		    ch |= 0x80;
		return ch;
	    }
	}
	if (i > scan_max)
	    i = 1;
    }
}
