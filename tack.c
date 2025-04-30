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
#include <stdarg.h>
#include <unistd.h>

MODULE_ID("$Id: tack.c,v 1.45 2025/04/29 20:03:37 tom Exp $")

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
    /* *INDENT-OFF* */
    {0, 0, NULL, NULL, "s) ANSI status reports", tools_status, NULL},
    {0, 0, NULL, NULL, "g) ANSI SGR modes (bold, underline, reverse)", tools_sgr, NULL},
    {0, 0, NULL, NULL, "c) ANSI character sets", tools_charset, NULL},
    {0, 0, NULL, NULL, hex_echo_menu_entry, tools_hex_echo, NULL},
    {0, 0, NULL, NULL, "e) echo tool", tools_report, NULL},
    {1, 0, NULL, NULL, "r) reply tool", tools_report, NULL},
    {0, 0, NULL, NULL, "p) performance testing", NULL, &sync_menu},
    {0, 0, NULL, NULL, "i) send reset and init", menu_reset_init, NULL},
    {0, 0, "u8) (u9", NULL, "u) test ENQ/ACK (DA1) handshake", sync_handshake, NULL},
    {0, 0, NULL, NULL, "2) test RV/rv secondary attributes (DA2)", ask_DA2, NULL},
    {0, 0, NULL, NULL, "v) test XR/xr version (XTVERSION)", ask_version, NULL},
    {0, 0, NULL, NULL, "d) change debug level", tools_debug, NULL},
    {MENU_LAST, 0, NULL, NULL, NULL, NULL, NULL}
    /* *INDENT-ON* */
};

static TestMenu tools_menu =
{
    0, 'q', NULL, "Tools Menu", "tools",
    NULL, NULL, tools_test_list, NULL, 0, 0
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
    {0, 0, NULL, NULL, tty_width_menu, tty_width, NULL},
    {0, 0, NULL, NULL, tty_delay_menu, tty_delay, NULL},
    {0, 0, NULL, NULL, tty_xon_menu, tty_xon, NULL},
    {0, 0, NULL, NULL, tty_trans_menu, tty_trans, NULL},
    {MENU_LAST, 0, NULL, NULL, NULL, NULL, NULL}
};

static TestMenu tty_menu =
{
    0, 'q', NULL, "Terminal and driver configuration",
    "tty", NULL,
    tty_show_state, tty_test_list, NULL, 0, 0
};

#if TACK_CAN_EDIT
TestMenu edit_menu =
{
    0, 'q', NULL, "Edit terminfo menu",
    "edit", NULL,
    NULL, edit_test_list, NULL, 0, 0
};
#endif

static TestMenu mode_menu =
{
    0, 'n', NULL, "Test modes and glitches:",
    "mode", "n) run standard tests",
    NULL, mode_test_list, NULL, 0, 0
};

static TestMenu acs_menu =
{
    0, 'n', NULL,
    "Test alternate character set and graphics rendition:",
    "acs", "n) run standard tests",
    NULL, acs_test_list, NULL, 0, 0
};

static TestMenu color_menu =
{
    0, 'n', NULL,
    "Test color:",
    "color", "n) run standard tests",
    NULL, color_test_list, NULL, 0, 0
};

static TestMenu crum_menu =
{
    0, 'n', NULL,
    "Test cursor movement:",
    "move", "n) run standard tests",
    NULL, crum_test_list, NULL, 0, 0
};

static TestMenu funkey_menu =
{
    0, 'n', NULL,
    "Test function keys:",
    "fkey", "n) run standard tests",
    sync_test, funkey_test_list, NULL, 0, 0
};

static TestMenu printer_menu =
{
    0, 'n', NULL,
    "Test printer:",
    "printer", "n) run standard tests",
    NULL, printer_test_list, NULL, 0, 0
};

static void pad_gen(TestList *, int *, int *);

static TestMenu pad_menu =
{
    0, 'n', NULL,
    "Test padding and string capabilities:",
    "pad", "n) run standard tests",
    sync_test, pad_test_list, NULL, 0, 0
};
/* *INDENT-OFF* */
static TestList normal_test_list[] = {
    MY_EDIT_MENU
    {0, 0, NULL, NULL, "i) send reset and init", menu_reset_init, NULL},
    {MENU_NEXT, 0, NULL, NULL, "x) test modes and glitches", NULL, &mode_menu},
    {MENU_NEXT, 0, NULL, NULL, "a) test alternate character set and graphic rendition", NULL, &acs_menu},
    {MENU_NEXT, 0, NULL, NULL, "c) test color", NULL, &color_menu},
    {MENU_NEXT, 0, NULL, NULL, "m) test cursor movement", NULL, &crum_menu},
    {MENU_NEXT, 0, NULL, NULL, "f) test function keys", NULL, &funkey_menu},
    {MENU_NEXT, 0, NULL, NULL, "p) test padding and string capabilities", NULL, &pad_menu},
    {0, 0, NULL, NULL, "P) test printer", NULL, &printer_menu},
    {MENU_MENU, 0, NULL, NULL, "/) test a specific capability", NULL, NULL},
    {0, 0, NULL, NULL, "t) auto generate pad delays", pad_gen, &pad_menu},
    {0, 0, "u8) (u9", NULL, NULL, sync_handshake, NULL},
    {MENU_LAST, 0, NULL, NULL, NULL, NULL, NULL}
};
/* *INDENT-ON* */

static TestMenu normal_menu =
{
    0, 'n', NULL, "Main test menu",
    "test", "n) run standard tests",
    NULL, normal_test_list, NULL, 0, 0
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
    {0, 0, NULL, NULL, "b) display basic information", start_basic, NULL},
    {0, 0, NULL, NULL, "m) change modes", start_modes, NULL},
    {0, 0, NULL, NULL, "t) tools", start_tools, NULL},
    {MENU_COMPLETE, 0, NULL, NULL, "n) begin testing", NULL, &normal_menu},
    {0, 0, NULL, NULL, logging_menu_entry, start_log, NULL},
    {MENU_LAST, 0, NULL, NULL, NULL, NULL, NULL}
};

static TestMenu start_menu =
{
    0, 'n', NULL, "Main Menu", "tack", NULL,
    NULL, start_test_list, NULL, 0, 0
};

#if TACK_CAN_EDIT
static TestList write_terminfo_list[] =
{
    {0, 0, NULL, NULL, "w) write the current terminfo to a file", save_info, NULL},
    {MENU_LAST, 0, NULL, NULL, NULL, NULL, NULL}
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
    menu_display(&tools_menu, NULL);
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
    menu_display(&tty_menu, NULL);
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
	    log_fp = NULL;
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
    printf("Copyright 2017-2025 Thomas E. Dickey\n");
    printf("Copyright 1997-2017 Free Software Foundation, Inc.\n");
    printf("Tack comes with NO WARRANTY, to the extent permitted by law.\n");
    printf("You may redistribute copies of Tack under the terms of the GNU General Public\n");
    printf("License version 2.  For more information about these matters, see the file\n");
    printf("named COPYING.\n");
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
	    if ((debug_fp = fopen(DBG_FILENAME, "w")) == NULL) {
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
    menu_display(&start_menu, NULL);

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
