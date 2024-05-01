/*
** Copyright 2017-2020,2024 Thomas E. Dickey
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
#include <stdarg.h>
#include <unistd.h>

MODULE_ID("$Id: tack.c,v 1.40 2024/05/01 19:16:08 tom Exp $")

/*
   This program is designed to test terminfo, not curses.  Therefore
   I have used as little of curses as possible.

   Pads associated with the following capabilities are used to set
   delay times in the handler:  (cr), (ind), (cub1), (ff), (tab).

   I use the (nxon) capability to set the tty handler with/without
   xon/xoff.  If (smxon)/(rmxon) is defined I will change the terminal
   too.

   (xon) inhibits the sending of delay characters in putp().
   If the terminal is defined with no padding then the (xon) boolean
   is a don't care.  In this case I recommend that it be reset.
 */

/*****************************************************************************
 *
 * Option processing
 *
 *****************************************************************************/

/* options and modes */
int debug_level;		/* debugging level */
int translate_mode;		/* translate tab, bs, cr, lf, ff */
int scan_mode;			/* use scan codes */
int char_mask;			/* either 0xFF else 0x7F, eight bit data mask */
int select_delay_type;		/* set handler delays for <cr><lf> */
int select_xon_xoff;		/* TTY driver XON/XOFF mode select */
int hex_out;			/* Display output in hex */
int send_reset_init;		/* Send the reset and initialization strings */
FILE *log_fp;			/* Terminal logfile */

#if defined(__GNUC__) && defined(_FORTIFY_SOURCE)
int ignore_unused;
#endif

/*****************************************************************************
 *
 * Menu definitions
 *
 *****************************************************************************/

static void tools_hex_echo(TestList *, int *, int *);
static void tools_debug(TestList *, int *, int *);

#define MENU_ENABLE_HEX_OUTPUT  "h) enable hex output on echo tool"
#define MENU_DISABLE_HEX_OUTPUT "h) disable hex output on echo tool"
static char hex_echo_menu_entry[80];

static TestList tools_test_list[] =
{
    {0, 0, 0, 0, "s) ANSI status reports", tools_status, 0},
    {0, 0, 0, 0, "g) ANSI SGR modes (bold, underline, reverse)", tools_sgr, 0},
    {0, 0, 0, 0, "c) ANSI character sets", tools_charset, 0},
    {0, 0, 0, 0, hex_echo_menu_entry, tools_hex_echo, 0},
    {0, 0, 0, 0, "e) echo tool", tools_report, 0},
    {1, 0, 0, 0, "r) reply tool", tools_report, 0},
    {0, 0, 0, 0, "p) performance testing", 0, &sync_menu},
    {0, 0, 0, 0, "i) send reset and init", menu_reset_init, 0},
    {0, 0, "u8) (u9", 0, "u) test ENQ/ACK handshake", sync_handshake, 0},
    {0, 0, 0, 0, "d) change debug level", tools_debug, 0},
    {MENU_LAST, 0, 0, 0, 0, 0, 0}
};

static TestMenu tools_menu =
{
    0, 'q', 0, "Tools Menu", "tools",
    0, 0, tools_test_list, 0, 0, 0
};

static void tty_width(TestList *, int *, int *);
static void tty_delay(TestList *, int *, int *);
static void tty_xon(TestList *, int *, int *);
static void tty_trans(TestList *, int *, int *);
static void tty_show_state(TestMenu *);

static char tty_width_menu[80];
static char tty_delay_menu[80];
static char tty_xon_menu[80];
static char tty_trans_menu[80];
static char enable_xon_xoff[] =
{"x) enable xon/xoff"};
static char disable_xon_xoff[] =
{"x) disable xon/xoff"};

static TestList tty_test_list[] =
{
    {0, 0, 0, 0, tty_width_menu, tty_width, 0},
    {0, 0, 0, 0, tty_delay_menu, tty_delay, 0},
    {0, 0, 0, 0, tty_xon_menu, tty_xon, 0},
    {0, 0, 0, 0, tty_trans_menu, tty_trans, 0},
    {MENU_LAST, 0, 0, 0, 0, 0, 0}
};

static TestMenu tty_menu =
{
    0, 'q', 0, "Terminal and driver configuration",
    "tty", 0,
    tty_show_state, tty_test_list, 0, 0, 0
};

#if TACK_CAN_EDIT
TestMenu edit_menu =
{
    0, 'q', 0, "Edit terminfo menu",
    "edit", 0,
    0, edit_test_list, 0, 0, 0
};
#endif

static TestMenu mode_menu =
{
    0, 'n', 0, "Test modes and glitches:",
    "mode", "n) run standard tests",
    0, mode_test_list, 0, 0, 0
};

static TestMenu acs_menu =
{
    0, 'n', 0,
    "Test alternate character set and graphics rendition:",
    "acs", "n) run standard tests",
    0, acs_test_list, 0, 0, 0
};

static TestMenu color_menu =
{
    0, 'n', 0,
    "Test color:",
    "color", "n) run standard tests",
    0, color_test_list, 0, 0, 0
};

static TestMenu crum_menu =
{
    0, 'n', 0,
    "Test cursor movement:",
    "move", "n) run standard tests",
    0, crum_test_list, 0, 0, 0
};

static TestMenu funkey_menu =
{
    0, 'n', 0,
    "Test function keys:",
    "fkey", "n) run standard tests",
    sync_test, funkey_test_list, 0, 0, 0
};

static TestMenu printer_menu =
{
    0, 'n', 0,
    "Test printer:",
    "printer", "n) run standard tests",
    0, printer_test_list, 0, 0, 0
};

static void pad_gen(TestList *, int *, int *);

static TestMenu pad_menu =
{
    0, 'n', 0,
    "Test padding and string capabilities:",
    "pad", "n) run standard tests",
    sync_test, pad_test_list, 0, 0, 0
};
/* *INDENT-OFF* */
static TestList normal_test_list[] = {
    MY_EDIT_MENU
    {0, 0, 0, 0, "i) send reset and init", menu_reset_init, 0},
    {MENU_NEXT, 0, 0, 0, "x) test modes and glitches", 0, &mode_menu},
    {MENU_NEXT, 0, 0, 0, "a) test alternate character set and graphic rendition", 0, &acs_menu},
    {MENU_NEXT, 0, 0, 0, "c) test color", 0, &color_menu},
    {MENU_NEXT, 0, 0, 0, "m) test cursor movement", 0, &crum_menu},
    {MENU_NEXT, 0, 0, 0, "f) test function keys", 0, &funkey_menu},
    {MENU_NEXT, 0, 0, 0, "p) test padding and string capabilities", 0, &pad_menu},
    {0, 0, 0, 0, "P) test printer", 0, &printer_menu},
    {MENU_MENU, 0, 0, 0, "/) test a specific capability", 0, 0},
    {0, 0, 0, 0, "t) auto generate pad delays", pad_gen, &pad_menu},
    {0, 0, "u8) (u9", 0, 0, sync_handshake, 0},
    {MENU_LAST, 0, 0, 0, 0, 0, 0}
};
/* *INDENT-ON* */

static TestMenu normal_menu =
{
    0, 'n', 0, "Main test menu",
    "test", "n) run standard tests",
    0, normal_test_list, 0, 0, 0
};

static void start_tools(TestList *, int *, int *);
static void start_modes(TestList *, int *, int *);
static void start_basic(TestList *, int *, int *);
static void start_log(TestList *, int *, int *);

#define MENU_START_LOGGING "l) start logging"
#define MENU_STOP_LOGGING  "l) stop logging"
static char logging_menu_entry[80] = MENU_START_LOGGING;

static TestList start_test_list[] =
{
    {0, 0, 0, 0, "b) display basic information", start_basic, 0},
    {0, 0, 0, 0, "m) change modes", start_modes, 0},
    {0, 0, 0, 0, "t) tools", start_tools, 0},
    {MENU_COMPLETE, 0, 0, 0, "n) begin testing", 0, &normal_menu},
    {0, 0, 0, 0, logging_menu_entry, start_log, 0},
    {MENU_LAST, 0, 0, 0, 0, 0, 0}
};

static TestMenu start_menu =
{
    0, 'n', 0, "Main Menu", "tack", 0,
    0, start_test_list, 0, 0, 0
};

#if TACK_CAN_EDIT
static TestList write_terminfo_list[] =
{
    {0, 0, 0, 0, "w) write the current terminfo to a file", save_info, 0},
    {MENU_LAST, 0, 0, 0, 0, 0, 0}
};
#endif

/*****************************************************************************
 *
 * Menu command interpretation.
 *
 *****************************************************************************/

/*
**	tools_hex_echo(testlist, state, ch)
**
**	Flip the hex echo flag.
*/
static void
tools_hex_echo(
		  TestList * t GCC_UNUSED,
		  int *state GCC_UNUSED,
		  int *ch GCC_UNUSED)
{
    if (hex_out) {
	hex_out = FALSE;
	strcpy(hex_echo_menu_entry, MENU_ENABLE_HEX_OUTPUT);
    } else {
	hex_out = TRUE;
	strcpy(hex_echo_menu_entry, MENU_DISABLE_HEX_OUTPUT);
    }
}

/*
**	tools_debug(testlist, state, ch)
**
**	Change the debug level.
*/
static void
tools_debug(
	       TestList * t GCC_UNUSED,
	       int *state GCC_UNUSED,
	       int *ch)
{
    char buf[32];

    ptext("Enter a new value: ");
    read_string(buf, sizeof(buf));
    if (buf[0]) {
	sscanf(buf, "%d", &debug_level);
    }
    sprintf(temp, "Debug level is now %d", debug_level);
    ptext(temp);
    *ch = REQUEST_PROMPT;
}

/*
**	start_tools(testlist, state, ch)
**
**	Run the generic test tools
*/
static void
start_tools(
	       TestList * t GCC_UNUSED,
	       int *state GCC_UNUSED,
	       int *ch GCC_UNUSED)
{
    if (hex_out) {
	strcpy(hex_echo_menu_entry, MENU_DISABLE_HEX_OUTPUT);
    } else {
	strcpy(hex_echo_menu_entry, MENU_ENABLE_HEX_OUTPUT);
    }
    menu_display(&tools_menu, 0);
}

/*
**	tty_show_state()
**
**	Display the current state on the tty driver settings
*/
static void
tty_show_state(
		  TestMenu * menu GCC_UNUSED)
{
    put_crlf();
    (void) sprintf(temp,
		   "Accepting %d bits, UNIX delays %d, XON/XOFF %sabled, speed %u, translate %s, scan-code mode %s.",
		   (char_mask == ALLOW_PARITY) ? 8 : 7,
		   select_delay_type,
		   select_xon_xoff ? "en" : "dis",
		   tty_baud_rate,
		   translate_mode ? "on" : "off",
		   scan_mode ? "on" : "off");
    ptextln(temp);
    put_crlf();
}

/*
**	tty_width(testlist, state, ch)
**
**	Change the character width
*/
static void
tty_width(
	     TestList * t GCC_UNUSED,
	     int *state GCC_UNUSED,
	     int *ch GCC_UNUSED)
{
    if (char_mask == STRIP_PARITY) {
	char_mask = ALLOW_PARITY;
	strcpy(tty_width_menu, "7) treat terminal as 7-bit");
    } else {
	char_mask = STRIP_PARITY;
	strcpy(tty_width_menu, "8) treat terminal as 8-bit");
    }
}

/*
**	tty_delay(testlist, state, ch)
**
**	Change the delay for <cr><lf> in the TTY driver
*/
static void
tty_delay(
	     TestList * t GCC_UNUSED,
	     int *state GCC_UNUSED,
	     int *ch GCC_UNUSED)
{
    if (select_delay_type) {
	select_delay_type = FALSE;
	strcpy(tty_delay_menu,
	       "d) enable UNIX tty driver delays for <cr><lf>");
    } else {
	select_delay_type = TRUE;
	strcpy(tty_delay_menu,
	       "d) disable UNIX tty driver delays for <cr><lf>");
    }
}

/*
**	tty_xon(testlist, state, ch)
**
**	Change the XON/XOFF flags in the TTY driver
*/
static void
tty_xon(
	   TestList * t GCC_UNUSED,
	   int *state GCC_UNUSED,
	   int *ch GCC_UNUSED)
{
    if (select_xon_xoff) {
	if (needs_xon_xoff) {
	    ptextln("This terminal is marked as needing XON/XOFF protocol with (nxon)");
	}
	if (exit_xon_mode) {
	    tc_putp(exit_xon_mode);
	}
	ChangeTermInfo(xon_xoff, FALSE);
	select_xon_xoff = FALSE;
	strcpy(tty_xon_menu, enable_xon_xoff);
    } else {
	if (enter_xon_mode) {
	    tc_putp(enter_xon_mode);
	}
	ChangeTermInfo(xon_xoff, TRUE);
	select_xon_xoff = TRUE;
	strcpy(tty_xon_menu, disable_xon_xoff);
    }
    tty_set();
}

/*
**	tty_trans(testlist, state, ch)
**
**	Change the translation mode for special characters
*/
static void
tty_trans(
	     TestList * t GCC_UNUSED,
	     int *state GCC_UNUSED,
	     int *ch GCC_UNUSED)
{
    if (translate_mode) {
	translate_mode = FALSE;
	strcpy(tty_trans_menu,
	       "t) use terminfo values for \\b\\f\\n\\r\\t");
    } else {
	translate_mode = TRUE;
	strcpy(tty_trans_menu,
	       "t) override terminfo values for \\b\\f\\n\\r\\t");
    }
}

/*
**	pad_gen(testlist, state, ch)
**
**	Menu function for automatic pad generation
*/
static void
pad_gen(
	   TestList * t,
	   int *state GCC_UNUSED,
	   int *ch)
{
    control_init();
    if (tty_can_sync == SYNC_NOT_TESTED) {
	verify_time();
    }
    auto_pad_mode = TRUE;
    menu_display(t->sub_menu, ch);
    auto_pad_mode = FALSE;
}

/*
**	start_modes(testlist, state, ch)
**
**	Change the TTY modes
*/
static void
start_modes(
	       TestList * t GCC_UNUSED,
	       int *state GCC_UNUSED,
	       int *ch GCC_UNUSED)
{

    if (select_delay_type) {
	strcpy(tty_delay_menu,
	       "d) disable UNIX tty driver delays for <cr><lf>");
    } else {
	strcpy(tty_delay_menu,
	       "d) enable UNIX tty driver delays for <cr><lf>");
    }
    if (char_mask == ALLOW_PARITY) {
	strcpy(tty_width_menu,
	       "7) treat terminal as 7-bit");
    } else {
	strcpy(tty_width_menu,
	       "8) treat terminal as 8-bit");
    }
    if (select_xon_xoff) {
	strcpy(tty_xon_menu, disable_xon_xoff);
    } else {
	strcpy(tty_xon_menu, enable_xon_xoff);
    }
    if (translate_mode) {
	strcpy(tty_trans_menu,
	       "t) override terminfo values for \\b\\f\\n\\r\\t");
    } else {
	strcpy(tty_trans_menu,
	       "t) use terminfo values for \\b\\f\\n\\r\\t");
    }
    menu_display(&tty_menu, 0);
    tty_set();
}

/*
**	start_basic(testlist, state, ch)
**
**	Display basic terminal information
*/
static void
start_basic(
	       TestList * t GCC_UNUSED,
	       int *state GCC_UNUSED,
	       int *ch)
{
    display_basic();
    *ch = REQUEST_PROMPT;
}

/*
**	start_log(testlist, state, ch)
**
**	Start/stop in logging function
*/
static void
start_log(
	     TestList * t GCC_UNUSED,
	     int *state GCC_UNUSED,
	     int *ch GCC_UNUSED)
{
    if (!strcmp(logging_menu_entry, MENU_START_LOGGING)) {
	ptextln("The log file will capture all characters sent to the terminal.");
	if ((log_fp = fopen(LOG_FILENAME, "w"))) {
	    ptextln("Start logging to file: " LOG_FILENAME);
	    strcpy(logging_menu_entry, MENU_STOP_LOGGING);
	} else {
	    ptextln("File open error: " LOG_FILENAME);
	}
    } else {
	if (log_fp) {
	    fclose(log_fp);
	    log_fp = 0;
	}
	ptextln("Terminal output logging stopped.");
	strcpy(logging_menu_entry, MENU_START_LOGGING);
    }
}

/*
**	show_usage()
**
**	Tell the user how its done.
*/
void
show_usage(
	      const char *name)
{
    (void) fprintf(stderr, "usage: %s [-diltV] [term]\n", name);
}

/*
**	print_version()
**
**	Print version and other useful information.
*/
void
print_version(void)
{
    printf("tack version %d.%02d (%d)\n",
	   MAJOR_VERSION,
	   MINOR_VERSION,
	   PATCH_VERSION);
    printf("Copyright 2017-2024 Thomas E. Dickey\n");
    printf("Copyright 1997-2017 Free Software Foundation, Inc.\n");
    printf("Tack comes with NO WARRANTY, to the extent permitted by law.\n");
    printf("You may redistribute copies of Tack under the terms of the\n");
    printf("GNU General Public License.  For more information about\n");
    printf("these matters, see the file named COPYING.\n");
}

#ifdef DEBUG
void
TackMsg(const char *fmt, ...)
{
    static FILE *my_fp;
    static const char *my_filename = "TackMsg.out";
    va_list ap;

    if (my_fp == 0) {
	if ((my_fp = fopen(my_filename, "w")) == 0) {
	    fprintf(stderr, "Cannot open %s\n", my_filename);
	    ExitProgram(EXIT_FAILURE);
	}
    }
    va_start(ap, fmt);
    vfprintf(my_fp, fmt, ap);
    va_end(ap);
    fflush(my_fp);
}
#endif

static char *
validate_term(char *value)
{
    char *result = NULL;
    if (value != NULL && *value != '\0' && strlen(value) <= 128) {
	int n;
	result = value;
	for (n = 0; value[n] != '\0'; ++n) {
	    int ch = UChar(value[n]);
	    if (ch >= 128 || ch == '/' || !isgraph(ch)) {
		result = NULL;
		break;
	    }
	}
    }
    if (result == NULL) {
	fprintf(stderr, "no useful value found for $TERM\n");
	ExitProgram(EXIT_FAILURE);
    }
    return strdup(result);
}

/*****************************************************************************
 *
 * Main sequence
 *
 *****************************************************************************/

int
main(int argc, char *argv[])
{
#if TACK_CAN_EDIT
    int i = 0, j = 0;
#endif
    int ch;

    /* scan the option flags */
    send_reset_init = TRUE;
    translate_mode = FALSE;
    tty_can_sync = SYNC_NOT_TESTED;
    while ((ch = getopt(argc, argv, "diltV")) != -1) {
	switch (ch) {
	case 'V':
	    print_version();
	    ExitProgram(EXIT_FAILURE);
	    /* NOTREACHED */
	case 'd':
	    if ((debug_fp = fopen(DBG_FILENAME, "w")) == 0) {
		perror(DBG_FILENAME);
		ExitProgram(EXIT_FAILURE);
	    }
	    break;
	case 'i':
	    send_reset_init = FALSE;
	    break;
	case 'l':
	    if ((log_fp = fopen(LOG_FILENAME, "w"))) {
		strcpy(logging_menu_entry, MENU_STOP_LOGGING);
	    } else {
		perror(LOG_FILENAME);
		ExitProgram(EXIT_FAILURE);
	    }
	    break;
	case 't':
	    translate_mode = FALSE;
	    break;
	default:
	    show_usage(argv[0]);
	    ExitProgram(EXIT_SUCCESS);
	    /* NOTREACHED */
	}
    }

    if (optind >= argc) {
	tty_basename = validate_term(getenv("TERM"));
    } else if (optind + 1 >= argc) {
	tty_basename = validate_term(argv[optind]);
    } else {
	show_usage(argv[0]);
	ExitProgram(EXIT_FAILURE);
    }

    curses_setup(argv[0]);

    menu_can_scan(&normal_menu);	/* extract which caps can be tested */
    menu_display(&start_menu, 0);

#if TACK_CAN_EDIT
    if (user_modified()) {
	sprintf(temp, "Hit y to save changes to file: %.256s  ? ",
		tty_basename);
	ptext(temp);
	if (wait_here() == 'y') {
	    save_info(write_terminfo_list, &i, &j);
	}
    }
#endif

    put_str("\nTerminal test complete\n");
    bye_kids(EXIT_SUCCESS);
    ExitProgram(EXIT_SUCCESS);
}

#if NO_LEAKS
void
ExitProgram(int code)
{
    free(tty_basename);
    del_curterm(cur_term);
    tack_edit_leaks();
    tack_fun_leaks();
#if defined(HAVE_EXIT_TERMINFO)
    exit_terminfo(code);
#elif defined(HAVE__NC_FREE_TINFO)
    _nc_free_tinfo(code);
#else
    exit(code);
#endif
}
#endif
