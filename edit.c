/*
** This software is Copyright (c) 1997 by Daniel Weaver.
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>

#if     GCC_PRINTF
#define GCC_PRINTFLIKE(fmt,var) __attribute__((format(printf,fmt,var)))
#else
#define GCC_PRINTFLIKE(fmt,var) /*nothing*/
#endif
#ifndef GCC_NORETURN
#define GCC_NORETURN /* nothing */
#endif

#include "tic.h"
#include "tac.h"

/*
 * Terminfo edit features

Edit menu:
i) show current terminfo
t) show caps tested
c) show caps changed
u) show caps that are defined but cannot be tested
v) display single cap
e) edit cap (string, boolean or number)
p) change padding on cap
w) write modified terminfo to file

 */
static void show_info(struct test_list *, int *, int *);
static void show_value(struct test_list *, int *, int *);
static void show_untested(struct test_list *, int *, int *);
static void show_changed(struct test_list *, int *, int *);

#define SHOW_VALUE	1
#define SHOW_EDIT	2
#define SHOW_DELETE	3

struct test_list edit_test_list[] = {
	{MENU_CLEAR, 0, 0, 0, "i) display current terminfo", show_info, 0},
	{0, 0, 0, 0, "w) write the current terminfo to a file", save_info, 0},
	{SHOW_VALUE, 3, 0, 0, "v) show value of a selected cap", show_value, 0},
	{SHOW_EDIT, 4, 0, 0, "e) edit value of a selected cap", show_value, 0},
	{SHOW_DELETE, 3, 0, 0, "d) delete string", show_value, 0},
	{0, 3, 0, 0, "m) show caps that have been modified", show_changed, 0},
	{MENU_CLEAR + FLAG_CAN_TEST, 0, 0, 0, "c) show caps that can be tested", show_report, 0},
	{MENU_CLEAR + FLAG_TESTED, 0, 0, 0, "t) show caps that have been tested", show_report, 0},
	{MENU_CLEAR + FLAG_FUNCTION_KEY, 0, 0, 0, "f) show a list of function keys", show_report, 0},
	{MENU_CLEAR, 0, 0, 0, "u) show caps defined that can not be tested", show_untested, 0},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

static char change_pad_text[MAX_CHANGES][80];
struct test_list change_pad_list[MAX_CHANGES] = {
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

extern void build_change_menu(struct test_menu *);
extern void change_one_entry(struct test_list *, int *, int *);

struct test_menu change_pad_menu = {
	0, "[q] > ", 0,
	"Select cap name", "change", 0,
	build_change_menu, change_pad_list, 0, 0, 0
};

static TERMTYPE	original_term;		/* terminal type description */

static char flag_boolean[BOOLCOUNT];	/* flags for booleans */
static char flag_numerics[NUMCOUNT];	/* flags for numerics */
static char flag_strings[STRCOUNT];	/* flags for strings */

static int start_display;		/* the display has just started */
static int display_lines;		/* number of lines displayed */

/* this deals with differences over whether 0x7f and 0x80..0x9f are controls */
#define CHAR_OF(s) (*(unsigned char *)(s))
#define REALCTL(s) (CHAR_OF(s) < 127 && iscntrl(CHAR_OF(s)))
#define REALPRINT(s) (CHAR_OF(s) < 127 && isprint(CHAR_OF(s)))

char *
file_expand(char *srcp)
{
	static char	buffer[1024];
	int		bufp;
	char		*ptr, *str = srcp;
	bool		islong = (strlen(str) > 3);

    	bufp = 0;
    	ptr = str;
    	while (*str) {
		if (*str == '%' && REALPRINT(str+1)) {
	    		buffer[bufp++] = *str++;
	    		buffer[bufp++] = *str;
		}
		else if (*str == '\033') {
	    		buffer[bufp++] = '\\';
	    		buffer[bufp++] = 'E';
		}
		else if (*str == '\\') {
	    		buffer[bufp++] = '\\';
	    		buffer[bufp++] = '\\';
		}
		else if (*str == ' ') {
	    		buffer[bufp++] = '\\';
	    		buffer[bufp++] = 's';
		}
		else if (*str == ',' || *str == ':' || *str == '^') {
	    		buffer[bufp++] = '\\';
	    		buffer[bufp++] = *str;
		}
		else if (REALPRINT(str))
		    	buffer[bufp++] = *str;
		else if (*str == '\r' && (islong || (strlen(srcp) > 2 && str[1] == '\0'))) {
	    		buffer[bufp++] = '\\';
	    		buffer[bufp++] = 'r';
		}
		else if (*str == '\n' && islong) {
	    		buffer[bufp++] = '\\';
	    		buffer[bufp++] = 'n';
		}
#define UnCtl(c) ((0xff & (c)) + '@')
		else if (REALCTL(str) && *str != '\\' && (!islong || isdigit(str[1]))) {
			(void) sprintf(&buffer[bufp], "^%c", UnCtl(*str));
			bufp += 2;
		} else {
			(void) sprintf(&buffer[bufp], "\\%03o", 0xff & *str);
			bufp += 4;
		}
		str++;
    	}
    	buffer[bufp] = '\0';
    	return(buffer);
}


static char *bufptr;		/* otherwise, the input buffer pointer */
static char *bufstart;		/* start of buffer so we can compute offsets */
static char separator;		/* capability separator */

/*
 *	_nc_reset_input()
 *
 *	Resets the input-reading routines.  Used on initialization,
 *	or after a seek has been done.  Exactly one argument must be
 *	non-null.
 */

static void
_tak_reset_input(FILE *fp, char *buf)
{
	bufstart = bufptr = buf;
}

/*
 *	int next_char()
 *
 *	Returns the next character in the input stream.  Comments and leading
 *	white space are stripped.
 *
 *	The global state variable 'firstcolumn' is set TRUE if the character
 *	returned is from the first column of the input line.
 *
 *	The global variable _nc_curr_line is incremented for each new line.
 *	The global variable _nc_curr_file_pos is set to the file offset of the
 *	beginning of each line.
 */

static int
next_char(void)
{
	if (*bufptr == '\0')
	    return(EOF);
    return(*bufptr++);
}

static void push_back(char c)
/* push a character back onto the input stream */
{
    if (bufptr == bufstart)
	    _nc_syserr_abort("Can't backspace off beginning of line");
    *--bufptr = c;
}

/*
 *	char
 *	trans_string(ptr)
 *
 *	Reads characters using next_char() until encountering a separator, nl,
 *	or end-of-file.  The returned value is the character which caused
 *	reading to stop.  The following translations are done on the input:
 *
 *		^X  goes to  ctrl-X (i.e. X & 037)
 *		{\E,\n,\r,\b,\t,\f}  go to
 *			{ESCAPE,newline,carriage-return,backspace,tab,formfeed}
 *		{\^,\\}  go to  {carat,backslash}
 *		\ddd (for ddd = up to three octal digits)  goes to the character ddd
 *
 *		\e == \E
 *		\0 == \200
 *
 */

static char
_nc_trans_string(char *ptr)
{
	int	count = 0;
	int	number;
	int	i, c;
	chtype	ch, last_ch = '\0';

	while ((ch = c = next_char()) != (chtype)separator && c != EOF) {
	    if ((_nc_syntax == SYN_TERMCAP) && c == '\n')
		break;
	    if (ch == '^' && last_ch != '%') {
		ch = c = next_char();
		if (c == EOF)
		    _nc_err_abort("Premature EOF");

#ifdef THIS_IS_NOT_NEEDED
		if (! (is7bits(ch) && isprint(ch))) {
		    _nc_warning("Illegal ^ character - %s",
			_tracechar((unsigned char)ch));
		}
#endif
		if (ch == '?')
		    *(ptr++) = '\177';
		else
		    *(ptr++) = (char)(ch & 037);
	    }
	    else if (ch == '\\') {
		ch = c = next_char();
		if (c == EOF)
		    _nc_err_abort("Premature EOF");

		if (ch >= '0'  &&  ch <= '7') {
		    number = ch - '0';
		    for (i=0; i < 2; i++) {
			ch = c = next_char();
			if (c == EOF)
			    _nc_err_abort("Premature EOF");

			if (c < '0'  ||  c > '7') {
			    if (isdigit(c)) {
				_nc_warning("Non-octal digit `%c' in \\ sequence", c);
				/* allow the digit; it'll do less harm */
			    } else {
				push_back((char)c);
				break;
			    }
			}

			number = number * 8 + c - '0';
		    }

		    if (number == 0)
			number = 0200;
		    *(ptr++) = (char) number;
		} else {
		    switch (c) {
			case 'E':
			case 'e':	*(ptr++) = '\033';	break;

			case 'l':
			case 'n':	*(ptr++) = '\n';	break;

			case 'r':	*(ptr++) = '\r';	break;

			case 'b':	*(ptr++) = '\010';	break;

			case 's':	*(ptr++) = ' ';		break;

			case 'f':	*(ptr++) = '\014';	break;

			case 't':	*(ptr++) = '\t';	break;

			case '\\':	*(ptr++) = '\\';	break;

			case '^':	*(ptr++) = '^'; 	break;

			case ',':	*(ptr++) = ',';		break;

			case ':':	*(ptr++) = ':';		break;

			case '\n':
			    continue;

			default:
			    _nc_warning("Illegal character %s in \\ sequence",
				    _tracechar((unsigned char)ch));
			    *(ptr++) = (char)ch;
		    } /* endswitch (ch) */
		} /* endelse (ch < '0' ||  ch > '7') */
	    } /* end else if (ch == '\\') */
	    else {
		*(ptr++) = (char)ch;
	    }

	    count ++;

	    last_ch = ch;

	    if (count > 1024)
		_nc_warning("Very long string found.  Missing separator?");
	} /* end while */

	*ptr = '\0';

	return(ch);
}

/*
**	send_info_string(str)
**
**	Return the terminfo string prefixed by the correct seperator
*/
static void
send_info_string(
	char *str,
	int *ch)
{
	int len;

	if (display_lines == -1) {
		return;
	}
	len = strlen(str);
	if (len + char_count + 3 >= columns) {
		if (start_display == 0) {
			put_str(",");
		}
		put_crlf();
		if (++display_lines > lines) {
			ptext("-- more -- ");
			*ch = wait_here();
			if (*ch != '\r' && *ch != '\n') {
				display_lines = -1;
				return;
			}
			display_lines = 0;
		}
		if (len >= columns) {
			/* if the terminal does not (am) them this loses */
			if (columns) {
				display_lines += ((strlen(str) + 3) / columns) + 1;
			}
			put_str("   ");
			put_str(str);
			start_display = 0;
			return;
		}
		ptext("   ");
	} else
	if (start_display == 0) {
		ptext(", ");
	} else {
		ptext("   ");
	}
	ptext(str);
	start_display = 0;
}

/*
**	show_info(test_list, status, ch)
**
**	Display the current terminfo
*/
static void
show_info(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;
	char buf[1024];

	display_lines = 1;
	start_display = 1;
	for (i = 0; i < BOOLCOUNT; i++) {
		if (CUR Booleans[i]) {
			send_info_string(boolnames[i], ch);
		}
	}
	for (i = 0; i < NUMCOUNT; i++) {
		if (CUR Numbers[i] >= 0) {
			sprintf(buf, "%s#%d", numnames[i], CUR Numbers[i]);
			send_info_string(buf, ch);
		}
	}
	for (i = 0; i < STRCOUNT; i++) {
		if (CUR Strings[i]) {
			sprintf(buf, "%s=%s", strnames[i],
				print_expand(CUR Strings[i]));
			send_info_string(buf, ch);
		}
	}
	put_crlf();
	generic_done_message(t, state, ch);
}

/*
**	save_info_string(str, fp)
**
**	Write the terminfo string prefixed by the correct seperator
*/
static void
save_info_string(
	char *str,
	FILE *fp)
{
	int len;

	len = strlen(str);
	if (len + display_lines >= 77) {
		if (display_lines > 0) {
			(void) fprintf(fp, "\n\t");
		}
		display_lines = 8;
	} else
	if (display_lines > 0) {
		(void) fprintf(fp, " ");
		display_lines++;
	} else {
		(void) fprintf(fp, "\t");
		display_lines = 8;
	}
	(void) fprintf(fp, "%s,", str);
	display_lines += len + 1;
}

/*
**	save_info(test_list, status, ch)
**
**	Write the current terminfo to a file
*/
void
save_info(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;
	FILE *fp;
	time_t now;
	char buf[1024];

	if ((fp = fopen(tty_basename, "w")) == (FILE *) NULL) {
		(void) sprintf(temp, "can't open: %s", tty_basename);
		ptextln(temp);
		generic_done_message(t, state, ch);
		return;
	}
	time(&now);
	/* Note: ctime() returns a newline at the end of the string */
	(void) fprintf(fp, "# Terminfo created by TAK for TERM=%s on %s",
		tty_basename, ctime(&now));
	(void) fprintf(fp, "%s|%s,\n", tty_basename, longname());

	display_lines = 0;
	for (i = 0; i < BOOLCOUNT; i++) {
		if (CUR Booleans[i]) {
			save_info_string(boolnames[i], fp);
		}
	}
	for (i = 0; i < NUMCOUNT; i++) {
		if (CUR Numbers[i] >= 0) {
			sprintf(buf, "%s#%d", numnames[i], CUR Numbers[i]);
			save_info_string(buf, fp);
		}
	}
	for (i = 0; i < STRCOUNT; i++) {
		if (CUR Strings[i]) {
			sprintf(buf, "%s=%s", strnames[i],
				file_expand(CUR Strings[i]));
			save_info_string(buf, fp);
		}
	}
	(void) fprintf(fp, "\n");
	(void) fclose(fp);
	sprintf(temp, "Terminfo saved as file: %s", tty_basename);
	ptextln(temp);
}

/*
**	show_value(test_list, status, ch)
**
**	Display the value of a selected cap
*/
static void
show_value(
	struct test_list *t,
	int *state,
	int *ch)
{
	struct name_table_entry const *nt;
	char *s;
	int n, op;
	char buf[1024];
	char new[1024];

	ptext("enter name: ");
	read_string(buf, 80);
	if (buf[0] == '\0' || buf[1] == '\0') {
		*ch = buf[0];
		return;
	}
	if (line_count + 2 >= lines) {
		put_clear();
	}
	op = t->flags & 255;
	if ((nt = _nc_find_entry(buf, _nc_info_hash_table))) {
		switch (nt->nte_type) {
		case BOOLEAN:
			if (op == SHOW_DELETE) {
				CUR Booleans[nt->nte_index] = 0;
				return;
			}
			sprintf(temp, "boolean  %s %s", buf,
				CUR Booleans[nt->nte_index] ? "True" : "False");
			break;
		case STRING:
			if (op == SHOW_DELETE) {
				CUR Strings[nt->nte_index] = (char *) 0;
				return;
			}
			if (CUR Strings[nt->nte_index]) {
				sprintf(temp, "string  %s %s", buf,
					expand(CUR Strings[nt->nte_index]));
			} else {
				sprintf(temp, "undefined string %s", buf);
			}
			break;
		case NUMBER:
			if (op == SHOW_DELETE) {
				CUR Numbers[nt->nte_index] = -1;
				return;
			}
			sprintf(temp, "numeric  %s %d", buf,
				CUR Numbers[nt->nte_index]);
			break;
		default:
			sprintf(temp, "unknown");
			break;
		}
		ptextln(temp);
	} else {
		sprintf(temp, "Cap not found: %s", buf);
		ptextln(temp);
		return;
	}
	if (op != SHOW_EDIT) {
		return;
	}
	if (nt->nte_type == BOOLEAN) {
		ptextln("Value flipped");
		CUR Booleans[nt->nte_index] = !CUR Booleans[nt->nte_index];
		return;
	}
	ptextln("Enter new value");
	read_string(buf, sizeof(buf));

	switch (nt->nte_type) {
	case STRING:
		_tak_reset_input((FILE *) 0, buf);
		_nc_trans_string(new);
		s = malloc(strlen(new) + 1);
		strcpy(s, new);
		CUR Strings[nt->nte_index] = s;
		sprintf(temp, "new string value  %s", nt->nte_name);
		ptextln(temp);
		ptextln(expand(CUR Strings[nt->nte_index]));
		break;
	case NUMBER:
		if (sscanf(buf, "%d", &n) == 1) {
			CUR Numbers[nt->nte_index] = n;
			sprintf(temp, "new numeric value  %s %d",
				nt->nte_name, n);
			ptextln(temp);
		} else {
			sprintf(temp, "Illegal number: %s", buf);
			ptextln(temp);
		}
		break;
	default:
		break;
	}
}

/*
**	get_string_cap_byname(name, long_name)
**
**	Given a cap name, find the value
**	Errors are quietly ignored.
*/
char *
get_string_cap_byname(
	char *name,
	char **long_name)
{
	struct name_table_entry const *nt;

	if ((nt = _nc_find_entry(name, _nc_info_hash_table))) {
		if (nt->nte_type == STRING) {
			*long_name = strfnames[nt->nte_index];
			return (CUR Strings[nt->nte_index]);
		}
	}
	*long_name = "??";
	return (char *) 0;
}

/*
**	get_string_cap_byvalue(value)
**
**	Given a capability string, find its position in the data base.
**	Return the index or -1 if not found.
*/
int
get_string_cap_byvalue(
	char *value)
{
	int i;

	if (value) {
		for (i = 0; i < STRCOUNT; i++) {
			if (CUR Strings[i] == value) {
				return i;
			}
		}
	}
	return -1;
}

/*
**	show_changed(test_list, status, ch)
**
**	Display a list of caps that have been changed.
*/
static void
show_changed(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, header = 1;
	char *a, *b;
	static char title[] = "                     old value   cap  new value";
	char abuf[1024];

	for (i = 0; i < BOOLCOUNT; i++) {
		if (original_term.Booleans[i] != CUR Booleans[i]) {
			if (header) {
				ptextln(title);
				header = 0;
			}
			sprintf(temp, "%30d %6s %d",
				original_term.Booleans[i], boolnames[i],
				CUR Booleans[i]);
			ptextln(temp);
		}
	}
	for (i = 0; i < NUMCOUNT; i++) {
		if (original_term.Numbers[i] != CUR Numbers[i]) {
			if (header) {
				ptextln(title);
				header = 0;
			}
			sprintf(temp, "%30d %6s %d",
				original_term.Numbers[i], numnames[i],
				CUR Numbers[i]);
			ptextln(temp);
		}
	}
	for (i = 0; i < STRCOUNT; i++) {
		a = original_term.Strings[i] ? original_term.Strings[i] : "";
		b = CUR Strings[i] ?  CUR Strings[i] : "";
		if (strcmp(a, b)) {
			if (header) {
				ptextln(title);
				header = 0;
			}
			strcpy(abuf, file_expand(a));
			sprintf(temp, "%30s %6s %s", abuf, strnames[i],
				file_expand(b));
			putln(temp);
		}
	}
	if (header) {
		ptextln("No changes");
	}
	put_crlf();
	generic_done_message(t, state, ch);
}

/*
**	user_modified()
**
**	Return TRUE if the user has modified the terminfo
*/
int
user_modified(void)
{
	char *a, *b;
	int i;

	for (i = 0; i < BOOLCOUNT; i++) {
		if (original_term.Booleans[i] != CUR Booleans[i]) {
			return TRUE;
		}
	}
	for (i = 0; i < NUMCOUNT; i++) {
		if (original_term.Numbers[i] != CUR Numbers[i]) {
			return TRUE;
		}
	}
	for (i = 0; i < STRCOUNT; i++) {
		a = original_term.Strings[i] ? original_term.Strings[i] : "";
		b = CUR Strings[i] ?  CUR Strings[i] : "";
		if (strcmp(a, b)) {
			return TRUE;
		}
	}
	return FALSE;
}

/*****************************************************************************
 *
 * Maintain the list of capabilities that can be tested
 *
 *****************************************************************************/

/*
**	mark_cap(name, flag)
**
**	Mark the cap data base with the flag provided.
*/
static void
mark_cap(
	char *name,
	int flag)
{
	struct name_table_entry const *nt;

	if ((nt = _nc_find_entry(name, _nc_info_hash_table))) {
		switch (nt->nte_type) {
		case BOOLEAN:
			flag_boolean[nt->nte_index] |= flag;
			break;
		case STRING:
			flag_strings[nt->nte_index] |= flag;
			break;
		case NUMBER:
			flag_numerics[nt->nte_index] |= flag;
			break;
		default:
			sprintf(temp, "unknown cap type (%s)", name);
			ptextln(temp);
			break;
		}
	} else {
		sprintf(temp, "Cap not found: %s", name);
		ptextln(temp);
	}
}

/*
**	can_test(name-list, flags)
**
**	Scan the name list and get the names.
**	Enter each name into the can-test data base.
**	<space> ( and ) may be used as seperators.
*/
void
can_test(
	char *s,
	int flags)
{
	int ch, i, j;
	char name[32];

	if (s) {
		for (i = j = 0; (name[j] = ch = *s); s++) {
			if (ch == ' ' || ch == ')' || ch == '(') {
				if (j) {
					name[j] = '\0';
					mark_cap(name, flags);
				}
				j = 0;
			} else {
				j++;
			}
		}
		if (j) {
			mark_cap(name, flags);
		}
	}
}

/*
**	show_report(test_list, status, ch)
**
**	Display a list of caps that can be tested
*/
void
show_report(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j, nc, flag;
	char *s;
	char *nx[BOOLCOUNT + NUMCOUNT + STRCOUNT];

	flag = t->flags & 255;
	nc = 0;
	for (i = 0; i < BOOLCOUNT; i++) {
		if (flag_boolean[i] & flag) {
			nx[nc++] = boolnames[i];
		}
	}
	for (i = 0; i < NUMCOUNT; i++) {
		if (flag_numerics[i] & flag) {
			nx[nc++] = numnames[i];
		}
	}
	for (i = 0; i < STRCOUNT; i++) {
		if (flag_strings[i] & flag) {
			nx[nc++] = strnames[i];
		}
	}
	/* sort */
	for (i = 0; i < nc - 1; i++) {
		for (j = i + 1; j < nc; j++) {
			if (strcmp(nx[i], nx[j]) > 0) {
				s = nx[i];
				nx[i] = nx[j];
				nx[j] = s;
			}
		}
	}
	if (flag & FLAG_FUNCTION_KEY) {
		ptextln("The following function keys can be tested:");
	} else
	if (flag & FLAG_CAN_TEST) {
		ptextln("The following capabilities can be tested:");
	} else
	if (flag & FLAG_TESTED) {
		ptextln("The following capabilities have been tested:");
	}
	put_crlf();
	for (i = 0; i < nc; i++) {
		sprintf(temp, "%s ", nx[i]);
		ptext(temp);
	}
	put_crlf();
	put_crlf();
	generic_done_message(t, state, ch);
}

/*
**	show_untested(test_list, status, ch)
**
**	Display a list of caps that are defined but cannot be tested.
**	Don't bother to sort this list.
*/
static void
show_untested(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;

	for (i = 0; i < BOOLCOUNT; i++) {
		if (flag_boolean[i] == 0 && CUR Booleans[i]) {
			sprintf(temp, "%s ", boolnames[i]);
			ptext(temp);
		}
	}
	for (i = 0; i < NUMCOUNT; i++) {
		if (flag_numerics[i] == 0 && CUR Numbers[i] >= 0) {
			sprintf(temp, "%s ", numnames[i]);
			ptext(temp);
		}
	}
	for (i = 0; i < STRCOUNT; i++) {
		if (flag_strings[i] == 0 && CUR Strings[i]) {
			sprintf(temp, "%s ", strnames[i]);
			ptext(temp);
		}
	}
	put_crlf();
	put_crlf();
	generic_done_message(t, state, ch);
}

/*
**	edit_init()
**
**	Initialize the function key data base
*/
void
edit_init(void)
{
	int i, j, lc;
	char *lab;
	int label_strings[STRCOUNT];

	for (i = 0; i < BOOLCOUNT; i++) {
		original_term.Booleans[i] = CUR Booleans[i];
	}
	for (i = 0; i < NUMCOUNT; i++) {
		original_term.Numbers[i] = CUR Numbers[i];
	}
	/* scan for labels */
	for (i = lc = 0; i < STRCOUNT; i++) {
		original_term.Strings[i] = CUR Strings[i];
		if (strncmp(strnames[i], "lf", 2) == 0) {
			flag_strings[i] |= FLAG_LABEL;
			if (CUR Strings[i]) {
				label_strings[lc++] = i;
			}
		}
	}
	/* scan for function keys */
	for (i = 0; i < STRCOUNT; i++) {
		if ((strnames[i][0] == 'k') && strcmp(strnames[i], "kmous")) {
			flag_strings[i] |= FLAG_FUNCTION_KEY;
			lab = (char *) 0;
			for (j = 0; j < lc; j++) {
				if (!strcmp(&strnames[i][1],
					&strnames[label_strings[j]][1])) {
					lab = CUR Strings[label_strings[j]];
					break;
				}
			}
			enter_key(strnames[i], CUR Strings[i], lab);
		}
	}
}

/*
**	change_one_entry(test_list, status, ch)
**
**	Change the padding on the selected cap
*/
void
change_one_entry(
	struct test_list *test,
	int *state,
	int *chp)
{
	struct name_table_entry const *nt;
	int i, j, x, star, slash,  v, dot, ch;
	char *s, *t, *p;
	char buf[1024];
	char pad[32];

	i = test->flags & 255;
	if (i == 255) {
		/* read the cap name from the user */
		ptext("enter name: ");
		read_string(pad, 32);
		if (pad[0] == '\0' || pad[1] == '\0') {
			*chp = pad[0];
			return;
		}
		if ((nt = _nc_find_entry(pad, _nc_info_hash_table)) &&
			(nt->nte_type == STRING)) {
			x = nt->nte_index;
		} else {
			sprintf(temp, "%s is not a string capability", pad);
			ptext(temp);
			generic_done_message(test, state, chp);
			return;
		}
	} else {
		x = tx_index[i];
		strcpy(pad, strnames[x]);
	}
	sprintf(buf, "Current value: (%s) %s", pad, file_expand(CUR Strings[x]));
	putln(buf);
	ptextln("Enter new pad.  0 for no pad.  CR for no change.");
	read_string(buf, 32);
	if (buf[0] == '\0' || buf[1] == '\0') {
		*chp = buf[0];
		return;
	}
	star = slash = FALSE;
	for (j = v = dot = 0; (ch = buf[j]); j++) {
		if (ch >= '0' && ch <= '9') {
			v = ch - '0' + v * 10;
			if (dot) {
				dot++;
			}
		} else if (ch == '*') {
			star = TRUE;
		} else if (ch == '/') {
			slash = TRUE;
		} else if (ch == '.') {
			dot = 1;
		} else {
			sprintf(temp, "Illegal character: %c", ch);
			ptextln(temp);
			ptext("General format:  99.9*/  ");
			generic_done_message(test, state, chp);
			return;
		}
	}
	while (dot > 2) {
		v /= 10;
		dot--;
	}
	if (dot == 2) {
		sprintf(pad, "%d.%d%s%s", v / 10, v % 10,
				star ? "*" : "", slash ? "/" : "");
	} else {
		sprintf(pad, "%d%s%s",
			v, star ? "*" : "", slash ? "/" : "");
	}
	s = CUR Strings[x];
	t = buf;
	for (v = 0; (ch = *t = *s++); t++) {
		if (v == '$' && ch == '<') {
			while ((ch = *s++) && (ch != '>'));
			for (p = pad; (*++t = *p++); );
			*t++ = '>';
			while ((*t++ = *s++));
			pad[0] = '\0';
			break;
		}
		v = ch;
	}
	if (pad[0]) {
		sprintf(t, "$<%s>", pad);
	}
	if ((s = malloc(strlen(buf) + 1))) {
		strcpy(s, buf);
		CUR Strings[x] = s;
		if (i != 255) {
			tx_cap[i] = s;
		}
	}
	generic_done_message(test, state, chp);
}

/*
**	build_change_menu(menu_list)
**
**	Build the change pad menu list
*/
void
build_change_menu(
	struct test_menu *m)
{
	int i, j, k;
	char *s;

	for (i = j = 0; i < txp; i++) {
		if ((k = get_string_cap_byvalue(tx_cap[i])) >= 0) {
			tx_index[i] = k;
			s = file_expand(tx_cap[i]);
			s[40] = '\0';
			sprintf(change_pad_text[j], "%c) (%s) %s",
				'a' + j, strnames[k], s);
			change_pad_list[j].flags = i;
			change_pad_list[j].lines_needed = 4;
			change_pad_list[j].menu_entry = change_pad_text[j];
			change_pad_list[j].test_procedure = change_one_entry;
			j++;
		}
	}
	strcpy(change_pad_text[j], "z) enter name");
	change_pad_list[j].flags = 255;
	change_pad_list[j].lines_needed = 4;
	change_pad_list[j].menu_entry = change_pad_text[j];
	change_pad_list[j].test_procedure = change_one_entry;
	j++;
	change_pad_list[j].flags = MENU_LAST;
	if (m->menu_title) {
		ptextln(m->menu_title);
	}
}