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

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include "tack.h"

/* terminal-synchronization and performance tests */

static void sync_home(struct test_list *, int *, int *);
static void sync_lines(struct test_list *, int *, int *);
static void sync_clear(struct test_list *, int *, int *);
static void sync_summary(struct test_list *, int *, int *);

struct test_list sync_test_list[] = {
	{MENU_NEXT, 0, 0, 0, "b) baud rate test", sync_home, 0},
	{MENU_NEXT, 0, 0, 0, "l) scroll performance", sync_lines, 0},
	{MENU_NEXT, 0, 0, 0, "c) clear screen performance", sync_clear, 0},
	{MENU_NEXT, 0, 0, 0, "p) summary of results", sync_summary, 0},
	{0, 0, 0, 0, txt_longer_test_time, longer_test_time, 0},
	{0, 0, 0, 0, txt_shorter_test_time, shorter_test_time, 0},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

struct test_menu sync_menu = {
	0, 'n', 0,
	"Performance tests", "perf", "n) run standard tests",
	sync_test, sync_test_list, 0, 0, 0
};

extern char letters[];

int tty_can_sync;		/* TRUE if tty_sync_error() returned FALSE */
int tty_newline_rate;		/* The number of newlines per second */
int tty_clear_rate;		/* The number of clear-screens per second */
int tty_cps;			/* The number of characters per second */

#define TTY_ACK_SIZE 64

int ACK_char;			/* terminating ACK character */
int junk_chars;			/* number of junk characters */
char *tty_ENQ;			/* enquire string */
char tty_ACK[TTY_ACK_SIZE];	/* ACK response, set by tty_sync_error() */

/*****************************************************************************
 *
 * Terminal synchronization.
 *
 *	These functions handle the messy business of enq-ack handshaking
 *	for timing purposes.
 *
 *****************************************************************************/

int
tty_sync_error(
	int term_char)
{
	int ch, ct, trouble, ack;
	long read_time;

	trouble = FALSE;
	for (;;) {
		tc_putp(tty_ENQ);	/* send ENQ */
		ch = getnext(STRIP_PARITY);
		read_time = time(0);	/* start the timer */
		tty_ACK[ack = 0] = ch;
		for (ct = junk_chars; ct > 0; ct--) {
			if (ch == ACK_char)
				return trouble;
			ch = getnext(STRIP_PARITY);
			if (ack < TTY_ACK_SIZE - 1)
				tty_ACK[++ack] = ch;
			if (ch == term_char) {
				return TRUE;
			}
			if (term_char && (ct == 1)) {
				junk_chars++;
				ct++;
			}
		}
		tty_ACK[ack + 1] = '\0';
		if (ch == ACK_char)
			return trouble;
		if (term_char) {	/* called from verify_time() */
			put_crlf();
			putln("Terminal did not respond to ENQ");
			tty_can_sync = SYNC_FAILED;
			return (ch | 4096);
		}
		/*
		   If we get here the terminal is sending too many
		   characters.  The trick is to find out if the characters
		   are comming from the terminal or the user.  We assume the
		   terminal will send only while it is processing commands in
		   its buffer, and that the buffer will empty in a short
		   poriod of time.  Any characters sent after a few seconds
		   must be from the user.
		*/

		do {
			ch = getchp(STRIP_PARITY);
			if (time(0) - read_time > 3) {
				putchp(7);	/* ring the bell */
				break;
			}
		} while (ch != 'c' && ch != 'C');
		set_attr(0);	/* just in case */
		putln(" -- sync -- ");
		trouble = TRUE;
	}
}

void 
flush_input(void)
/* crude but effective way to flush input */
{
	if (tty_can_sync == SYNC_TESTED)
		tty_sync_error(0);
}

int
enq_ack(void)
/* send an ENQ, get an ACK */
{
	int sync_error;

	for (sync_error = 0; tty_sync_error(0); sync_error++) {
		sleep(1);
		if (sync_error > 2) {
			ptext("\nPad processing terminated: ");
			(void) wait_here();
			time_pad = FALSE;
			return FALSE;
		}
	}
	char_sent = 0;
	return TRUE;
}

/*****************************************************************************
 *
 * Terminal performance tests
 *
 *	The only entry point of this group of functions is verify_time(),
 *	which determines the terminal's effective baud rate.  It is called
 *	at the beginning of the pad-test and function-key-test routines.
 *
 *****************************************************************************/

/*
**	sync_home(test_list, status, ch)
**
**	Baudrate test
*/
static void
sync_home(
	struct test_list *t,
	int *state,
	int *ch)
{
	int j, k;

	if (!cursor_home && !cursor_address && !row_address) {
		ptext("Terminal can not home cursor.  ");
		generic_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(home) Start baudrate search")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		for (j = 1; j < lines; j++) {
			for (k = 0; k < columns; k++) {
				if (k & 0xF) {
					put_this(letter);
				} else {
					put_this('.');
				}
			}
			SLOW_TERMINAL_EXIT;
		}
		NEXT_LETTER;
	}
	pad_test_shutdown(t, auto_right_margin == 0);
	/* note:  tty_frame_size is the real framesize times two.
	   This takes care of half bits. */
	j = (tx_cps * tty_frame_size) >> 1;
	if (j > tty_baud_rate) {
		tty_baud_rate = j;
	}
	if (tx_cps > tty_cps) {
		tty_cps = tx_cps;
	}
	sprintf(temp, "%d characters per second.  Baudrate %d  ", tx_cps, j);
	ptext(temp);
	generic_done_message(t, state, ch);
}

/*
**	sync_lines(test_list, status, ch)
**
**	How many newlines/second?
*/
static void
sync_lines(
	struct test_list *t,
	int *state,
	int *ch)
{
	int j;

	if (skip_pad_test(t, state, ch,
		"(nel) Start scroll performance test")) {
		return;
	}
	pad_test_startup(0);
	reps = 100;
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d", test_complete);
		put_str(temp);
		put_newlines(reps);
	}
	pad_test_shutdown(t, 0);
	j = sliding_scale(tx_count[0], 1000000, usec_run_time);
	if (j > tty_newline_rate) {
		tty_newline_rate = j;
	}
	sprintf(temp, "%d linefeeds per second.  ", j);
	ptext(temp);
	generic_done_message(t, state, ch);
}

/*
**	sync_clear(test_list, status, ch)
**
**	How many clear-screens/second?
*/
static void
sync_clear(
	struct test_list *t,
	int *state,
	int *ch)
{
	int j;

	if (!clear_screen) {
		ptext("Terminal can not clear-screen.  ");
		generic_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(clear) Start clear-screen performance test")) {
		return;
	}
	pad_test_startup(0);
	reps = 20;
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d", test_complete);
		put_str(temp);
		for (j = 0; j < reps; j++) {
			put_clear();
		}
	}
	pad_test_shutdown(t, 0);
	j = sliding_scale(tx_count[0], 1000000, usec_run_time);
	if (j > tty_clear_rate) {
		tty_clear_rate = j;
	}
	sprintf(temp, "%d clear-screens per second.  ", j);
	ptext(temp);
	generic_done_message(t, state, ch);
}

/*
**	sync_symmary(test_list, status, ch)
**
**	Print out the test results.
*/
static void
sync_summary(
	struct test_list *t,
	int *state,
	int *ch)
{
	char size[32];

	put_crlf();
	ptextln("Terminal  size    characters/sec linefeeds/sec  clears/sec");
	sprintf(size, "%dx%d", columns, lines);
	sprintf(temp, "%-10s%-11s%11d   %11d %11d", tty_basename, size,
		tty_cps, tty_newline_rate, tty_clear_rate);
	ptextln(temp);
	generic_done_message(t, state, ch);
}

/*
**	probe_enq_ok()
**
**	does the terminal do enq/ack handshaking?
*/
static void 
probe_enq_ok(void)
{
	int i;
	long read_time;

	/* set up enq/ack sequences */
	tty_ENQ = user9 ? user9 : "\005";
	junk_chars = 0;
	ACK_char = 6;
	can_test("u8 u9", FLAG_TESTED);
	if (user8) {
		char *cp;

		junk_chars = 0;
		for (cp = user8; *cp; cp++) {
			if (*cp != '%') {
				junk_chars++;
			} else {
				++cp;
				junk_chars++;
			}
		}
		ACK_char = cp[-1];
	}
	put_str("Hit lower case g to start testing...");
	fflush(stdout);
	i = tty_sync_error('g');
	if (i == FALSE) {
		i = getchp(STRIP_PARITY);
	}
	if (i != 'g') {
		/*
		   Either the terminal did not respond or the user does not
		   like us.  If the terminal returned nothing then 'i' will
		   equal 'g'.  If the user refuses to type a 'g' or has
		   forgot then wait for the letter 'g' for 2 seconds then
		   give up.
		*/
		tty_can_sync = SYNC_FAILED;
		ptext("\nThis program expects the ENQ sequence to be");
		ptext(" answered with the ACK character.  This will help");
		ptext(" the program reestablish synchronization when");
		ptextln(" the terminal is overrun with data.");
		ptext("\nENQ sequence from (u9): ");
		putln(expand(tty_ENQ));
		ptext("ACK recieved: ");
		putln(expand(tty_ACK));
		sprintf(temp, "Length of ACK %d.  Expected length of ACK %d.",
			(int) strlen(tty_ACK), junk_chars + 1);
		ptextln(temp);
		temp[0] = ACK_char;
		temp[1] = '\0';
		ptext("Terminating character found in (u8): ");
		putln(expand(temp));
		put_crlf();
		put_str("Please, hit lower case g to continue:");
		fflush(stdout);
		read_time = time(0);
		for (i = 1; i < 20; i++) {
			if (wait_here() == 'g') {
				break;
			}
			if (time(0) - read_time > 2) {
				break;
			}
		}
		return;
	}
	tty_can_sync = SYNC_TESTED;
}

/*
**	verify_time()
**
**	verify that the time tests are ready to run.
**	If the baud rate is not set then compute it.
*/
void
verify_time(void)
{
	int status, ch;

	if (tty_can_sync == SYNC_FAILED) {
		return;
	}
	probe_enq_ok();
	put_crlf();
	if (tty_baud_rate == 0) {
		sync_home(&sync_test_list[0], &status, &ch);
	}
}

/*
**	sync_test(menu)
**
**	Run at the beginning of the pad tests and function key tests
*/
void
sync_test(
	struct test_menu *menu)
{
	control_init();
	if (tty_can_sync == SYNC_NOT_TESTED) {
		verify_time();
	}
	if (menu->menu_title) {
		ptextln(menu->menu_title);
	}
}
