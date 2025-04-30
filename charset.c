/*
** Copyright 2017-2024,2025 Thomas E. Dickey
** Copyright 1997-2012,2017 Free Software Foundation, Inc.
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

#include <tack.h>

MODULE_ID("$Id: charset.c,v 1.30 2025/04/26 23:31:35 tom Exp $")

/*
	Menu definitions for alternate character set and SGR tests.
*/

#undef  BIT
#define BIT(n) (1 << (n))
/* *INDENT-OFF* */

const struct mode_list alt_modes[] =
{
    {"normal",     "(sgr0)",  "(sgr0)",  BIT(0)},
    {"standout",   "(smso)",  "(rmso)",  BIT(1)},
    {"underline",  "(smul)",  "(rmul)",  BIT(2)},
    {"reverse",    "(rev)",   "(sgr0)",  BIT(3)},
    {"blink",      "(blink)", "(sgr0)",  BIT(4)},
    {"dim",        "(dim)",   "(sgr0)",  BIT(5)},
    {"bold",       "(bold)",  "(sgr0)",  BIT(6)},
    {"invis",      "(invis)", "(sgr0)",  BIT(7)},
    {"protect",    "(prot)",  "(sgr0)",  BIT(8)},
    {"altcharset", "(smacs)", "(rmacs)", BIT(9)},
};
/* *INDENT-ON* */

/* On many terminals the underline attribute is the last scan line.
   This is OK unless the following line is reverse video.
   Then the underline attribute does not show up.  The following map
   will reorder the display so that the underline attribute will
   show up. */
const int mode_map[10] =
{0, 1, 3, 4, 5, 6, 7, 8, 9, 2};

struct graphics_pair {
    unsigned char c;
    const char *name;
};

static struct graphics_pair glyph[] =
{
    {'+', "arrow pointing right"},
    {',', "arrow pointing left"},
    {'.', "arrow pointing down"},
    {'0', "solid square block"},
    {'i', "lantern symbol"},
    {'-', "arrow pointing up"},
    {'`', "diamond"},
    {'a', "checker board (stipple)"},
    {'f', "degree symbol"},
    {'g', "plus/minus"},
    {'h', "board of squares"},
    {'j', "lower right corner"},
    {'k', "upper right corner"},
    {'l', "upper left corner"},
    {'m', "lower left corner"},
    {'n', "plus"},
    {'o', "scan line 1"},
    {'p', "scan line 3"},
    {'q', "horizontal line"},
    {'r', "scan line 7"},
    {'s', "scan line 9"},
    {'t', "left tee (|-)"},
    {'u', "right tee (-|)"},
    {'v', "bottom tee(_|_)"},
    {'w', "top tee (T)"},
    {'x', "vertical line"},
    {'y', "less/equal"},
    {'z', "greater/equal"},
    {'{', "Pi"},
    {'|', "not equal"},
    {'}', "UK pound sign"},
    {'~', "bullet"},
    {'\0', "\0"}
};

/*
**	charset_hs(test_list, status, ch)
**
**	(hs) test Has status line
*/
static void
charset_hs(
	      TestList * t,
	      int *state,
	      int *ch)
{
    if (has_status_line != 1) {
	ptext("(hs) Has-status line is not defined.  ");
	generic_done_message(t, state, ch);
    }
}

/*
**	charset_status(test_list, status, ch)
**
**	(tsl) (fsl) (wsl) test Status line
*/
static void
charset_status(
		  TestList * t,
		  int *state,
		  int *ch)
{
    int i, max;
    char *s;
    static const char m[] = "*** status line *** 123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.";

    if (has_status_line != 1) {
	return;
    }
    put_clear();
    max = width_status_line == -1 ? columns : width_status_line;
    sprintf(temp, "Terminal has status line of %d characters", max);
    ptextln(temp);

    put_str("This line s");
    s = TPARM_1(to_status_line, 0);
    tc_putp(s);
    for (i = 0; i < max; i++)
	putchp(m[i]);
    tc_putp(from_status_line);
    putln("hould not be broken.");
    ptextln("If the previous line is not a complete sentence then (tsl) to-status-line, (fsl) from-status-line, or (wsl) width-of-status-line is incorrect.");
    generic_done_message(t, state, ch);
}

/*
**	charset_eslok(test_list, status, ch)
**
**	(eslok) test Status line with cursor addressing and erasure.
*/
static void
charset_eslok(
		 TestList * t,
		 int *state,
		 int *ch)
{
    int i, max;
    char *s;
    static const char m[] = "*** status line *** 123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.";

    if (has_status_line != 1) {
	return;
    }
    if (status_line_esc_ok != 1) {
	ptextln("(eslok) status_line_esc_ok not present");
	return;
    }
    put_clear();
    max = width_status_line == -1 ? columns : width_status_line;
    sprintf(temp, "Terminal has status line of %d characters", max);
    ptextln(temp);

    put_str("This line s");
    s = TPARM_1(to_status_line, 10);
    tc_putp(s);
    for (i = 10; i < max; i++)
	putchp(m[i]);
    putchp('\r');
    for (i = 0; i < 10; i++)
	putchp(m[i]);
    if (cursor_address) {
	tputs(TPARM_2(cursor_address, 4, 4), lines, tc_putch);
	put_str("STATUS");
    }
    tc_putp(from_status_line);
    putln("hould not be broken.");
    ptextln("If the previous line is not a complete sentence then the preceding test failed, or (eslok) status_line_esc_ok is incorrect.  The status line should have \"STATUS\".");
    generic_done_message(t, state, ch);
}

/*
**	charset_dsl(test_list, status, ch)
**
**	(dsl) test Disable status line
*/
static void
charset_dsl(
	       TestList * t,
	       int *state,
	       int *ch)
{
    if (has_status_line != 1) {
	return;
    }
    if (dis_status_line) {
	ptextln("Disable status line (dsl)");
	tc_putp(dis_status_line);
	ptext("If you can still see the status line then (dsl) disable-status-line has failed.  ");
    } else {
	ptext("(dsl) Disable-status-line is not defined.  ");
    }
    generic_done_message(t, state, ch);
}

void
eat_cookie(void)
{				/* put a blank if this is not a magic cookie
				   terminal */
    if (magic_cookie_glitch < 1)
	putchp(' ');
}

void
put_mode(const char *s)
{				/* send the attribute string (with or without
				   % execution) */
    tc_putp(TPARM_0(s));	/* allow % execution */
}

void
set_attr(int a)
{				/* set the attribute from the bits in a */
    int i, b[10];
    int use_sgr = 0;

    if (magic_cookie_glitch > 0) {
	char_count += magic_cookie_glitch;
    }
    if (a == 0 && exit_attribute_mode) {
	put_mode(exit_attribute_mode);
	return;
    }
    memset(b, 0, sizeof(b));
    for (i = 0; i < 10; i++) {
	b[i] = (a >> i) & 1;
	if (b[i])
	    use_sgr = 1;
    }
    if (use_sgr) {
	tc_putp(TPARM_9(set_attributes,
			b[1], b[2], b[3], b[4], b[5],
			b[6], b[7], b[8], b[9]));
    }
}

/*
**	charset_sgr(test_list, status, ch)
**
**	(sgr) test Set Graphics Rendition
*/
static void
charset_sgr(
	       TestList * t,
	       int *state,
	       int *ch)
{
    int i;

    if (!set_attributes) {
	ptext("(sgr) Set-graphics-rendition is not defined.  ");
	generic_done_message(t, state, ch);
	return;
    }
    if (!exit_attribute_mode) {
	ptextln("(sgr0) Set-graphics-rendition-zero is not defined.");
	/* go ahead and test anyway */
    }
    ptext("Test video attributes");

    for (i = 0; i < (int) (sizeof(alt_modes) / sizeof(struct mode_list));
	 i++) {
	put_crlf();
	sprintf(temp, "%2d %-20s", i, alt_modes[i].name);
	put_str(temp);
	set_attr(alt_modes[i].number);
	sprintf(temp, "%s", alt_modes[i].name);
	put_str(temp);
	set_attr(0);
    }
    put_crlf();
    generic_done_message(t, state, ch);
}

static void
charset_sgr2(
		TestList * t,
		int *state,
		int *ch)
{
    int i, j;

    putln("Double mode test");
    for (i = 0; i <= 9; i++) {
	sprintf(temp, " %2d ", mode_map[i]);
	put_str(temp);
    }
    for (i = 0; i <= 9; i++) {
	put_crlf();
	sprintf(temp, "%d", mode_map[i]);
	put_str(temp);
	for (j = 0; j <= 9; j++) {
	    eat_cookie();
	    set_attr((1 << mode_map[i]) | (1 << mode_map[j]));
	    put_str("Aa");
	    set_attr(0);
	    if (j < 9)
		eat_cookie();
	}
    }
    put_crlf();

#ifdef max_attributes
    if (max_attributes >= 0) {
	sprintf(temp, "(ma) Maximum attributes %d  ", max_attributes);
	ptext(temp);
    }
#endif
    generic_done_message(t, state, ch);
}

static void
charset_italics(
		   TestList * t,
		   int *state,
		   int *ch)
{
    putln("Testing italics");
    if (enter_italics_mode && exit_italics_mode) {
	put_mode(enter_italics_mode);
	ptext(" This should be in ITALICS");
	put_mode(exit_italics_mode);
	put_crlf();
    } else {
	putln("your terminal description does not tell how to show italics");
    }
    generic_done_message(t, state, ch);
}

/*
 * rmxx/smxx describes the ECMA-48 strikeout/crossed-out attributes, as an
 * experimental feature of tmux.
 */
static void
charset_crossed(
		   TestList * t,
		   int *state,
		   int *ch)
{
    const char *smxx = safe_tgets("smxx");
    const char *rmxx = safe_tgets("rmxx");
    putln("Testing cross-out/strike-out (ncurses/tmux extension)");
    if (VALID_STRING(smxx) && VALID_STRING(rmxx)) {
	put_mode(smxx);
	ptext(" This should be CROSSED-OUT");
	put_mode(rmxx);
	put_crlf();
    } else {
	putln("your terminal description does not tell how to cross-out text");
    }
    ptext("(smxx) (rmxx) ");
    generic_done_message(t, state, ch);
}

/*
**	test_one_attr(mode-number, begin-string, end-string)
**
**	Display one attribute line.
*/
static void
test_one_attr(
		 int n,
		 const char *begin_mode,
		 const char *end_mode)
{
    int i;

    sprintf(temp, "%-10s %s ", alt_modes[n].name, alt_modes[n].begin_mode);
    ptext(temp);
    for (; char_count < 19;) {
	putchp(' ');
    }
    if (begin_mode) {
	putchp('.');
	put_mode(begin_mode);
	put_str(alt_modes[n].name);
	for (i = (int) strlen(alt_modes[n].name); i < 13; i++) {
	    putchp(' ');
	}
	if (end_mode) {
	    put_mode(end_mode);
	    sprintf(temp, ". %s", alt_modes[n].end_mode);
	} else {
	    set_attr(0);
	    strcpy(temp, ". (sgr)");
	}
	ptextln(temp);
    } else {
	for (i = 0; i < magic_cookie_glitch; i++)
	    putchp('*');
	put_str("*** missing ***");
	for (i = 0; i < magic_cookie_glitch; i++)
	    putchp('*');
	put_crlf();
    }
}

/*
**	charset_attributes(test_list, status, ch)
**
**	Test SGR
*/
static void
charset_attributes(
		      TestList * t,
		      int *state,
		      int *ch)
{
    put_clear();
    putln("Test video attributes");
    test_one_attr(1, enter_standout_mode, exit_standout_mode);
    test_one_attr(2, enter_underline_mode, exit_underline_mode);
    test_one_attr(9, enter_alt_charset_mode, exit_alt_charset_mode);
    if (!exit_attribute_mode && !set_attributes) {
	ptextln("(sgr0) exit attribute mode is not defined.");
	generic_done_message(t, state, ch);
	return;
    }
    test_one_attr(3, enter_reverse_mode, exit_attribute_mode);
    test_one_attr(4, enter_blink_mode, exit_attribute_mode);
    test_one_attr(5, enter_dim_mode, exit_attribute_mode);
    test_one_attr(6, enter_bold_mode, exit_attribute_mode);
    test_one_attr(7, enter_secure_mode, exit_attribute_mode);
    test_one_attr(8, enter_protected_mode, exit_attribute_mode);
    generic_done_message(t, state, ch);
}

#define GLYPHS 256

/*
**	charset_smacs(test_list, status, ch)
**
**	display all possible acs characters
**	(smacs) (rmacs)
*/
static void
charset_smacs(
		 TestList * t,
		 int *state,
		 int *ch)
{
    if (enter_alt_charset_mode) {
	int i, c;

	put_clear();
	ptextln("The following characters are available. (smacs) (rmacs)");
	for (i = ' '; i <= '`'; i += 32) {
	    put_crlf();
	    put_mode(exit_alt_charset_mode);
	    for (c = 0; c < 32; c++) {
		putchp(c + i);
	    }
	    put_crlf();
	    put_mode(enter_alt_charset_mode);
	    for (c = 0; c < 32; c++) {
		putchp(c + i);
	    }
	    put_mode(exit_alt_charset_mode);
	    put_crlf();
	}
	put_mode(exit_alt_charset_mode);
	put_crlf();
	generic_done_message(t, state, ch);
    }
}

static void
test_acs(
	    int attr)
{				/* alternate character set */
    int i;
    char valid_glyph[GLYPHS];
    char acs_table[GLYPHS];

    line_count = 0;
    for (i = 0; i < GLYPHS; i++) {
	valid_glyph[i] = FALSE;
	acs_table[i] = (char) i;
    }
    if (acs_chars) {
	putln("Alternate character set map:");
	putln(expand(acs_chars));
	put_crlf();
	for (i = 0; acs_chars[i]; i += 2) {
	    int j;

	    if (acs_chars[i + 1] == 0) {
		break;
	    }
	    for (j = 0;; j++) {
		if (glyph[j].c == (unsigned char) acs_chars[i]) {
		    acs_table[glyph[j].c] = acs_chars[i + 1];
		    valid_glyph[glyph[j].c] = TRUE;
		    break;
		}
		if (glyph[j].name[0] == '\0') {
		    if (isgraph(UChar(acs_chars[i]))) {
			sprintf(temp, "    %c",
				acs_chars[i]);
		    } else {
			sprintf(temp, " 0x%02x",
				UChar(acs_chars[i]));
		    }
		    strcpy(&temp[5], " *** has no mapping ***");
		    putln(temp);
		    break;
		}
	    }
	}
    } else {
	static unsigned char vt100[] = "`afgjklmnopqrstuvwxyz{|}~";

	ptextln("acs_chars not defined (acsc)");
	/* enable the VT-100 graphics characters (default) */
	for (i = 0; vt100[i]; i++) {
	    valid_glyph[vt100[i]] = TRUE;
	}
    }
    if (attr) {
	set_attr(attr);
    }
    put_mode(ena_acs);
    for (i = 0; glyph[i].name[0]; i++) {
	if (valid_glyph[glyph[i].c]) {
	    put_mode(enter_alt_charset_mode);
	    put_this(acs_table[glyph[i].c]);
	    char_count++;
	    put_mode(exit_alt_charset_mode);
	    if (magic_cookie_glitch >= 1) {
		sprintf(temp, " %-30.30s", glyph[i].name);
		put_str(temp);
		if (char_count + 33 >= columns)
		    put_crlf();
	    } else {
		sprintf(temp, " %-24.24s", glyph[i].name);
		put_str(temp);
		if (char_count + 26 >= columns)
		    put_crlf();
	    }
	    if (line_count >= lines) {
		(void) wait_here();
		put_clear();
	    }
	}
    }
    if (char_count > 1) {
	put_crlf();
    }
#ifdef ACS_ULCORNER
    maybe_wait(5);
    put_mode(enter_alt_charset_mode);
    put_that(ACS_ULCORNER);
    put_that(ACS_TTEE);
    put_that(ACS_URCORNER);
    put_that(ACS_ULCORNER);
    put_that(ACS_HLINE);
    put_that(ACS_URCORNER);
    char_count += 6;
    put_mode(exit_alt_charset_mode);
    put_crlf();
    put_mode(enter_alt_charset_mode);
    put_that(ACS_LTEE);
    put_that(ACS_PLUS);
    put_that(ACS_RTEE);
    put_that(ACS_VLINE);
    if (magic_cookie_glitch >= 1)
	put_this(' ');
    else {
	put_mode(exit_alt_charset_mode);
	put_this(' ');
	put_mode(enter_alt_charset_mode);
    }
    put_that(ACS_VLINE);
    char_count += 6;
    put_mode(exit_alt_charset_mode);
    put_str("   Here are 2 boxes");
    put_crlf();
    put_mode(enter_alt_charset_mode);
    put_that(ACS_LLCORNER);
    put_that(ACS_BTEE);
    put_that(ACS_LRCORNER);
    put_that(ACS_LLCORNER);
    put_that(ACS_HLINE);
    put_that(ACS_LRCORNER);
    char_count += 6;
    put_mode(exit_alt_charset_mode);
    put_crlf();
#endif
}

/*
**	charset_bel(test_list, status, ch)
**
**	(bel) test Bell
*/
static void
charset_bel(
	       TestList * t,
	       int *state,
	       int *ch)
{
    if (bell) {
	ptextln("Testing bell (bel)");
	tc_putp(bell);
	ptext("If you did not hear the Bell then (bel) has failed.  ");
    } else {
	ptext("(bel) Bell is not defined.  ");
    }
    generic_done_message(t, state, ch);
}

/*
**	charset_flash(test_list, status, ch)
**
**	(flash) test Visual bell
*/
static void
charset_flash(
		 TestList * t,
		 int *state,
		 int *ch)
{
    if (flash_screen) {
	ptextln("Testing visual bell (flash)");
	tc_putp(flash_screen);
	ptext("If you did not see the screen flash then (flash) has failed.  ");
    } else {
	ptext("(flash) Flash is not defined.  ");
    }
    generic_done_message(t, state, ch);
}

/*
**	charset_civis(test_list, status, ch)
**
**	(civis) test Cursor invisible
*/
static void
charset_civis(
		 TestList * t,
		 int *state,
		 int *ch)
{
    if (cursor_normal) {
	if (cursor_invisible) {
	    ptext("(civis) Turn off the cursor.  ");
	    tc_putp(cursor_invisible);
	    ptext("If you can still see the cursor then (civis) has failed.  ");
	} else {
	    ptext("(civis) Cursor-invisible is not defined.  ");
	}
	generic_done_message(t, state, ch);
	tc_putp(cursor_normal);
    }
}

/*
**	charset_cvvis(test_list, status, ch)
**
**	(cvvis) test Cursor very visible
*/
static void
charset_cvvis(
		 TestList * t,
		 int *state,
		 int *ch)
{
    if (cursor_normal) {
	if (cursor_visible) {
	    ptext("(cvvis) Make cursor very visible.  ");
	    tc_putp(cursor_visible);
	    ptext("If the cursor is not very visible then (cvvis) has failed.  ");
	} else {
	    ptext("(cvvis) Cursor-very-visible is not defined.  ");
	}
	generic_done_message(t, state, ch);
	tc_putp(cursor_normal);
    }
}

/*
**	charset_cnorm(test_list, status, ch)
**
**	(cnorm) test Cursor normal
*/
static void
charset_cnorm(
		 TestList * t,
		 int *state,
		 int *ch)
{
    if (cursor_normal) {
	ptext("(cnorm) Normal cursor.  ");
	tc_putp(cursor_normal);
	ptext("If the cursor is not normal then (cnorm) has failed.  ");
    } else {
	ptext("(cnorm) Cursor-normal is not defined.  ");
    }
    generic_done_message(t, state, ch);
}

/*
**	charset_enacs(test_list, status, ch)
**
**	test Alternate character set mode and alternate characters
**	(acsc) (enacs) (smacs) (rmacs)
*/
static void
charset_enacs(
		 TestList * t,
		 int *state,
		 int *ch)
{
    if (enter_alt_charset_mode || acs_chars) {
	int c, i;

	c = 0;
	while (1) {
	    put_clear();
	    /*
	       for terminals that use separate fonts for
	       attributes (such as X windows) the line
	       drawing characters must be checked for
	       each font.
	     */
	    if (c >= '0' && c <= '9') {
		test_acs(alt_modes[c - '0'].number);
		set_attr(0);
	    } else {
		test_acs(0);
	    }

	    while (1) {
		ptextln("[r] to repeat, [012345789] to test with attributes on, [?] for a list of attributes, anything else to go to next test.  ");
		generic_done_message(t, state, ch);
		if (*ch != '?') {
		    break;
		}
		for (i = 0; i <= 9; i++) {
		    sprintf(temp, " %d %s %s", i, alt_modes[i].begin_mode,
			    alt_modes[i].name);
		    ptextln(temp);
		}
	    }
	    if (*ch >= '0' && *ch <= '9') {
		c = *ch;
	    } else if (*ch != 'r') {
		break;
	    }
	}
    } else {
	ptext("(smacs) Enter-alt-char-set-mode and (acsc) Alternate-char-set are not defined.  ");
	generic_done_message(t, state, ch);
    }
}

/*
**	charset_can_test()
**
**	Initialize the can_test data base
*/
void
charset_can_test(void)
{
    int i;

    for (i = 0; i < 9; i++) {
	can_test(alt_modes[i].begin_mode, FLAG_CAN_TEST);
	can_test(alt_modes[i].end_mode, FLAG_CAN_TEST);
    }
}
/* *INDENT-OFF* */

TestList acs_test_list[] =
{
    MY_EDIT_MENU
    {MENU_NEXT, 3, "bel",   NULL, "b) bel/flash", charset_bel, NULL},
    {MENU_NEXT, 3, "flash", NULL, NULL, charset_flash, NULL},
    {MENU_NEXT, 3, "civis", NULL, "c) cursor appearance", charset_civis, NULL},
    {MENU_NEXT, 3, "cvvis", NULL, NULL, charset_cvvis, NULL},
    {MENU_NEXT, 3, "cnorm", NULL, NULL, charset_cnorm, NULL},
    {MENU_NEXT, 3, "hs",    NULL, "t) title/statusline", charset_hs, NULL},
    {MENU_NEXT, 3, "tsl) (fsl) (wsl", "hs", NULL, charset_status, NULL},
    {MENU_NEXT, 3, "eslok", "hs", NULL, charset_eslok, NULL},
    {MENU_NEXT, 3, "dsl", "hs", NULL, charset_dsl, NULL},
    {MENU_NEXT, 0, "acsc) (enacs) (smacs) (rmacs", NULL, "a) alternate character set", charset_enacs, NULL},
    {MENU_NEXT, 12, "smacs) (rmacs", NULL, NULL, charset_smacs, NULL},
    {MENU_NEXT, 12, "sgr) (sgr0", NULL, NULL, charset_attributes, NULL},
    {MENU_NEXT, 12, "sgr) (sgr0", "ma", NULL, charset_sgr, NULL},
    {MENU_NEXT, 12, "sgr) (sgr0", "ma", NULL, charset_sgr2, NULL},
    {MENU_NEXT, 3, "sitm) (ritm", NULL, NULL, charset_italics, NULL},
    {MENU_NEXT, 3, NULL, NULL, NULL, charset_crossed, NULL},
    {MENU_LAST, 0, NULL, NULL, NULL, NULL, NULL}
};
/* *INDENT-ON* */
