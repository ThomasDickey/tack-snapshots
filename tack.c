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

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "tac.h"

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
int time_pad;			/* run the time tests */
int char_mask;			/* either 0xFF else 0x7F, eight bit data mask */
int select_delay_type;		/* set handler delays for <cr><lf> */
int select_xon_xoff;		/* TTY driver XON/XOFF mode select */
int hex_out;			/* Display output in hex */
int send_reset_init;		/* Send the reset and initialization strings */

static bool expert;		/* expert mode -- suppress verbose prompts */

/*****************************************************************************
 *
 * Menu definitions
 *
 *****************************************************************************/

static void tools_status(struct test_list *, int *, int *);
static void tools_sgr(struct test_list *, int *, int *);
static void tools_charset(struct test_list *, int *, int *);
static void tools_echo(struct test_list *, int *, int *);
static void tools_reply(struct test_list *, int *, int *);
static void tools_hex_echo(struct test_list *, int *, int *);

static char hex_echo_menu_entry[80];

struct test_list tools_test_list[] = {
	{0, 0, 0, 0, "s) ANSI status reports", tools_status, 0},
	{0, 0, 0, 0, "g) ANSI SGR modes (bold, underline, reverse)", tools_sgr, 0},
	{0, 0, 0, 0, "c) ANSI character sets", tools_charset, 0},
	{0, 0, 0, 0, hex_echo_menu_entry, tools_hex_echo, 0},
	{0, 0, 0, 0, "e) echo tool", tools_echo, 0},
	{0, 0, 0, 0, "r) reply tool", tools_reply, 0},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

struct test_menu tools_menu = {
	0, "[q] > ", 0, "Tools Menu", "tools",
	0, 0, tools_test_list, 0, 0, 0
};

extern struct test_menu sync_menu;

static void tty_width(struct test_list *, int *, int *);
static void tty_delay(struct test_list *, int *, int *);
static void tty_xon(struct test_list *, int *, int *);
static void tty_trans(struct test_list *, int *, int *);
static void tty_show_state(struct test_menu *);

static char tty_width_menu[80];
static char tty_delay_menu[80];
static char tty_xon_menu[80];
static char tty_trans_menu[80];

struct test_list tty_test_list[] = {
	{0, 0, 0, 0, tty_width_menu, tty_width, 0},
	{0, 0, 0, 0, tty_delay_menu, tty_delay, 0},
	{0, 0, 0, 0, tty_xon_menu, tty_xon, 0},
	{0, 0, 0, 0, tty_trans_menu, tty_trans, 0},
	{0, 0, 0, 0, "p) performance testing", 0, &sync_menu},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

struct test_menu tty_menu = {
	0, "[q] > ", 0, "Terminal and driver configuration",
	"tty", 0,
	tty_show_state, tty_test_list, 0, 0, 0
};

extern struct test_list edit_test_list[];

struct test_menu edit_menu = {
	0, "[q] > ", 0, "Edit terminfo menu",
	"edit", 0,
	0, edit_test_list, 0, 0, 0
};

extern struct test_list mode_test_list[];

struct test_menu mode_menu = {
	MENU_NEXT, "[n] > ", 0, "Mode test menu",
	"mode", "n) run standard tests",
	0, mode_test_list, 0, 0, 0
};

extern struct test_list acs_test_list[];

struct test_menu acs_menu = {
	MENU_NEXT, "[n] > ", 0,
	"Alternate character set and graphics rendition test menu",
	"acs", "n) run standard tests",
	0, acs_test_list, 0, 0, 0
};

extern struct test_list color_test_list[];

struct test_menu color_menu = {
	MENU_NEXT, "[n] > ", 0,
	"Color test menu",
	"color", "n) run standard tests",
	0, color_test_list, 0, 0, 0
};

extern struct test_list crum_test_list[];

struct test_menu crum_menu = {
	MENU_NEXT, "[n] > ", 0,
	"Cursor movement test menu",
	"move", "n) run standard tests",
	0, crum_test_list, 0, 0, 0
};

extern struct test_list funkey_test_list[];

struct test_menu funkey_menu = {
	MENU_NEXT, "[n] > ", 0,
	"Function key test menu",
	"fkey", "n) run standard tests",
	sync_test, funkey_test_list, 0, 0, 0
};

extern struct test_list printer_test_list[];

struct test_menu printer_menu = {
	0, "[n] > ", 0,
	"Printer test menu",
	"printer", "n) run standard tests",
	0, printer_test_list, 0, 0, 0
};

extern struct test_list pad_test_list[];

struct test_menu pad_menu = {
	MENU_NEXT, "[n] > ", 0,
	"Pad test menu",
	"pad", "n) run standard tests",
	sync_test, pad_test_list, 0, 0, 0
};

struct test_list normal_test_list[] = {
	{0, 0, 0, 0, "e) edit terminfo", 0, &edit_menu},
	{MENU_NEXT, 0, 0, 0, "x) test modes and glitches", 0, &mode_menu},
	{MENU_NEXT, 0, 0, 0, "a) test alternate character sets", 0, &acs_menu},
	{MENU_NEXT, 0, 0, 0, "c) test color", 0, &color_menu},
	{MENU_NEXT, 0, 0, 0, "m) test cursor movement", 0, &crum_menu},
	{MENU_NEXT, 0, 0, 0, "f) test function keys", 0, &funkey_menu},
	{MENU_NEXT, 0, 0, 0, "p) test everything else", 0, &pad_menu},
	{0, 0, 0, 0, "P) test printer", 0, &printer_menu},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};


struct test_menu normal_menu = {
	MENU_NEXT, "[n] > ", 0, "Main test menu",
	"test", "n) run standard tests",
	0, normal_test_list, 0, 0, 0
};

static void start_tools(struct test_list *, int *, int *);
static void start_modes(struct test_list *, int *, int *);
static void start_basic(struct test_list *, int *, int *);
static void start_log(struct test_list *, int *, int *);

static char logging_menu_entry[80] = "l) start logging";

struct test_list start_test_list[] = {
	{0, 0, 0, 0, "b) display basic information", start_basic, 0},
	{0, 0, 0, 0, "t) tools", start_tools, 0},
	{0, 0, 0, 0, "m) change modes", start_modes, 0},
	{0, 0, 0, 0, "n) begin testing", 0, &normal_menu},
	{0, 0, 0, 0, logging_menu_entry, start_log, 0},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

struct test_menu start_menu = {
	0, "[n] > ", 0,
	"Main Menu", "tack",
	0, 0, start_test_list, 0, 0, 0
};

struct test_list write_terminfo_list[] = {
	{0, 0, 0, 0, "w) write the current terminfo to a file", save_info, 0},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

/*****************************************************************************
 *
 * Menu command interpretation.
 *
 *****************************************************************************/

/*
**	tools_status(testlist, state, ch)
**
**	Run the ANSI status report tool
*/
static void
tools_status(
	struct test_list * t,
	int *state,
	int *ch)
{
	*ch = test_ansi_reports();
}

/*
**	tools_sgr(testlist, state, ch)
**
**	Run the ANSI graphics rendition mode tool
*/
static void
tools_sgr(
	struct test_list * t,
	int *state,
	int *ch)
{
	*ch = test_ansi_sgr();
}

/*
**	tools_charset(testlist, state, ch)
**
**	Run the ANSI alt-charset mode tool
*/
static void
tools_charset(
	struct test_list * t,
	int *state,
	int *ch)
{
	test_ansi_graphics();
}

/*
**	tools_echo(testlist, state, ch)
**
**	Run the echo tool
*/
static void
tools_echo(
	struct test_list * t,
	int *state,
	int *ch)
{
	/* this call echo's all characters AS IS */
	test_report(0, hex_out);
}

/*
**	tools_reply(testlist, state, ch)
**
**	Run the reply tool
*/
static void
tools_reply(
	struct test_list * t,
	int *state,
	int *ch)
{
	/* this call echo's the first control character in ^E mode */
	test_report(1, hex_out);
}

/*
**	tools_hex_echo(testlist, state, ch)
**
**	Flip the hex echo flag.
*/
static void
tools_hex_echo(
	struct test_list * t,
	int *state,
	int *ch)
{
	if (hex_out) {
		hex_out = FALSE;
		strcpy(hex_echo_menu_entry,
			"h) enable hex output on echo tool");
	} else {
		hex_out = TRUE;
		strcpy(hex_echo_menu_entry,
			"h) disable hex output on echo tool");
	}
}

/*
**	start_tools(testlist, state, ch)
**
**	Run the generic test tools
*/
static void
start_tools(
	struct test_list * t,
	int *state,
	int *ch)
{
	if (hex_out) {
		strcpy(hex_echo_menu_entry,
			"h) disable hex output on echo tool");
	} else {
		strcpy(hex_echo_menu_entry,
			"h) enable hex output on echo tool");
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
	struct test_menu *menu)
{
	put_crlf();
	(void) sprintf(temp,
		"Accepting %d bits, UNIX delays %d, XON/XOFF %sabled, speed %ld, translate %s, scan-code mode %s.",
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
	struct test_list * t,
	int *state,
	int *ch)
{
	if (*ch == '8') {
		char_mask = ALLOW_PARITY;
		strcpy(tty_width_menu, "7) treat terminal as 7-bit");
	} else if (*ch == '7') {
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
	struct test_list * t,
	int *state,
	int *ch)
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
	struct test_list * t,
	int *state,
	int *ch)
{
	if (select_xon_xoff) {
		select_xon_xoff = FALSE;
		strcpy(tty_xon_menu,
			"x) enable xon/xoff in tty handler");
	} else {
		select_xon_xoff = TRUE;
		strcpy(tty_xon_menu,
			"x) disable xon/xoff in tty handler");
	}
}

/*
**	tty_trans(testlist, state, ch)
**
**	Change the translation mode for special characters
*/
static void
tty_trans(
	struct test_list * t,
	int *state,
	int *ch)
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
**	start_modes(testlist, state, ch)
**
**	Change the TTY modes
*/
static void
start_modes(
	struct test_list * t,
	int *state,
	int *ch)
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
		strcpy(tty_xon_menu,
			"x) disable xon/xoff in tty handler");
	} else {
		strcpy(tty_xon_menu,
			"x) enable xon/xoff in tty handler");
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
	struct test_list * t,
	int *state,
	int *ch)
{
	display_basic();
}

/*
**	start_log(testlist, state, ch)
**
**	Start/stop in logging function
*/
static void
start_log(
	struct test_list * t,
	int *state,
	int *ch)
{
	ptextln("running log");
	if (logging_menu_entry[5] == 'a') {
		strcpy(logging_menu_entry, "l) stop logging");
	} else {
		strcpy(logging_menu_entry, "l) start logging");
	}
}

/*****************************************************************************
 *
 * Main sequence
 *
 *****************************************************************************/

int
main(int argc, char *argv[])
{
	int i, j;
	char *term_variable;

	/* scan the option flags */
	send_reset_init = TRUE;
	translate_mode = FALSE;
	term_variable = getenv("TERM");
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch (argv[i][j]) {
				case 'V':
					ptextln("tac version 0.0");
					return (1);
				case 'i':
					send_reset_init = FALSE;
					break;
				case 't':
					translate_mode = FALSE;
					break;
				case 'x':
					expert = TRUE;
					break;
				default:
					(void) fprintf(stderr,
						"usage: %s [-itxV] [term]\n",
						argv[0]);
					return (0);
				}
			}
		} else {
			term_variable = argv[i];
		}
	}
	(void) strcpy(tty_basename, term_variable);
	tty_init();
	char_count = 0;
	setupterm(tty_basename, 1, (int *) 0);

	/* set up the defaults */
	replace_mode = TRUE;
	scan_mode = 0;
	time_pad = 0;
	select_delay_type = debug_level = 0;
	char_mask = (meta_on && meta_on[0] == '\0') ? ALLOW_PARITY : STRIP_PARITY;
	select_xon_xoff = needs_xon_xoff ? 1 : 0;

	fflush(stdout);	/* flush any output */
	tty_set();

	go_home();	/* set can_go_home */
	put_clear();	/* set can_clear_screen */

	reset_init(send_reset_init);

	tty_can_sync = SYNC_NOT_TESTED;

	/*
	   I assume that the reset and init strings may not have the correct
	   pads.  (Because that part of the test comes much later.)  Because
	   of this, I allow the terminal some time to catch up.
	*/
	fflush(stdout);	/* waste some time */
	sleep(1);	/* waste more time */
	menu_can_scan(&normal_menu);	/* extract which caps can be tested */
	charset_can_test();
	edit_init();			/* initialize the edit data base */

	if (send_reset_init && enter_ca_mode) {
		tc_putp(enter_ca_mode);
		put_clear();	/* just in case we switched pages */
	}
	put_crlf();
	ptextln("Welcome to tack.");

	display_basic();

	put_crlf();
	menu_display(&start_menu, 0);

	if (user_modified()) {
		sprintf(temp, "Hit y to save changes to file: %s  ? ",
			tty_basename);
		ptext(temp);
		if (wait_here() == 'y') {
			save_info(write_terminfo_list, &i, &j);
		}
	}

	put_str("\nTerminal test complete\n");
	bye_kids(0);
	return (0);
}
