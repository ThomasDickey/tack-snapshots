/*
** Copyright (C) 1991, 1997-2013,2017 Free Software Foundation, Inc.
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
/* initialization and wrapup code */

#include <tack.h>

MODULE_ID("$Id: init.c,v 1.29 2017/07/25 22:58:47 tom Exp $")

FILE *debug_fp;
char temp[1024];
char *tty_basename = NULL;

#ifndef HAVE_CURSES_DATA_BOOLNAMES
char **boolnames;
char **numnames;
char **strnames;
size_t max_booleans;
size_t max_numbers;
size_t max_strings;
#endif

void
put_name(const char *cap, const char *name)
{				/* send the cap name followed by the cap */
    if (cap) {
	ptext(name);
	tc_putp(cap);
    }
}

static void
report_cap(const char *tag, const char *s)
{				/* expand the cap or print *** missing *** */
    int i;

    ptext(tag);
    for (i = char_count; i < 13; i++) {
	putchp(' ');
    }
    put_str(" = ");
    if (s) {
	putln(expand(s));
    } else {
	putln("*** missing ***");
    }
}

void
reset_init(void)
{				/* send the reset and init strings */
    int i;

    ptext("Terminal reset");
    i = char_count;
    put_name(reset_1string, " (rs1)");
    put_name(reset_2string, " (rs2)");
    /* run the reset file */
    if (reset_file && reset_file[0]) {
	FILE *fp;
	int ch;

	can_test("rf", FLAG_TESTED);
	if ((fp = fopen(reset_file, "r"))) {	/* send the reset file */
	    sprintf(temp, " (rf) %s", reset_file);
	    ptextln(temp);
	    while (1) {
		ch = getc(fp);
		if (ch == EOF)
		    break;
		put_this(ch);
	    }
	    fclose(fp);
	} else {
	    sprintf(temp, "\nCannot open reset file (rf) %s", reset_file);
	    ptextln(temp);
	}
    }
    put_name(reset_3string, " (rs3)");
    if (i != char_count) {
	put_crlf();
    }
    ptext(" init");
    put_name(init_1string, " (is1)");
    put_name(init_2string, " (is2)");
    if (set_tab && clear_all_tabs && init_tabs != 8) {
	put_crlf();
	tc_putp(clear_all_tabs);
	for (char_count = 0; char_count < columns; char_count++) {
	    put_this(' ');
	    if ((char_count & 7) == 7) {
		tc_putp(set_tab);
	    }
	}
	put_cr();
    }
    /* run the initialization file */
    if (init_file && init_file[0]) {
	FILE *fp;
	int ch;

	can_test("if", FLAG_TESTED);
	if ((fp = fopen(init_file, "r"))) {	/* send the init file */
	    sprintf(temp, " (if) %s", init_file);
	    ptextln(temp);
	    while (1) {
		ch = getc(fp);
		if (ch == EOF)
		    break;
		put_this(ch);
	    }
	    fclose(fp);
	} else {
	    sprintf(temp, "\nCannot open init file (if) %s", init_file);
	    ptextln(temp);
	}
    }
    if (init_prog) {
	can_test("iprog", FLAG_TESTED);
	IGNORE_RC(system(init_prog));
    }
    put_name(init_3string, " (is3)");

    fflush(stdout);
}

/*
**	display_basic()
**
**	display the basic terminal definitions
*/
void
display_basic(void)
{
    char *s;
    put_str("Name: ");
    put_str(termname());
    if ((s = longname()) != 0) {
	put_str("|");
	putln(s);
    }

    report_cap("\\r ^M (cr)", carriage_return);
    report_cap("\\n ^J (ind)", scroll_forward);
    report_cap("\\b ^H (cub1)", cursor_left);
    report_cap("\\t ^I (ht)", tab);
/*      report_cap("\\f ^L (ff)", form_feed);	*/
    if (newline) {
	/* OK if missing */
	report_cap("      (nel)", newline);
    }
    report_cap("      (clear)", clear_screen);
    if (!cursor_home && cursor_address) {
	report_cap("(cup) (home)", TPARM_2(cursor_address, 0, 0));
    } else {
	report_cap("      (home)", cursor_home);
    }
#ifdef user9
    report_cap("ENQ   (u9)", user9);
#endif
#ifdef user8
    report_cap("ACK   (u8)", user8);
#endif

    sprintf(temp,
	    "\nTerminal size: %d x %d.  Baud rate: %u.  Frame size: %d.%d",
	    columns, lines,
	    tty_baud_rate,
	    tty_frame_size >> 1,
	    (tty_frame_size & 1) * 5);
    putln(temp);
}

/*
 * Conventional infocmp (Unix or ncurses) will print a comment at the
 * beginning of the terminal description, where the pathname of the terminfo
 * information is after a "file" or "file:" keyword.
 */
static char *
ask_infocmp(void)
{
    char *result = 0;
    size_t need = strlen(tty_basename) + 20;
    char *command = malloc(need);
    FILE *pp;
    char buffer[BUFSIZ];
    char *s, *t;

    if (command != 0) {
	sprintf(command, "infocmp -1 \"%s\"", tty_basename);
	if ((pp = popen(command, "r")) != 0) {
	    if (fgets(buffer, sizeof(buffer) - 1, pp) != 0
		&& *buffer == '#'
		&& ((t = strstr(buffer, " file: "))
		    || (t = strstr(buffer, " file "))
		    || ((t = strstr(buffer, " Reconstructed "))
			&& (t = strstr(t, " from "))))
		&& (s = strchr(buffer, '\n')) != 0) {
		*s = '\0';
		s = strchr(t + 1, ' ');
		result = strdup(s + 1);
	    }
#ifndef HAVE_CURSES_DATA_BOOLNAMES
	    if (result) {
		int max_b = 200;
		int max_n = 200;
		int max_s = 200;
		boolnames = calloc(max_b, sizeof(*boolnames));
		numnames = calloc(max_n, sizeof(*numnames));
		strnames = calloc(max_s, sizeof(*strnames));
		while (fgets(buffer, sizeof(buffer) - 1, pp) != 0) {
		    int mytype = BOOLEAN;
		    if (*buffer != '\t')
			continue;
		    for (s = buffer; isspace(UChar(*s)); ++s) ;
		    for (t = s; *t != '\0'; ++t) {
			if (strchr("@,", *t)) {
			    *t = '\0';
			    break;
			} else if (*t == '#') {
			    mytype = NUMBER;
			    *t = '\0';
			    break;
			} else if (*t == '=') {
			    mytype = STRING;
			    *t = '\0';
			    break;
			}
		    }
		    s = strdup(s);
		    switch (mytype) {
		    case 0:
			boolnames[max_booleans++] = s;
			break;
		    case 1:
			numnames[max_numbers++] = s;
			break;
		    case 2:
			strnames[max_strings++] = s;
			break;
		    }
		}
	    }
#endif
	    pclose(pp);
	}
	free(command);
    }
    return result;
}

/*
 * ncurses initializes acs_map[] in setupterm; Unix curses does not.
 */
static void
init_acs(void)
{
    int s, d;
    const char *value = acs_chars;

#ifdef NUM_ACS			/* NetBSD curses */
#ifndef acs_map
#define acs_map _acs_map
#endif
#elif !defined(NCURSES_VERSION)
    if (acs_map == 0) {
	acs_map = calloc(256, sizeof(acs_map[0]));
    }
#endif
    if (value != 0) {
	while (*value != '\0') {
	    if ((s = UChar(*value++)) == '\0')
		break;
	    if ((d = UChar(*value++)) == '\0')
		break;
	    acs_map[s] = (chtype) d;
	}
    }
}

/*
 * curses provides baudrate(), but not for low-level applications.
 */
static unsigned
init_baudrate(void)
{
    struct speed {
	int given_speed;	/* values for 'ospeed' */
	unsigned actual_speed;	/* the actual speed */
    };

#define DATA(number) { B##number, number }

    static struct speed const speeds[] =
    {
	DATA(0),
	DATA(50),
	DATA(75),
	DATA(110),
	DATA(134),
	DATA(150),
	DATA(200),
	DATA(300),
	DATA(600),
	DATA(1200),
	DATA(1800),
	DATA(2400),
	DATA(4800),
	DATA(9600),
#ifdef B19200
	DATA(19200),
#elif defined(EXTA)
	{EXTA, 19200},
#endif
#ifdef B28800
	DATA(28800),
#endif
#ifdef B38400
	DATA(38400),
#elif defined(EXTB)
	{EXTB, 38400},
#endif
#ifdef B57600
	DATA(57600),
#endif
    /* ifdef to prevent overflow when OLD_TTY is not available */
    };
#undef DATA
    struct termios data;
    unsigned result = 1;
    if (tcgetattr(fileno(stdout), &data) == 0) {
	int ospeed = (int) cfgetospeed(&data);
	size_t n;
	for (n = 0; n < sizeof(speeds) / sizeof(speeds[0]); ++n) {
	    if (ospeed == speeds[n].given_speed) {
		result = speeds[n].actual_speed;
		break;
	    }
	}
    }
    return result;
}

/*
**	curses_setup(exec_name)
**
**	Startup ncurses
*/
void
curses_setup(
		char *exec_name)
{
    int status;
    char *tty_name;

    tty_init();

	/**
	 * Load the terminfo data base and set the cur_term variable.
	 */
    if (setupterm(tty_basename, 1, &status) != OK) {
	if (status < 0) {
	    fprintf(stderr, "The terminal database could not be found\n");
	} else {
	    fprintf(stderr, "The \"%s\" terminal is listed as %s\n",
		    tty_basename,
		    (status > 0) ? "hardcopy" : "generic");
	}
	show_usage(exec_name);
	ExitProgram(EXIT_FAILURE);
    }
    init_acs();

    tty_baud_rate = init_baudrate();
    tty_cps = (tty_baud_rate << 1) / (unsigned) tty_frame_size;

    /*
     * "everyone" has infocmp, and its first line of output should be a
     * comment telling which database is used.
     */
    tty_name = ask_infocmp();

    /* set up the defaults */
    replace_mode = TRUE;
    scan_mode = 0;
    char_count = 0;
    select_delay_type = debug_level = 0;
    char_mask = (meta_on && meta_on[0] == '\0') ? ALLOW_PARITY : STRIP_PARITY;
    /* Don't change the XON/XOFF modes yet. */
    select_xon_xoff = initial_stty_query(TTY_XON_XOFF) ? 1 : needs_xon_xoff;

    fflush(stdout);		/* flush any output */
    tty_set();

    go_home();			/* set can_go_home */
    put_clear();		/* set can_clear_screen */

    if (send_reset_init) {
	reset_init();
    }

    /*
     * I assume that the reset and init strings may not have the correct
     * pads.  (Because that part of the test comes much later.)  Because
     * of this, I allow the terminal some time to catch up.
     */
    fflush(stdout);		/* waste some time */
    sleep(1);			/* waste more time */
    charset_can_test();
    can_test("lines cols cr nxon rf if iprog rmp smcup rmcup", FLAG_CAN_TEST);
    edit_init();		/* initialize the edit data base */

    if (send_reset_init && enter_ca_mode) {
	tc_putp(enter_ca_mode);
	put_clear();		/* just in case we switched pages */
    }
    /*
     * "everyone" has infocmp, and its first line of output should be a
     * comment telling which database is used.
     */
    if (tty_name != 0) {
	ptext("Using terminfo from: ");
	ptextln(tty_name);
	put_crlf();
	free(tty_name);
    }

    if (tty_can_sync == SYNC_NEEDED) {
	verify_time();
    }

    display_basic();
}

/*
**	bye_kids(exit-condition)
**
**	Shutdown the terminal, clear the signals, and exit
*/
void
bye_kids(int n)
{				/* reset the tty and exit */
    ignoresig();
    if (send_reset_init) {
	if (exit_ca_mode) {
	    tc_putp(exit_ca_mode);
	}
	if (initial_stty_query(TTY_XON_XOFF)) {
	    if (enter_xon_mode) {
		tc_putp(enter_xon_mode);
	    }
	} else if (exit_xon_mode) {
	    tc_putp(exit_xon_mode);
	}
    }
    if (debug_fp) {
	fclose(debug_fp);
    }
    if (log_fp) {
	fclose(log_fp);
    }
    tty_reset();
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
    if (not_a_tty)
	sleep(1);
    ExitProgram(n);
}
