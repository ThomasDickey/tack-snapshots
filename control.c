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

#include <unistd.h>
#include <sys/time.h>	/* for sun */
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include "tack.h"

/* terminfo test program control subroutines */

/* globals */
int test_complete;		/* counts number of tests completed */

char txt_longer_test_time[80];	/* +) use longer time */
char txt_shorter_test_time[80];	/* -) use shorter time */
int pad_test_duration = 1;	/* number of seconds for a pad test */
int no_alarm_event;		/* TRUE if the alarm has not gone off yet */
int usec_run_time;		/* length of last test in microseconds */
struct timeval test_start_time;	/* Time of day when the test started */
struct timezone zone;		/* Time zone at start of test */

char txt_longer_augment[80];	/* >) use bigger augment */
char txt_shorter_augment[80];	/* <) use smaller augment */

/* caps under test data base */
char *tt_cap[TT_MAX];		/* value of string */
int tt_affected[TT_MAX];	/* lines or columns effected (repitition factor) */
int tt_count[TT_MAX];		/* Number of times sent */
int ttp;			/* number of entries used */

/* Saved value of the above data base */
char *tx_cap[TT_MAX];		/* value of string */
int tx_affected[TT_MAX];	/* lines or columns effected (repitition factor) */
int tx_count[TT_MAX];		/* Number of times sent */
int tx_index[TT_MAX];		/* String index */
int txp;			/* number of entries used */
int tx_characters;		/* printing characters sent by test */
int tx_cps;			/* characters per second */
struct test_list *tx_source;	/* The test that generated this data */

extern char *pad_repeat_test;	/* commands that force repeat */
extern char letters[];
extern int raw_characters_sent;	/* Total output characters */
extern struct test_menu pad_menu;	/* Pad menu structure */
extern struct test_list pad_test_list[];
struct test_list *augment_test;

#define RESULT_BLOCK		1024
static int blocks;		/* number of result blocks available */
static struct test_results *results;	/* pointer to next available */
struct test_results *pads[STRCOUNT];	/* save pad results here */


/*****************************************************************************
 *
 * Execution control for capability tests
 *
 *****************************************************************************/

/*
**	get_next_block()
**
**	Get a results block for pad test data.
*/
struct test_results *
get_next_block(void)
{
	if (blocks <= 0) {
		results = (struct test_results *)
			malloc(sizeof(struct test_results) * RESULT_BLOCK);
		if (!results) {
			ptextln("Malloc failed");
			return (struct test_results *) 0;
		}
		blocks = RESULT_BLOCK;
	}
	blocks--;
	return results++;
}

/*
**	set_augment_txt()
**
**	Initialize the augment menu selections
*/
void
set_augment_txt(void)
{
	sprintf(txt_longer_augment,
		">) Change lines/characters effected to %d", augment << 1);
	sprintf(txt_shorter_augment,
		"<) Change lines/characters effected to %d", augment >> 1);
}

void
control_init(void)
{
	sprintf(txt_longer_test_time, "+) Change test time to %d seconds",
		pad_test_duration + 1);
	sprintf(txt_shorter_test_time, "-) Change test time to %d seconds",
		pad_test_duration - 1);
	set_augment_txt();
}


void
pad_time(char *cap, int *star, int *plain)
{				/* return the pad time in tens of miliseconds */
	int dec, i;

	*star = *plain = 0;
	if (!cap)
		return;
	for (; *cap; ++cap) {
		if (*cap == '$' && cap[1] == '<') {
			cap += 2;
			for (i = dec = 0;; ++cap) {
				if (!*cap)
					return;
				if (*cap >= '0' && *cap <= '9')
					i = i * 10 + *cap - '0';
				else if (*cap == '.')
					dec = 1;
				else if (*cap == '*') {
					if (!dec)
						i *= 10;
					*star += i;
					i = 0;
				} else if (*cap == '>') {
					if (!dec)
						i *= 10;
					*plain += i;
					break;
				} else if (*cap != '/')
					break;
			}
		}
	}
}


char *
liberated(cap)
char *cap;
{				/* return the cap without the padding */
	static char cb[1024];
	char *ts, *ls;

	cb[0] = '\0';
	ls = NULL;
	if (cap) {
		for (ts = cb; (*ts = *cap); ++cap) {
			if (*cap == '$' && cap[1] == '<') {
				ls = ts;
			}
			++ts;
			if (*cap == '>') {
				if (ls) {
					ts = ls;
					ls = NULL;
				}
			}
		}
	}
	return cb;
}


void
page_loop(void)
{				/* send CR/LF or go home and bump letter */
	if (line_count + 2 >= lines) {
		NEXT_LETTER;
		go_home();
	} else {
		put_crlf();
	}
}

/*
**	skip_pad_test(test-list-entry, state, ch, text)
**
**	Print the start test line.  Handle start up commands.
**	Return TRUE if a return is requested.
*/
int
skip_pad_test(
	struct test_list *test,
	int *state,
	int *ch,
	char *text)
{
	char rep_text[16];

	while(1) {
		if (text) {
			ptext(text);
		}
		if ((test->flags & MENU_LC_MASK)) {
			sprintf(rep_text, " *%d", augment);
			ptext(rep_text);
		}
		ptext(" [n] > ");
		*ch = wait_here();
		if (*ch == 's') {
			/* Skip is converted to next */
			*ch = 'n';
			return TRUE;
		}
		if (*ch == 'q') {
			/* Quit is converted to help */
			*ch = '?';
			return TRUE;
		}
		if (*ch == '\r' || *ch == '\n' || *ch == 'n' || *ch == 'r') {
			/* this is the only response that allows the test to run */
			*ch = 0;
		}
		if (subtest_menu(pad_test_list, state, ch)) {
			continue;
		}
		return (*ch != 0);
	}
}

/*
**	pad_done_message(test_list)
**
**	Print the Done message and request input.
*/
void
pad_done_message(
	struct test_list *test,
	int *state,
	int *ch)
{
	int default_action = 0;
	char done_message[128];
	char rep_text[16];

	while (1) {
		if ((test->flags & MENU_LC_MASK)) {
			sprintf(rep_text, "*%d", augment);
		} else {
			rep_text[0] = '\0';
		}
		if (test->caps_done) {
			sprintf(done_message, "(%s)%s Done ", test->caps_done,
			rep_text);
			ptext(done_message);
		} else {
			if (rep_text[0]) {
				ptext(rep_text);
				ptext(" ");
			}
			ptext("Done ");
		}
		if (debug_level & 2) {
			dump_test_stats(test, state, ch);
		} else {
			*ch = wait_here();
		}
		if (*ch == '\r' || *ch == '\n') {
			*ch = default_action;
			return;
		}
		if (*ch == 's' || *ch == 'n') {
			*ch = 0;
			return;
		}
		if (strchr(pad_repeat_test, *ch)) {
			/* default action is now repeat */
			default_action = 'r';
		}
		if (subtest_menu(pad_test_list, state, ch)) {
			continue;
		}
		return;
	}
}

/*
**	sliding_scale(dividend, factor, divisor)
**
**	Return (dividend * factor) / divisor
*/
int
sliding_scale(
	int dividend,
	int factor,
	int divisor)
{
	double d = dividend;

	if (divisor) {
		d = (d * (double) factor) / (double) divisor;
		return (int) (d + 0.5);
	}
	return 0;
}

/*
**	pad_test_startup()
**
**	Do the stuff needed to begin a test.
*/
void
pad_test_startup(
	int clear)
{
	if (clear) {
		put_clear();
	}
	reps = augment;
	raw_characters_sent = 0;
	test_complete = ttp = char_count = 0;
	letter = letters[letter_number = 0];
	timerclear(&test_start_time);
	if (pad_test_duration) {
		set_alarm_clock(pad_test_duration);
	}
	(void) gettimeofday(&test_start_time, &zone);
}

/*
**	pad_test_shutdown()
**
**	Do the stuff needed to end a test.
*/
void
pad_test_shutdown(
	struct test_list *t,
	int crlf)
{
	int i;
	int counts;			/* total counts */
	int ss;				/* Save string index */
	int cpo;			/* characters per operation */
	int delta;			/* difference in characters */
	struct test_results *r;		/* Results of current test */
	int ss_index[TT_MAX];		/* String index */
	struct timeval test_stop_time;

	flush_input();
	(void) gettimeofday(&test_stop_time, &zone);
	tx_source = t;
	tx_characters = raw_characters_sent;
	usec_run_time =
		((test_stop_time.tv_sec - test_start_time.tv_sec) * 1000000)
		+ test_stop_time.tv_usec - test_start_time.tv_usec;
	tx_cps = sliding_scale(tx_characters, 1000000, usec_run_time);

	/* save the data base */
	for (txp = ss = counts = 0; txp < ttp; txp++) {
		tx_cap[txp] = tt_cap[txp];
		tx_count[txp] = tt_count[txp];
		tx_affected[txp] = tt_affected[txp];
		tx_index[txp] = get_string_cap_byvalue(tt_cap[txp]);
		if (tx_index[txp] >= 0) {
			if (cap_match(t->caps_done, strnames[tx_index[txp]])) {
				ss_index[ss++] = txp;
				counts += tx_count[txp];
			}
		}
	}

	if (crlf) {
		put_crlf();
	}
	if (counts == 0 || tty_cps == 0) {
		/* nothing to do */
		return;
	}
	/* calculate the suggested pad times */
	delta = sliding_scale(tty_cps, usec_run_time, 1000000) - tx_characters;
	if (delta < 0) {
		/* probably should bump tx_characters */
		delta = 0;
	}
	for (i = 0; i < ss; i++) {
		if (!(r = get_next_block())) {
			return;
		}
		cpo = sliding_scale(delta, tx_count[ss_index[i]] * 10,
			counts * counts);
		r->next = pads[tx_index[ss_index[i]]];
		pads[tx_index[ss_index[i]]] = r;
		r->test = t;
		r->reps = tx_affected[ss_index[i]];
		r->delay = cpo;
	}
}

/*
**	show_cap_results(index)
**
**	Display the previous results
*/
void
show_cap_results(
	int x)
{
	struct test_results *r;		/* a result */
	int delay;

	if ((r = pads[x])) {
		sprintf(temp, "(%s)", strnames[x]);
		ptext(temp);
		while (r) {
			delay = (r->delay * 10000) / tty_cps;
			sprintf(temp, "$<%d>", delay);
			put_columns(temp, strlen(temp), 10);
			r = r->next;
		}
		r = pads[x];
		while (r) {
			if (r->reps > 1) {
				delay = ((r->delay * 10000) / r->reps) / tty_cps;
				sprintf(temp, "$<%d.%d*>", delay / 10, delay % 10);
				put_columns(temp, strlen(temp), 10);
			}
			r = r->next;
		}
		put_crlf();
	}
}

/*
**	dump_test_stats(test_list, status, ch)
**
**	Dump the statistics about the last test
*/
void
dump_test_stats(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;
	char tbuf[32];
	int x[32];

	put_crlf();
	if (tx_source && tx_source->caps_done) {
		cap_index(tx_source->caps_done, x);
#ifdef NOT_DONE_YET
		if (x[0] >= 0) {
			sprintf(temp, "Caps summary for (%s)",
				tx_source->caps_done);
			ptextln(temp);
			for (i = 0; x[i] >= 0; i++) {
				show_cap_results(x[i]);
			}
			put_crlf();
		}
#endif
	}
	sprintf(tbuf, "%011u", usec_run_time);
	sprintf(temp, "Test time: %d.%s, characters per second %d, characters %d",
		usec_run_time / 1000000, &tbuf[5], tx_cps, tx_characters);
	ptextln(temp);
	for (i = 0; i < txp; i++) {
		if ((j = get_string_cap_byvalue(tx_cap[i])) >= 0) {
			sprintf(tbuf, "(%s)", strnames[j]);
		} else {
			strcpy(tbuf, "(?)");
		}
		sprintf(temp, "%8d  %3d  %8s %s", tx_count[i], tx_affected[i],
			tbuf, expand(tx_cap[i]));
		putln(temp);
	}
	generic_done_message(t, state, ch);
}

/*
**	longer_test_time(test_list, status, ch)
**
**	Extend the number of seconds for each test.
*/
void
longer_test_time(
	struct test_list *t,
	int *state,
	int *ch)
{
	pad_test_duration += 1;
	sprintf(txt_longer_test_time, "+) Change test time to %d seconds",
		pad_test_duration + 1);
	sprintf(txt_shorter_test_time, "-) Change test time to %d seconds",
		pad_test_duration - 1);
	sprintf(temp, "Tests will run for %d seconds", pad_test_duration);
	ptext(temp);
	*ch = REQUEST_PROMPT;
}

/*
**	shorter_test_time(test_list, status, ch)
**
**	Shorten the number of seconds for each test.
*/
void
shorter_test_time(
	struct test_list *t,
	int *state,
	int *ch)
{
	if (pad_test_duration > 1) {
		pad_test_duration -= 1;
		sprintf(txt_longer_test_time, "+) Change test time to %d seconds",
			pad_test_duration + 1);
		sprintf(txt_shorter_test_time, "-) Change test time to %d seconds",
			pad_test_duration - 1);
	}
	sprintf(temp, "Tests will run for %d second%s", pad_test_duration,
		pad_test_duration > 1 ? "s" : "");
	ptext(temp);
	*ch = REQUEST_PROMPT;
}

/*
**	longer_augment(test_list, status, ch)
**
**	Lengthen the number of lines/characters effected
*/
void
longer_augment(
	struct test_list *t,
	int *state,
	int *ch)
{
	augment <<= 1;
	set_augment_txt();
	if (augment_test) {
		t = augment_test;
	}
	sprintf(temp, "The pad tests will effect %d %s.", augment,
		((t->flags & MENU_LC_MASK) == MENU_lines) ?
		"lines" : "characters");
	ptextln(temp);
	*ch = REQUEST_PROMPT;
}

/*
**	shorter_augment(test_list, status, ch)
**
**	Shorten the number of lines/characters effected
*/
void
shorter_augment(
	struct test_list *t,
	int *state,
	int *ch)
{
	if (augment > 1) {
		/* don't let the augment go to zero */
		augment >>= 1;
	}
	set_augment_txt();
	if (augment_test) {
		t = augment_test;
	}
	sprintf(temp, "The pad tests will effect %d %s.", augment,
		((t->flags & MENU_LC_MASK) == MENU_lines) ?
		"lines" : "characters");
	ptextln(temp);
	*ch = REQUEST_PROMPT;
}