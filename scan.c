/*
** This software is Copyright (c) 1991 by Daniel Weaver.
**
** Permission is hereby granted to copy, distribute or otherwise
** use any part of this package as long as you do not try to make
** money from it or pretend that you wrote it.  This copyright
** notice must be maintained in any copy made.
**
** Use of this software constitutes acceptance for use in an AS IS
** condition. There are NO warranties with regard to this software.
** In no event shall the author be liable for any damages whatsoever
** arising out of or in connection with the use or performance of this
** software.  Any use of this software is at the user's own risk.
**
**  If you make modifications to this software that you feel
**  increases it usefulness for the rest of the community, please
**  email the changes, enhancements, bug fixes as well as any and
**  all ideas to me. This software is going to be maintained and
**  enhanced as deemed necessary by the community.
*/
/* scan mode keyboard support */

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include "tac.h"

extern char *getenv();

int scan_max;			/* length of longest scan code */
char **scan_up, **scan_down, **scan_name;
int *scan_tested, *scan_length, *scan_value;

static int shift_state;
static char *str;
static int debug_char_count;

#define SHIFT_KEY 0x100
#define CONTROL_KEY 0x200
#define META_KEY 0x400
#define CAPS_LOCK 0x800

struct {
	char *name;
	int type;
}  scan_special[] = {
	{"<shift>", SHIFT_KEY},
	{"<left shift>", SHIFT_KEY},
	{"<right shift>", SHIFT_KEY},
	{"<control>", CONTROL_KEY},
	{"<left control>", CONTROL_KEY},
	{"<right control>", CONTROL_KEY},
	{"<meta>", META_KEY},
	{"<left meta>", META_KEY},
	{"<right meta>", META_KEY},
	{"<caps lock>", CAPS_LOCK},
	{"<tab>", '\t'},
	{"<space>", ' '},
	{"<return>", '\r'},
	{"<linefeed>", '\n'},
	{"<formfeed>", '\f'},
	{"<backspace>", '\b'},
	{0, 0}
};

static void
scan_blanks(void)
{				/* scan past the white space */
	while (*str == ' ' || *str == '\t')
		str++;
}

static char *
smash(void)
{				/* convert a string to hex */
	char *s, *t;
	int ch, i, j;

	t = s = str;
	for (i = 0; (ch = *str); str++) {
		if (ch >= '0' && ch <= '9')
			j = ch - '0';
		else if (ch >= 'a' && ch <= 'f')
			j = 10 - 'a' + ch;
		else if (ch >= 'A' && ch <= 'F')
			j = 10 - 'A' + ch;
		else if (ch == ' ' || ch == '\t')
			break;
		else
			continue;
		if (i) {
			*s |= j;
			s++;
		} else
			*s = j << 4;
		i ^= 1;
	}
	*s = '\0';
	return t;
}

void
scan_init(char *fn)
{				/* read the scan mode key definitions */
	char *s, *sl;
	FILE *fp;
	int ch, i, j;
	char home[512];

	if ((str = getenv("HOME")))
		strcpy(home, str);
	else
		home[0] = '\0';
	fp = NULL;
	if ((str = getenv("KEYBOARD"))) {
		if (!(fp = fopen(str, "r")) && home[0]) {
			sprintf(temp, "%s/.scan.%s", home, str);
			fp = fopen(temp, "r");
		}
	}
	if (!fp) {
		sprintf(temp, ".scan.%s", fn);
		fp = fopen(temp, "r");
	}
	if (!fp && home[0]) {
		sprintf(temp, "%s/.scan.%s", home, fn);
		fp = fopen(temp, "r");
	}
	if (!fp) {
		ptext("Unable to open scanfile: ");
		ptextln(temp);
		bye_kids(1);
		return;
	}
	/*
	   scan file format:
	
	<down value> <up value> <name>
	
	values are in hex. <name> may be any string of characters
	
	*/
	scan_up = (char **) malloc(sizeof(char *) * MAX_SCAN);
	scan_down = (char **) malloc(sizeof(char *) * MAX_SCAN);
	scan_name = (char **) malloc(sizeof(char *) * MAX_SCAN);
	scan_tested = (int *) malloc(sizeof(int *) * MAX_SCAN);
	scan_length = (int *) malloc(sizeof(int *) * MAX_SCAN);
	scan_value = (int *) malloc(sizeof(int *) * MAX_SCAN);
	scan_up[0] = scan_down[0] = scan_name[0] = (char *) 0;
	str = (char *) malloc(4096);	/* buffer space */
	sl = str + 4000;	/* an upper limit */
	scan_max = 1;
	for (i = 0;;) {
		for (s = str; (ch = getc(fp)) != EOF;) {
			if (ch == '\n' || ch == '\r')
				break;
			*s++ = ch;
		}
		*s++ = '\0';
		if (ch == EOF)
			break;
		if (*str == '#' || *str == '\0')
			continue;
		scan_down[i] = smash();
		scan_blanks();
		scan_up[i] = smash();
		scan_blanks();
		scan_name[i] = str;

		scan_length[i] = strlen(scan_down[i]);
		ch = strlen(scan_up[i]) + scan_length[i];
		if (ch > scan_max)
			scan_max = ch;

		scan_value[i] = scan_name[i][0];
		if (scan_name[i][1])	/* multi-character name */
			for (j = 0; scan_special[j].name; j++) {
				if (!strcmp(scan_name[i], scan_special[j].name)) {
					scan_value[i] = scan_special[j].type;
					break;
				}
			}

		i++;
		if (str > sl) {
			str = (char *) malloc(4096);
			sl = str + 4000;
		} else
			str = s;
	}
	fclose(fp);
#ifdef notdef
	for (i = 0; scan_down[i]; i++) {
		put_str(hex_expand_to(scan_down[i], 3));
		put_str(hex_expand_to(scan_up[i], 3));
		put_str("   ");
		put_str(scan_name[i]);
		put_crlf();
	}
	(void) wait_here();
#endif
}

int
scan_key(void)
{				/* read a key and translate scan mode to
				   ASCII */
	int i, j, ch;
	char buf[64];

	for (i = 1;; i++) {
		ch = getchar();
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
		buf[i - 1] = ch;
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
				ch = scan_value[j];
				if (ch == CAPS_LOCK)
					shift_state ^= SHIFT_KEY;
				if (ch >= 256)
					break;
				if (shift_state & SHIFT_KEY)
					if (ch >= 0x60)
						ch -= 0x20;
					else if (ch >= 0x30 && ch <= 0x3f)
						ch -= 0x10;
				if (shift_state & CONTROL_KEY)
					if ((ch | 0x20) >= 0x60 &&
						(ch | 0x20) <= 0x7f)
						ch = (ch | 0x20) - 0x60;
				if (shift_state & META_KEY)
					ch |= 0x80;
				return ch;
			}
		}
		if (i > scan_max)
			i = 1;
	}
}
