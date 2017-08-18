/*
** Copyright (C) 1997-2012,2017 Free Software Foundation, Inc.
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
#include <time.h>

#include <tack.h>

MODULE_ID("$Id: edit.c,v 1.42 2017/08/18 16:15:44 tom Exp $")

/*
 * These are adapted from tic.h
 */
typedef struct {
    const char *nt_name;
    NAME_TYPE nt_type;
    int nt_index;
} NAME_TABLE;

#define NAME_ENTRY_DATA 1

/*
 * Terminfo edit features
 */
#if TACK_CAN_EDIT

#define SHOW_VALUE	1
#define SHOW_EDIT	2
#define SHOW_DELETE	3

static char change_pad_text[MAX_CHANGES][80];
static TestList change_pad_list[MAX_CHANGES] =
{
    {MENU_LAST, 0, 0, 0, 0, 0, 0}
};

static TERMTYPE original_term;	/* terminal type description */
#endif

static char *flag_boolean;	/* flags for booleans */
static char *flag_numbers;	/* flags for numerics */
static char *flag_strings;	/* flags for strings */
static int *label_strings;
static int xon_index;		/* Subscript for (xon) */
static int xon_shadow;

#if TACK_CAN_EDIT
static int start_display;	/* the display has just started */
static int display_lines;	/* number of lines displayed */
#endif

static void
alloc_arrays(void)
{
    if (flag_boolean == 0) {
	flag_boolean = (char *) calloc((size_t) MAX_BOOLEAN, sizeof(char));
    }
    if (flag_numbers == 0) {
	flag_numbers = (char *) calloc((size_t) MAX_NUMBERS, sizeof(char));
    }
    if (flag_strings == 0) {
	label_strings = (int *) calloc((size_t) MAX_STRINGS, sizeof(int));
	flag_strings = (char *) calloc((size_t) MAX_STRINGS, sizeof(char));
    }
}

static int
compare_capability(const void *a, const void *b)
{
    const NAME_TABLE *p = (const NAME_TABLE *) a;
    const NAME_TABLE *q = (const NAME_TABLE *) b;
    return strcmp(p->nt_name, q->nt_name);
}

#if (defined(HAVE_CURSES_DATA_BOOLNAMES) || defined(DECL_CURSES_DATA_BOOLNAMES))

#define DATA(index,name,type) { name,type,index }
static NAME_TABLE name_table[] =
{
#include <tackgen.h>
};
#undef DATA
#define SIZEOF_NAME_TABLE (sizeof(name_table) / sizeof(name_table[0]))
#define alloc_name_table()	/* nothing */

#else

static NAME_TABLE *name_table;
static size_t sizeof_name_table;
#define SIZEOF_NAME_TABLE sizeof_name_table

static int
compare_data(const void *a, const void *b)
{
    const NAME_TABLE *p = (const NAME_TABLE *) a;
    const NAME_TABLE *q = (const NAME_TABLE *) b;
    return strcmp(p->nt_name, q->nt_name);
}

static void
alloc_name_table(void)
{
    if (name_table == 0) {
	size_t s;
	size_t d = 0;
	sizeof_name_table = (size_t) (max_booleans + max_numbers + max_strings);
	name_table = calloc(sizeof_name_table, sizeof(NAME_TABLE));
	for (s = 0; s < max_booleans; ++s) {
	    name_table[d].nt_name = boolnames[s];
	    name_table[d].nt_type = BOOLEAN;
	    name_table[d].nt_index = (int) s;
	    ++d;
	}
	for (s = 0; s < max_numbers; ++s) {
	    name_table[d].nt_name = numnames[s];
	    name_table[d].nt_type = NUMBER;
	    name_table[d].nt_index = (int) s;
	    ++d;
	}
	for (s = 0; s < max_strings; ++s) {
	    name_table[d].nt_name = strnames[s];
	    name_table[d].nt_type = STRING;
	    name_table[d].nt_index = (int) s;
	    ++d;
	}
	qsort(name_table, sizeof_name_table, sizeof(NAME_TABLE), compare_data);
    }
}

#endif

static NAME_TABLE const *
find_capability(const char *name)
{
    NAME_TABLE key;
    NAME_TABLE *lookup;

    alloc_name_table();
    memset(&key, 0, sizeof(key));
    key.nt_name = name;
    lookup = bsearch(&key, name_table,
		     SIZEOF_NAME_TABLE,
		     sizeof(name_table[0]),
		     compare_capability);
    return lookup ? lookup : 0;
}

#if !TACK_CAN_EDIT
/*
 * This is used to relate array-index to capability name/type.
 */
static NAME_TABLE const *
find_cap_by_index(int capIndex, NAME_TYPE capType)
{
    static NAME_TABLE const *result = 0;
    size_t n;

    alloc_name_table();
    for (n = 0; n < SIZEOF_NAME_TABLE; ++n) {
	if (name_table[n].nt_index == capIndex
	    && name_table[n].nt_type == capType) {
	    result = &name_table[n];
	    break;
	}
    }
    return result;
}
#endif

static NAME_TABLE const *
find_string_cap_by_name(const char *name)
{
    NAME_TABLE const *result = find_capability(name);
    if (result != 0 && result->nt_type != STRING)
	result = 0;
    return result;
}

#if TACK_CAN_EDIT

#define set_saved_boolean(num, value) original_term.Booleans[num] = (char)value
#define set_saved_number(num, value)  original_term.Numbers[num]  = (short)value
#define set_saved_string(num, value)  original_term.Strings[num]  = value

#define get_saved_boolean(num)        original_term.Booleans[num]
#define get_saved_number(num)         original_term.Numbers[num]
#define get_saved_string(num)         original_term.Strings[num]

#define set_newer_boolean(num, value) CUR Booleans[num] = (char)value
#define set_newer_number(num, value)  CUR Numbers[num] = (short)value
#define set_newer_string(num, value)  CUR Strings[num] = value

#define get_newer_boolean(num)        CUR Booleans[num]
#define get_newer_number(num)         CUR Numbers[num]
#define get_newer_string(num)         CUR Strings[num]

#else

#define set_saved_boolean(num, value)	/* nothing */
#define set_saved_number(num, value)	/* nothing */
#define set_saved_string(num, value)	/* nothing */

#define set_newer_boolean(num, value)	/* nothing */
#define set_newer_number(num, value)	/* nothing */
#define set_newer_string(num, value)	/* nothing */

#if 0
static int
get_newer_boolean(int num)
{
    int result = 0;
    const NAME_TABLE *p = find_cap_by_index(num, BOOLEAN);
    if (p != 0)
	result = tigetflag((char *) p->nt_name);
    return result;
}

static int
get_newer_number(int num)
{
    int result = 0;
    const NAME_TABLE *p = find_cap_by_index(num, NUMBER);
    if (p != 0)
	result = tigetnum((char *) p->nt_name);
    return result;
}
#endif

static char *
get_newer_string(int num)
{
    char *result = 0;
    const NAME_TABLE *p = find_cap_by_index(num, STRING);
    if (p != 0)
	result = tigetstr((char *) p->nt_name);
    return result;
}

#endif

/*
**	get_string_cap_byname(name, long_name)
**
**	Given a cap name, find the value
**	Errors are quietly ignored.
*/
const char *
get_string_cap_byname(
			 const char *name,
			 const char **long_name)
{
    NAME_TABLE const *nt;

    if ((nt = find_string_cap_by_name(name)) != 0) {
#ifdef HAVE_CURSES_DATA_BOOLFNAMES
	*long_name = strfnames[nt->nt_index];
#endif
	return (get_newer_string(nt->nt_index));
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
			  const char *value)
{
    if (value) {
	int i;

	for (i = 0; i < (int) MAX_STRINGS; i++) {
	    /* FIXME - this implies ncurses... */
	    if (get_newer_string(i) == value) {
		return i;
	    }
	}
	/* search for translated strings */
	for (i = 0; i < TM_last; i++) {
	    if (TM_string[i].value == value) {
		return TM_string[i].index;
	    }
	}
    }
    return -1;
}

/*
**	user_modified()
**
**	Return TRUE if the user has modified the terminfo
*/
#if TACK_CAN_EDIT
int
user_modified(void)
{
    const char *a, *b;
    int i, v;

    for (i = 0; i < MAX_BOOLEAN; i++) {
	v = (i == xon_index) ? xon_shadow : get_newer_boolean(i);
	if (get_saved_boolean(i) != v) {
	    return TRUE;
	}
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	if (get_saved_number(i) != get_newer_number(i)) {
	    return TRUE;
	}
    }
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	if ((a = get_saved_string(i)) == 0)
	    a = "";
	if ((b = get_newer_string(i)) == 0)
	    b = "";
	if (strcmp(a, b)) {
	    return TRUE;
	}
    }
    return FALSE;
}
#endif

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
    NAME_TABLE const *nt;

    alloc_arrays();
    if ((nt = find_capability(name)) != 0) {
	switch (nt->nt_type) {
	case BOOLEAN:
	    flag_boolean[nt->nt_index] = ((char)
					  (flag_boolean[nt->nt_index]
					   | flag));
	    break;
	case STRING:
	    flag_strings[nt->nt_index] = ((char)
					  (flag_strings[nt->nt_index]
					   | flag));
	    break;
	case NUMBER:
	    flag_numbers[nt->nt_index] = ((char)
					  (flag_numbers[nt->nt_index]
					   | flag));
	    break;
	default:
	    sprintf(temp, "unknown cap type (%s)", name);
	    ptextln(temp);
	    break;
	}
    } else {
#ifdef HAVE_CURSES_DATA_BOOLNAMES
	sprintf(temp, "Cap not found: %s", name);
	ptextln(temp);
	(void) wait_here();
#endif
    }
}

/*
**	can_test(name-list, flags)
**
**	Scan the name list and get the names.
**	Enter each name into the can-test data base.
**	<space> ( and ) may be used as separators.
*/
void
can_test(
	    const char *s,
	    int flags)
{
    if (s) {
	int ch, j;
	char name[32];

	for (j = 0; (ch = name[j] = *s); s++) {
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
**	cap_index(name-list, index-list)
**
**	Scan the name list and return a list of indexes.
**	<space> ( and ) may be used as separators.
**	This list is terminated with -1.
*/
void
cap_index(
	     const char *s,
	     int *inx)
{
    if (s) {
	int j;
	char name[32];

	for (j = 0;; s++) {
	    int ch = name[j] = *s;
	    if (ch == ' ' || ch == ')' || ch == '(' || ch == 0) {
		if (j) {
		    NAME_TABLE const *nt;
		    name[j] = '\0';
		    if ((nt = find_string_cap_by_name(name)) != 0) {
			*inx++ = nt->nt_index;
		    }
		}
		if (ch == 0) {
		    break;
		}
		j = 0;
	    } else {
		j++;
	    }
	}
    }
    *inx = -1;
}

/*
**	cap_match(name-list, cap)
**
**	Scan the name list and see if the cap is in the list.
**	Return TRUE if we find an exact match.
**	<space> ( and ) may be used as separators.
*/
int
cap_match(
	     const char *names,
	     const char *cap)
{
    if (names) {
	int l = (int) strlen(cap);
	const char *s;

	while ((s = strstr(names, cap))) {
	    int c = (names == s) ? 0 : *(s - 1);
	    int t = s[l];
	    if ((c == 0 || c == ' ' || c == '(') &&
		(t == 0 || t == ' ' || t == ')')) {
		return TRUE;
	    }
	    if (t == 0) {
		break;
	    }
	    names = s + l;
	}
    }
    return FALSE;
}

/*
**	show_report(test_list, status, ch)
**
**	Display a list of caps that can be tested
*/
void
show_report(
	       TestList * t,
	       int *state GCC_UNUSED,
	       int *ch)
{
    int i, j, nc, flag;
    const char *s;
    size_t count = (size_t) (MAX_BOOLEAN + MAX_NUMBERS + MAX_STRINGS);
    const char **nx = (const char **) calloc(sizeof(const char *), count);

    alloc_arrays();
    flag = t->flags & 255;
    nc = 0;
    for (i = 0; i < MAX_BOOLEAN; i++) {
	if (flag_boolean[i] & flag) {
	    nx[nc++] = boolnames[i];
	}
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	if (flag_numbers[i] & flag) {
	    nx[nc++] = numnames[i];
	}
    }
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	if (flag_strings[i] & flag) {
	    nx[nc++] = STR_NAME(i);
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
    } else if (flag & FLAG_CAN_TEST) {
	ptextln("The following capabilities can be tested:");
    } else if (flag & FLAG_TESTED) {
	ptextln("The following capabilities have been tested:");
    }
    put_crlf();
    for (i = 0; i < nc; i++) {
	sprintf(temp, "%s ", nx[i]);
	ptext(temp);
    }
    put_newlines(1);
    *ch = REQUEST_PROMPT;
    free(nx);
}

#ifdef NCURSES_VERSION
#if TACK_CAN_EDIT
static size_t
safe_length(const char *value)
{
    size_t result = 0;
    if (VALID_STRING(value))
	result = strlen(value) + 1;
    return result;
}

static size_t
safe_copy(char *target, const char *source)
{
    size_t result = safe_length(source);
    if (result) {
	memcpy(target, source, result);
    }
    return result;
}

static void
copy_termtype(TERMTYPE *target, TERMTYPE *source)
{
    size_t need;
    int n;
#if NCURSES_XNAMES
    int num_Names = (source->ext_Booleans + source->ext_Numbers + source->ext_Strings);
#endif

    memset(target, 0, sizeof(*target));

#define copy_array(member,count) \
    target->member = calloc(count, sizeof(target->member[0])); \
    memcpy(target->member, source->member, count * sizeof(target->member[0]))

    copy_array(Booleans, MAX_BOOLEAN);
    copy_array(Numbers, MAX_NUMBERS);
    copy_array(Strings, MAX_STRINGS);

    need = safe_length(source->term_names);
    for (n = 0; n < STRCOUNT; ++n) {
	need += safe_length(source->Strings[n]);
    }

    if (need) {
	size_t have = 0;
	target->term_names =
	    target->str_table = malloc(need);
	have = safe_copy(target->term_names, source->term_names);
	for (n = 0; n < STRCOUNT; ++n) {
	    if (VALID_STRING(source->Strings[n])) {
		target->Strings[n] = target->str_table + have;
		have = safe_copy(target->Strings[n], source->Strings[n]);
	    }
	}
    }
#if NCURSES_XNAMES
    target->num_Booleans = source->num_Booleans;
    target->num_Numbers = source->num_Numbers;
    target->num_Strings = source->num_Strings;
    target->ext_Booleans = source->ext_Booleans;
    target->ext_Numbers = source->ext_Numbers;
    target->ext_Strings = source->ext_Strings;

    need = 0;
    for (n = 0; n < source->ext_Strings; ++n) {
	need += safe_length(source->ext_str_table + need);
    }
    for (n = 0; n < num_Names; ++n) {
	need += safe_length(source->ext_Names[n]);
    }
    if (need) {
	size_t have = 0;
	target->ext_Names = calloc((size_t) num_Names,
				   sizeof(target->ext_Names[0]));
	target->ext_str_table = malloc(need);
	for (n = STRCOUNT; n < source->num_Strings; ++n) {
	    if (safe_length(source->Strings[n])) {
		target->Strings[n] = target->ext_str_table + have;
		have += safe_copy(target->Strings[n], source->Strings[n]);
	    }
	}
	for (n = 0; n < num_Names; ++n) {
	    target->ext_Names[n] = target->ext_str_table + have;
	    have += safe_copy(target->ext_Names[n], source->ext_Names[n]);
	}
    }
#endif /* NCURSES_XNAMES */
}
#endif /* TACK_CAN_EDIT */

#if NO_LEAKS
static void
free_termtype(TERMTYPE *tp)
{
    free(tp->Booleans);
    free(tp->Numbers);
    free(tp->Strings);
    free(tp->str_table);
#if NCURSES_XNAMES
    free(tp->ext_str_table);
    free(tp->ext_Names);
#endif
    memset(tp, 0, sizeof(*tp));
}
#endif /* NO_LEAKS */
#endif /* NCURSES_VERSION */

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
    NAME_TABLE const *nt;

    alloc_arrays();

#if TACK_CAN_EDIT
    copy_termtype(&original_term, CUR_TP);
    for (i = 0; i < MAX_BOOLEAN; i++) {
	set_saved_boolean(i, get_newer_boolean(i));
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	set_saved_number(i, get_newer_number(i));
    }
#endif
    /* scan for labels */
    for (i = lc = 0; i < (int) MAX_STRINGS; i++) {
	set_saved_string(i, get_newer_string(i));
	if (strncmp(STR_NAME(i), "lf", (size_t) 2) == 0) {
	    flag_strings[i] |= FLAG_LABEL;
	    if (get_newer_string(i)) {
		label_strings[lc++] = i;
	    }
	}
    }
    /* scan for function keys */
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	const char *this_name = STR_NAME(i);
	if ((this_name[0] == 'k') && strcmp(this_name, "kmous")) {
	    flag_strings[i] |= FLAG_FUNCTION_KEY;
	    lab = (char *) 0;
	    for (j = 0; j < lc; j++) {
		if (!strcmp(this_name,
			    STR_NAME(label_strings[j]))) {
		    lab = get_newer_string(label_strings[j]);
		    break;
		}
	    }
	    enter_key(this_name, get_newer_string(i), lab);
	}
    }
    /* Lookup the translated strings */
    for (i = 0; i < TM_last; i++) {
	if ((nt = find_string_cap_by_name(TM_string[i].name)) != 0) {
	    TM_string[i].index = nt->nt_index;
	} else {
	    sprintf(temp, "TM_string lookup failed for: %s",
		    TM_string[i].name);
	    ptextln(temp);
	}
    }
    if ((nt = find_capability("xon")) != 0) {
	xon_index = nt->nt_index;
    }
    xon_shadow = xon_xoff;
    FreeIfNeeded(label_strings);
}

#if TACK_CAN_EDIT

/*
**	save_info_string(str, fp)
**
**	Write the terminfo string prefixed by the correct separator
*/
static void
save_info_string(
		    const char *str,
		    FILE *fp)
{
    int len;

    len = (int) strlen(str);
    if (len + display_lines >= 77) {
	if (display_lines > 0) {
	    (void) fprintf(fp, "\n\t");
	}
	display_lines = 8;
    } else if (display_lines > 0) {
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
 * This is adapted (and reduced) from ncurses' _nc_tic_expand function.
 */

/* this deals with differences over whether 0x7f and 0x80..0x9f are controls */
#define REALPRINT(s) (UChar(*(s)) < 127 && isprint(UChar(*(s))))

#define P_LIMIT(p)   (length - (size_t)(p))
#define L_BRACE '{'
#define S_QUOTE '\''
#define R_BRACE '}'

static char *
form_terminfo(const char *srcp)
{
    static char *buffer;
    static size_t length;

    int bufp;
    const char *str = VALID_STRING(srcp) ? srcp : "\0\0";
    size_t need = (2 + strlen(str)) * 4;
    int ch;

    if (srcp == 0) {
#if NO_LEAKS
	if (buffer != 0) {
	    free(buffer);
	    buffer = 0;
	    length = 0;
	}
#endif
	return 0;
    }
    if (buffer == 0 || need > length) {
	if ((buffer = (char *) realloc(buffer, length = need)) == 0) {
	    return 0;
	}
    }

    bufp = 0;
    while ((ch = UChar(*str)) != 0) {
	if (ch == '%' && REALPRINT(str + 1)) {
	    buffer[bufp++] = *str++;
	    /*
	     * If we have a "%{number}", try to translate it into a "%'char'"
	     * form, since that will run a little faster when we are
	     * interpreting it.  Having one form for the constant makes it
	     * simpler to compare terminal descriptions.
	     */
	    if (str[0] == L_BRACE
		&& isdigit(UChar(str[1]))) {
		char *dst = 0;
		long value = strtol(str + 1, &dst, 0);
		if (dst != 0
		    && *dst == R_BRACE
		    && value < 127
		    && value != '\\'
		    && isprint((int) value)) {
		    ch = (int) value;
		    buffer[bufp++] = S_QUOTE;
		    if (ch == '\\'
			|| ch == S_QUOTE)
			buffer[bufp++] = '\\';
		    buffer[bufp++] = (char) ch;
		    buffer[bufp++] = S_QUOTE;
		    str = dst;
		} else {
		    buffer[bufp++] = *str;
		}
	    } else {
		buffer[bufp++] = *str;
	    }
	} else if (ch == 128) {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = '0';
	} else if (ch == '\033') {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = 'E';
	} else if (ch == '\\' && (str == srcp || str[-1] != '^')) {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = '\\';
	} else if (ch == ' ' && (str == srcp || strcspn(str, " \t") == 0)) {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = 's';
	} else if (ch == ',' || ch == ':' || ch == '^') {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = (char) ch;
	} else if (REALPRINT(str)
		   && (ch != ','
		       && ch != ':'
		       && ch != '^')) {
	    buffer[bufp++] = (char) ch;
	} else if (ch == '\r') {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = 'r';
	} else if (ch == '\n') {
	    buffer[bufp++] = '\\';
	    buffer[bufp++] = 'n';
	} else if (UChar(ch) < 32
		   && isdigit(UChar(str[1]))) {
	    sprintf(&buffer[bufp], "^%c", ch + '@');
	    bufp += 2;
	} else {
	    sprintf(&buffer[bufp], "\\%03o", ch);
	    bufp += 4;
	}

	str++;
    }

    buffer[bufp] = '\0';

    return (buffer);
}

/*
**	save_info(test_list, status, ch)
**
**	Write the current terminfo to a file
*/
void
save_info(
	     TestList * t,
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
    (void) fprintf(fp, "# Terminfo created by TACK for TERM=%s on %s",
		   tty_basename, ctime(&now));
    (void) fprintf(fp, "%s|%s,\n", tty_basename, longname());

    display_lines = 0;
    for (i = 0; i < MAX_BOOLEAN; i++) {
	if (i == xon_index ? xon_shadow : get_newer_boolean(i)) {
	    save_info_string(boolnames[i], fp);
	}
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	if (get_newer_number(i) >= 0) {
	    sprintf(buf, "%s#%d", numnames[i], get_newer_number(i));
	    save_info_string(buf, fp);
	}
    }
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	const char *value = get_newer_string(i);
	if (value) {
	    sprintf(buf, "%s=%s", STR_NAME(i), form_terminfo(value));
	    save_info_string(buf, fp);
	}
    }
    (void) fprintf(fp, "\n");
    (void) fclose(fp);
    sprintf(temp, "Terminfo saved as file: %s", tty_basename);
    ptextln(temp);
}

/*
**	send_info_string(str)
**
**	Return the terminfo string prefixed by the correct separator
*/
static void
send_info_string(
		    const char *str,
		    int *ch)
{
    int len;

    if (display_lines == -1) {
	return;
    }
    len = (int) strlen(str);
    if (len + char_count + 3 >= columns) {
	if (start_display == 0) {
	    put_str(",");
	}
	put_crlf();
	if (++display_lines > lines) {
	    ptext("-- more -- ");
	    *ch = wait_here();
	    if (*ch == 'q') {
		display_lines = -1;
		return;
	    }
	    display_lines = 0;
	}
	if (len >= columns) {
	    /* if the terminal does not (am) then this loses */
	    if (columns) {
		display_lines += (((int) strlen(str) + 3) / columns) + 1;
	    }
	    put_str("   ");
	    put_str(str);
	    start_display = 0;
	    return;
	}
	ptext("   ");
    } else if (start_display == 0) {
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
	     TestList * t GCC_UNUSED,
	     int *state GCC_UNUSED,
	     int *ch)
{
    int i;
    char buf[1024];

    display_lines = 1;
    start_display = 1;
    for (i = 0; i < MAX_BOOLEAN; i++) {
	if ((i == xon_index) ? xon_shadow : get_newer_boolean(i)) {
	    send_info_string(boolnames[i], ch);
	}
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	if (get_newer_number(i) >= 0) {
	    sprintf(buf, "%s#%d", numnames[i], get_newer_number(i));
	    send_info_string(buf, ch);
	}
    }
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	char *value = get_newer_string(i);
	if (value) {
	    sprintf(buf, "%s=%s", STR_NAME(i), print_expand(value));
	    send_info_string(buf, ch);
	}
    }
    put_newlines(2);
    *ch = REQUEST_PROMPT;
}

/*
 * This is adapted (and reduced) from ncurses' _nc_trans_string function.
 *
 * Reads characters using next_char() until encountering a separator, nl,
 * or end-of-file.  The returned value is the character which caused
 * reading to stop.  The following translations are done on the input:
 *
 *	^X  goes to  ctrl-X (i.e. X & 037)
 *	{\E,\n,\r,\b,\t,\f}  go to
 *		{ESCAPE,newline,carriage-return,backspace,tab,formfeed}
 *	{\^,\\}  go to  {carat,backslash}
 *	\ddd (for ddd = up to three octal digits)  goes to the character ddd
 *
 *	\e == \E
 *	\0 == \200
 *
 */
#define next_char() UChar(*source++)

static void
scan_terminfo(const char *source, char *target, char *last)
{
    int number = 0;
    int i, c;
    int last_ch = '\0';

    while ((c = next_char()) != '\0') {
	if (target >= (last - 1)) {
	    if (c != '\0') {
		while ((c = next_char()) != ',' && c != '\0') {
		    ;
		}
	    }
	    break;
	}
	if (c == '^' && last_ch != '%') {
	    c = next_char();
	    if (c == '\0')
		goto error;

	    if (!(c <= 126 && isprint(c))) {
		fprintf(stderr, "Illegal ^ character - '%s'\n", unctrl(UChar(c)));
	    }
	    if (c == '?') {
		*(target++) = '\177';
	    } else {
		if ((c &= 037) == 0)
		    c = 128;
		*(target++) = (char) (c);
	    }
	} else if (c == '\\') {
	    c = next_char();
	    if (c == '\0')
		goto error;

	    if ((c >= '0') && (c <= '7')) {
		number = c - '0';
		for (i = 0; i < 2; i++) {
		    c = next_char();
		    if (c == '\0')
			goto error;

		    if ((c < '0') || (c > '7')) {
			if (isdigit(c)) {
			    fprintf(stderr,
				    "Non-octal digit `%c' in \\ sequence\n", c);
			    /* allow the digit; it'll do less harm */
			} else {
			    --source;
			    break;
			}
		    }

		    number = (number * 8) + (c - '0');
		}

		number = UChar(number);
		if (number == 0)
		    number = 0200;
		*(target++) = (char) number;
	    } else {
		switch (c) {
		case 'e':
		    /* FALLTHRU */
		case 'E':
		    c = '\033';
		    break;

		case 'n':
		    c = '\n';
		    break;

		case 'r':
		    c = '\r';
		    break;

		case 'b':
		    c = '\010';
		    break;

		case 'f':
		    c = '\014';
		    break;

		case 't':
		    c = '\t';
		    break;

		case 'a':
		    c = '\007';
		    break;

		case 'l':
		    c = '\n';
		    break;

		case 's':
		    c = ' ';
		    break;

		default:
		    if (strchr("^,|:\\", c) != 0)
			break;
		    fprintf(stderr,
			    "Illegal character '%s' in \\ sequence\n",
			    unctrl(UChar(c)));
		    /* FALLTHRU */
		case '\n':
		    /* ignored */
		    continue;
		}		/* endswitch (c) */
		*(target++) = (char) c;
	    }			/* endelse (c < '0' ||  c > '7') */
	} else {
	    *(target++) = (char) c;
	}
	last_ch = c;
    }				/* end while */

    *target = '\0';
    return;

  error:
    fprintf(stderr, "Expected more input\n");
    *target = '\0';
    return;
}

/*
**	show_value(test_list, status, ch)
**
**	Display the value of a selected cap
*/
static void
show_value(
	      TestList * t,
	      int *state GCC_UNUSED,
	      int *ch)
{
    NAME_TABLE const *nt;
    char *s;
    int n, op, b;
    char buf[1024];
    char tmp[1024];

    ptext("enter name: ");
    read_string(buf, (size_t) 80);
    if (buf[0] == '\0' || buf[1] == '\0') {
	*ch = buf[0];
	return;
    }
    if (line_count + 2 >= lines) {
	put_clear();
    }
    op = t->flags & 255;
    if ((nt = find_capability(buf)) != 0) {
	switch (nt->nt_type) {
	case BOOLEAN:
	    if (op == SHOW_DELETE) {
		if (nt->nt_index == xon_index) {
		    xon_shadow = 0;
		} else {
		    set_newer_boolean(nt->nt_index, 0);
		}
		return;
	    }
	    b = ((nt->nt_index == xon_index)
		 ? xon_shadow
		 : get_newer_boolean(nt->nt_index));
	    sprintf(temp, "boolean  %s %s", buf,
		    b ? "True" : "False");
	    break;
	case STRING:
	    if (op == SHOW_DELETE) {
		set_newer_string(nt->nt_index, 0);
		return;
	    }
	    if (get_newer_string(nt->nt_index)) {
		sprintf(temp, "string  %s %s", buf,
			expand(get_newer_string(nt->nt_index)));
	    } else {
		sprintf(temp, "undefined string %s", buf);
	    }
	    break;
	case NUMBER:
	    if (op == SHOW_DELETE) {
		set_newer_number(nt->nt_index, -1);
		return;
	    }
	    sprintf(temp, "numeric  %s %d", buf,
		    get_newer_number(nt->nt_index));
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
    if (nt->nt_type == BOOLEAN) {
	ptextln("Value flipped");
	if (nt->nt_index == xon_index) {
	    xon_shadow = !xon_shadow;
	} else {
	    set_newer_boolean(nt->nt_index,
			      !get_newer_boolean(nt->nt_index));
	}
	return;
    }
    ptextln("Enter new value");
    read_string(buf, sizeof(buf));

    switch (nt->nt_type) {
    case STRING:
	scan_terminfo(buf, tmp, tmp + sizeof(tmp));
	s = (char *) malloc(strlen(tmp) + 1);
	strcpy(s, tmp);
	set_newer_string(nt->nt_index, s);
	sprintf(temp, "new string value  %s", nt->nt_name);
	ptextln(temp);
	ptextln(expand(get_newer_string(nt->nt_index)));
	break;
    case NUMBER:
	if (sscanf(buf, "%d", &n) == 1) {
	    set_newer_number(nt->nt_index, n);
	    sprintf(temp, "new numeric value  %s %d",
		    nt->nt_name, n);
	    ptextln(temp);
	} else {
	    sprintf(temp, "Illegal number: %s", buf);
	    ptextln(temp);
	}
	break;
    case BOOLEAN:
    default:
	break;
    }
}

/*
**	show_untested(test_list, status, ch)
**
**	Display a list of caps that are defined but cannot be tested.
**	Don't bother to sort this list.
*/
static void
show_untested(
		 TestList * t GCC_UNUSED,
		 int *state GCC_UNUSED,
		 int *ch)
{
    int i;

    alloc_arrays();
    ptextln("Caps that are defined but cannot be tested:");
    for (i = 0; i < MAX_BOOLEAN; i++) {
	if (flag_boolean[i] == 0 && get_newer_boolean(i)) {
	    sprintf(temp, "%s ", boolnames[i]);
	    ptext(temp);
	}
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	if (flag_numbers[i] == 0 && get_newer_number(i) >= 0) {
	    sprintf(temp, "%s ", numnames[i]);
	    ptext(temp);
	}
    }
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	if (flag_strings[i] == 0 && get_newer_string(i)) {
	    sprintf(temp, "%s ", STR_NAME(i));
	    ptext(temp);
	}
    }
    put_newlines(1);
    *ch = REQUEST_PROMPT;
}

/*
**	show_changed(test_list, status, ch)
**
**	Display a list of caps that have been changed.
*/
static void
show_changed(
		TestList * t GCC_UNUSED,
		int *state GCC_UNUSED,
		int *ch)
{
    int i, header = 1, v;
    const char *a;
    const char *b;
    static char title[] = "                     old value   cap  new value";
    char abuf[1024];

    for (i = 0; i < MAX_BOOLEAN; i++) {
	v = (i == xon_index) ? xon_shadow : get_newer_boolean(i);
	if (get_saved_boolean(i) != v) {
	    if (header) {
		ptextln(title);
		header = 0;
	    }
	    sprintf(temp, "%30d %6s %d",
		    get_saved_boolean(i), boolnames[i], v);
	    ptextln(temp);
	}
    }
    for (i = 0; i < MAX_NUMBERS; i++) {
	if (get_saved_number(i) != get_newer_number(i)) {
	    if (header) {
		ptextln(title);
		header = 0;
	    }
	    sprintf(temp, "%30d %6s %d",
		    get_saved_number(i), numnames[i],
		    get_newer_number(i));
	    ptextln(temp);
	}
    }
    for (i = 0; i < (int) MAX_STRINGS; i++) {
	if ((a = get_saved_string(i)) == 0)
	    a = "";
	if ((b = get_newer_string(i)) == 0)
	    b = "";
	if (strcmp(a, b)) {
	    if (header) {
		ptextln(title);
		header = 0;
	    }
	    strcpy(abuf, form_terminfo(a));
	    sprintf(temp, "%30s %6s %s", abuf, STR_NAME(i), form_terminfo(b));
	    putln(temp);
	}
    }
    if (header) {
	ptextln("No changes");
    }
    put_crlf();
    *ch = REQUEST_PROMPT;
}

/*
**	change_one_entry(test_list, status, ch)
**
**	Change the padding on the selected cap
*/
static void
change_one_entry(
		    TestList * test,
		    int *state,
		    int *chp)
{
    NAME_TABLE const *nt;
    int i, j, x, star, slash, v, dot, ch;
    const char *s;
    char *t, *p;
    const char *current_string;
    char buf[1024];
    char pad[1024];

    i = test->flags & 255;
    if (i == 255) {
	/* read the cap name from the user */
	ptext("enter name: ");
	read_string(pad, (size_t) 32);
	if (pad[0] == '\0' || pad[1] == '\0') {
	    *chp = pad[0];
	    return;
	}
	if ((nt = find_string_cap_by_name(pad)) != 0) {
	    x = nt->nt_index;
	    current_string = get_newer_string(x);
	} else {
	    sprintf(temp, "%s is not a string capability", pad);
	    ptext(temp);
	    generic_done_message(test, state, chp);
	    return;
	}
    } else {
	x = tx_index[i];
	current_string = tx_cap[i];
	strcpy(pad, STR_NAME(x));
    }
    if (!current_string) {
	ptextln("That string is not currently defined.  Please enter a new value, including the padding delay:");
	read_string(buf, sizeof(buf));
	scan_terminfo(buf, pad, pad + sizeof(pad));
	t = (char *) malloc(strlen(pad) + 1);
	strcpy(t, pad);
	set_newer_string(x, t);
	sprintf(temp, "new string value  %s", STR_NAME(x));
	ptextln(temp);
	ptextln(expand(t));
	return;
    }
    sprintf(buf, "Current value: (%s) %s",
	    pad,
	    form_terminfo(current_string));
    putln(buf);
    ptextln("Enter new pad.  0 for no pad.  CR for no change.");
    read_string(buf, (size_t) 32);
    if (buf[0] == '\0' || (buf[1] == '\0' && isalpha(UChar(buf[0])))) {
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
    s = current_string;
    t = buf;
    for (v = 0; (ch = *t = *s++); t++) {
	if (v == '$' && ch == '<') {
	    while ((ch = *s++) && (ch != '>')) ;
	    for (p = pad; (*++t = *p++);) ;
	    *t++ = '>';
	    while ((*t++ = *s++)) ;
	    pad[0] = '\0';
	    break;
	}
	v = ch;
    }
    if (pad[0]) {
	sprintf(t, "$<%s>", pad);
    }
    if ((t = (char *) malloc(strlen(buf) + 1))) {
	strcpy(t, buf);
	set_newer_string(x, t);
	if (i != 255) {
	    tx_cap[i] = t;
	}
    }
    generic_done_message(test, state, chp);
}

/*
**	build_change_menu(menu_list)
**
**	Build the change pad menu list
*/
static void
build_change_menu(
		     TestMenu * m)
{
    int i, j, k;
    char *s;

    for (i = j = 0; i < txp; i++) {
	if ((k = tx_index[i]) >= 0) {
	    s = form_terminfo(tx_cap[i]);
	    s[40] = '\0';
	    sprintf(change_pad_text[j], "%c) (%s) %s",
		    'a' + j, STR_NAME(k), s);
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
	put_crlf();
	ptextln(m->menu_title);
    }
}
/* *INDENT-OFF* */
TestList edit_test_list[] = {
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
/* *INDENT-ON* */

TestMenu change_pad_menu =
{
    0, 'q', 0,
    "Select cap name", "change", 0,
    build_change_menu, change_pad_list, 0, 0, 0
};

#else
#endif /* TACK_CAN_EDIT */

#if NO_LEAKS
void
tack_edit_leaks(void)
{
    free_termtype(&original_term);

    FreeIfNeeded(label_strings);
    FreeIfNeeded(flag_boolean);
    FreeIfNeeded(flag_numbers);
    FreeIfNeeded(flag_strings);
}
#endif
