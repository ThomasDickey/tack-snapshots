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
#include "tac.h"

/* terminfo test program control subroutines */

#define CAP_MAX 64
#define EDIT_MAX 4

struct cap_tab {
   char *name;
   char *value;
   int cur_star, cur_plain;
   int new_star, new_plain;
};

/* globals */
int test_complete;		/* counts number of tests completed */
int test_count;			/* counts how many times end_test() was
				   called */
long start_time;		/* start of test (in seconds) */
long end_time;			/* end of test (in seconds) */
int capper;			/* points into cap_list[] for time tests */
struct cap_tab cap_list[CAP_MAX];	/* holds time test results */
char *ced_name[EDIT_MAX];	/* cap name used for pad edit */
char **ced_value[EDIT_MAX];	/* cap value used for pad edit */
int milli_pad;			/* accumulated pad in 10 microsecond units */
int milli_reps;			/* multiplicative pad in 10 microsecond units */
int pad_sent;			/* number of characters for pad */

char txt_longer_test_time[80];	/* >) use longer time */
char txt_shorter_test_time[80];	/* <) use shorter time */
int pad_test_duration = 1;	/* number of seconds for a pad test */
int no_alarm_event;		/* TRUE if the alarm has not gone off yet */
int usec_run_time;		/* length of last test in microseconds */
struct timeval test_start_time;	/* Time of day when the test started */
struct timezone zone;		/* Time zone at start of test */

/* caps under test data base */
char *tt_cap[TT_MAX];		/* value of string */
int tt_affected[TT_MAX];	/* lines or columns effected (repitition factor) */
int tt_count[TT_MAX];		/* Number of times sent */
int ttp;			/* number of entries used */

/* Saved valure of the above data base */
char *tx_cap[TT_MAX];		/* value of string */
int tx_affected[TT_MAX];	/* lines or columns effected (repitition factor) */
int tx_count[TT_MAX];		/* Number of times sent */
int tx_index[TT_MAX];		/* String index */
int txp;			/* number of entries used */
int tx_characters;		/* printing characters sent by test */
int tx_cps;			/* characters per second */

extern char *malloc();
extern char letters[];
extern int raw_characters_sent;	/* Total output characters */
extern struct test_menu pad_menu;	/* Pad menu structure */
extern struct test_list pad_test_list[];

#ifdef ACCO
extern FILE *fpacco;

#endif

/*****************************************************************************
 *
 * Execution control for capability tests
 *
 *****************************************************************************/

void
control_init(void)
{
	sprintf(txt_longer_test_time, ">) Change test time to %d seconds",
		pad_test_duration << 1);
	sprintf(txt_shorter_test_time, "<) Change test time to %d seconds",
		pad_test_duration >> 1);
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
#ifdef NOT_USED
int
enter_cap(char *n, char *v)
{				/* enter a cap into the cap_list (for time
				   tests) */
	int i;

	/* find an empty slot */
	for (i = 0; cap_list[i].value; ++i) {
		if (i >= CAP_MAX)
			return -1;
		if (!strcmp(cap_list[i].value, v)) {
			if (cap_list[i].name && n &&
				strcmp(cap_list[i].name, n))
				continue;
			if (!cap_list[i].name)
				cap_list[i].name = n;
			return i;
		}
	}
	cap_list[i].value = v;
	cap_list[i].name = n;
	cap_list[i].new_star = cap_list[i].new_plain = 32767;
	pad_time(v, &cap_list[i].cur_star, &cap_list[i].cur_plain);
	return i;
}


int
multi_test(char *error_name,
	int clr, int or_bits, int and_bits, int acco_bits, int num_caps,
	char *cn1, char **cv1,
	char *cn2, char **cv2,
	char *cn3, char **cv3,
	char *cn4, char **cv4)
{				/* clear the screen and start the test
				   (general case) */
	int ch, i, or_flag, and_flag, select_flag;
	static char name_buffer[128];

	ced_name[0] = cn1;
	ced_name[1] = cn2;
	ced_name[2] = cn3;
	ced_name[3] = cn4;
	ced_value[0] = cv1;
	ced_value[1] = cv2;
	ced_value[2] = cv3;
	ced_value[3] = cv4;
	for (i = 0; i < num_caps; i++) {
		can_test(ced_name[i], FLAG_CAN_TEST);
	}
	test_count = milli_pad = milli_reps = 0;
	or_flag = or_bits & 1;
	and_flag = and_bits & 1;
	select_flag = !cap_select;
	name_buffer[0] = '\0';
	ch = 0;
#ifdef ACCO
	fprintf(fpacco, "(");
	if (or_flag)
		fprintf(fpacco, "TRUE");
	else
		for (i = 0; i < num_caps; i++) {
			if ((or_bits >> i) & 2) {
				fprintf(fpacco, "%s", ced_name[i]);
				if ((or_bits >> i) & -4)
					fprintf(fpacco, "|");
			}
		}
	fprintf(fpacco, ")|(");
	if (and_flag)
		for (i = 0; i < num_caps; i++) {
			if ((and_bits >> i) & 2) {
				fprintf(fpacco, "%s", ced_name[i]);
				if ((and_bits >> i) & -4)
					fprintf(fpacco, "&");
			}
		}
	else
		fprintf(fpacco, "FALSE");
	fprintf(fpacco, ") ACCO(");
	for (i = 0; i < num_caps; i++) {
		if ((acco_bits >> i) & 2) {
			fprintf(fpacco, "%s", ced_name[i]);
			if ((acco_bits >> i) & -4)
				fprintf(fpacco, "+");
		}
	}
	fprintf(fpacco, ") %s\n", error_name ? error_name : "");
#endif
	for (i = 0; i < num_caps; i++) {
		if ((or_bits >> i) & 2)
			or_flag |= *ced_value[i] != NULL;
		if ((and_bits >> i) & 2)
			and_flag &= *ced_value[i] != NULL;
		if (*ced_value[i] && ((acco_bits >> i) & 2)) {
			++ch;
			capper = enter_cap(ced_name[i], *ced_value[i]);
			(void) acco_pad(ced_name[i], *ced_value[i]);
		}
		if (cap_select)
			select_flag |= strcmp(cap_select, ced_name[i]) == 0;
		sprintf(&name_buffer[strlen(name_buffer)], "(%s) ", ced_name[i]);
	}
	if (!select_flag)
		return FALSE;
	if (or_flag | and_flag) {
		for (i = num_caps; i < EDIT_MAX; ced_name[i++] = NULL)
			continue;
		sprintf(done_test, "%sDone: ", current_test = name_buffer);
		letter = letters[letter_number = 0];
		put_clear();
		clear_select = -1;
		char_max = full_test;
		reps = 0;
		if (ch != 1)
			capper = -1;
		if (time_pad)
			return enq_ack();
		put_str("Begin ");
		ptext(name_buffer);
		ptext("pad test, hit CR to continue, n to skip test: ");
		return begin_pad_char(clr);
	}
	ptext(error_name);
	if (!time_pad) {
		ptext(", not present: ");
		ch = wait_here();
		if (ch == 'c' || ch == 'C')
			put_clear();
	} else
		ptextln(", not present");
	return FALSE;
}


static int
time_test_state(int clr)
{				/* calculate and report the suggested pad
				   times */
	long v;

	/*
	   time is one of the things that UNIX does poorly. So I fuss around
	   with sloppy numbers.
	*/
	v = end_time - start_time -
		((char_sent >> 1) * tty_frame_size) / tty_baud_rate;
	if (capper == -2) {	/* truely brain damaged terminals come here */
		v = ((char_sent >> 1) * tty_frame_size)
			/ (end_time - start_time);
		sprintf(temp, "This terminal has a maximum effective baud rate of %ld. ", v);
		ptext(temp);
		ptext(" System load may be to high. ");
		ptextln(" The results of this test will be incorrect!");
		return 2;
	} else {
		if (capper == -1)
			put_str("combined");
		else {
			sprintf(temp, "(%s)", cap_list[capper].name);
			ptext(temp);
		}
		put_dec(" pad time should be %d.%d milliseconds", v);

		if (reps) {
			sprintf(temp, " for %d reps.", reps);
			ptextln(temp);
			put_dec("%d.%d* milliseconds per rep", v / reps);
		}
		put_crlf();

		if (capper >= 0) {
			if (milli_pad < cap_list[capper].new_plain)
				cap_list[capper].new_plain = v;
			if (reps != 0 &&
				v / reps < cap_list[capper].new_star)
				cap_list[capper].new_star = v / reps;
			/*
			   if the terminal makes it here once then we want to
			   report each time. This forces a smaller number
			   into the tables.
			*/
		}
		if (milli_reps != 0 && reps != 0) {
			ptext("current value is ");
			if (milli_pad)
				put_dec("%d.%d milliseconds and ",
					milli_pad);
			put_dec("%d.%d* milliseconds per rep",
				milli_reps);
		} else
			put_dec("current value is %d.%d milliseconds", milli_pad);
		put_crlf();
		if (clear_select != -1) {
			clr_test_value[clear_select] = v;
			clr_test_reps[clear_select] = reps;
		}
	}
	return 0;
}

/*
 * This code is executed at the bottom of each test to print the "Done"
 * message and control the looping of tests.
 */

int
repeat_pad_test(char *txt, char *cap, int clr)
{
	int ch;
	static char *help[] = {
		"?  -  this help message\n"
		"c  -  continue",
		"d  -  double the pad multiplier",
		"h  -  halve the pad multiplier",
		"i  -  send the reset and init strings",
		"p  -  change the padding for this capability",
		"q  -  quit (prompts for verification)",
		"?  -  print this help message",
		"<  -  decrease the time for each test",
		">  -  increase the time for each test",
		"<number> - set the pad multiplier",
		"y  -  record success and go on to next test",
		"n  -  record failure and go on to next test",
		"<CR> -  record nothing, go on to next test",
	0};

	test_count++;
	if (char_sent < char_max)
		return TRUE;
	if (txt)
		ptext(txt);
	if (xon_xoff && tty_can_sync && tty_sync_error(0) && test_length >= 10) {
		end_time = time(0);
		switch (time_test_state(clr)) {
		case 0:
			if (time_pad)
				return FALSE;
			break;
		case 1:
			if (time_pad)
				return TRUE;
			break;
		case 2:
			break;
		}
	}

	/* process the character(s) at the end of a test */
	/* return FALSE if end of loop */
	set_attr(0);	/* just in case */
	for (;;) {
		ptext(done_test);
		ch = wait_here();
		switch (ch) {
		case '?':
			put_clear();
			for (ch = 0; help[ch]; ch++) {
				ptextln(help[ch]);
			}
			sprintf(temp, "Tests will affect %d lines (or characters).",
				augment);
			ptextln(temp);
			sprintf(temp, "Tests will run %d seconds.", test_length);
			ptextln(temp);
			if (debug_level > 0) {
				sprintf(temp, "cpms = %f %d, pad_sent=%d, milli_pad=%d ",
					(float) tty_cpms / (float) (1 << CPMS_SHIFT), tty_cpms,
					pad_sent, milli_pad);
				ptextln(temp);
				sprintf(temp, "OUTPUT TRANS %d, NOECHO %d, CHAR MODE %d",
					stty_query(TTY_OUT_TRANS) != 0,
					stty_query(TTY_NOECHO) != 0,
					stty_query(TTY_CHAR_MODE) != 0);
				ptextln(temp);
			}
			/* in case the "Done" gets eaten */
			ptext(done_test);
			break;
		case '>':
		case '<':
			if (ch == '>')
				test_length *= 2;
			else
				test_length /= 2;
			control_init();
			sprintf(temp, "\nPad test will run %d seconds.",
				test_length);
			ptext(temp);
			break;
		case 'C':
			put_clear();
		case 'c':
			/* continue */
			break;
		case 'd':
		case 'D':
			/* double the test augment */
			augment *= 2;
			sprintf(temp, "\nPad test will affect %d lines (or characters).",
				augment);
			ptext(temp);
			break;
		case 'h':
		case 'H':
			/* cut the test augment in half */
			augment /= 2;
			sprintf(temp, "\nPad test will affect %d lines (or characters).",
				augment);
			ptext(temp);
			break;
		case 'i':
		case 'I':
			/* send the reset and init strings */
			reset_init(TRUE);
			tc_putp(exit_insert_mode);
			replace_mode = 1;
			put_mode(exit_attribute_mode);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			/* read a new test augment */
			augment = 0;
			while (ch >= '0' && ch <= '9') {
				if (stty_query(TTY_NOECHO))
					tc_putch(ch);
				augment = ch - '0' + augment * 10;
				ch = getchp(STRIP_PARITY);
			}
			sprintf(temp, "\nPad test will affect %d lines (or characters).",
				augment);
			ptext(temp);
			break;
		case 'q':
		case 'Q':
			ptext("Enter Y to quit:");
			ch = wait_here();
			}
			break;
		case 'r':
		case 'R':
			if (augment < 1)
				augment = 1;
			char_max = full_test;
			put_clear();
			char_sent = 0;
			if (time_pad)
				return enq_ack();
			start_time = time(0);
			return TRUE;
		case 'n':
		case 'N':
			log_status(cap, FALSE);
			goto next;
		case 'y':
		case 'Y':
			log_status(cap, TRUE);
		case '\r':
		case '\n':
	next:
		default:
			if (clr)
				put_clear();
			return FALSE;
		}
	}
}
#endif


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
	while(1) {
		if (text) {
			ptext(text);
		}
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
		if (*ch == '\r' || *ch == '\n' || *ch == 'n') {
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
	char done_message[128];

	while (1) {
		if (test->caps_done) {
			sprintf(done_message, "(%s) Done ", test->caps_done);
			ptext(done_message);
		} else {
			ptext("Done ");
		}
		*ch = wait_here();
		if (*ch == '\r' || *ch == '\n' || *ch == 's' || *ch == 'n') {
			*ch = 0;
			return;
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
	int crlf)
{
	struct timeval test_stop_time;

	flush_input();
	(void) gettimeofday(&test_stop_time, &zone);
	tx_characters = raw_characters_sent;
	usec_run_time =
		((test_stop_time.tv_sec - test_start_time.tv_sec) * 1000000)
		+ test_stop_time.tv_usec - test_start_time.tv_usec;
	tx_cps = sliding_scale(tx_characters, 1000000, usec_run_time);

	/* save the data base */
	for (txp = 0; txp < ttp; txp++) {
		tx_cap[txp] = tt_cap[txp];
		tx_count[txp] = tt_count[txp];
		tx_affected[txp] = tt_affected[txp];
	}

	if (crlf) {
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

	put_crlf();
	if (pad_menu.resume_tests && pad_menu.resume_tests->caps_done) {
		sprintf(temp, "Caps summary for (%s)", pad_menu.resume_tests->caps_done);
		ptextln(temp);
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
	pad_test_duration <<= 1;
	sprintf(txt_longer_test_time, ">) Change test time to %d seconds",
		pad_test_duration << 1);
	sprintf(txt_shorter_test_time, "<) Change test time to %d seconds",
		pad_test_duration >> 1);
	sprintf(temp, "Tests will run for %d seconds", pad_test_duration);
	ptext(temp);
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
		pad_test_duration >>= 1;
		sprintf(txt_longer_test_time, ">) Change test time to %d seconds",
			pad_test_duration << 1);
		sprintf(txt_shorter_test_time, "<) Change test time to %d seconds",
			pad_test_duration >> 1);
	}
	sprintf(temp, "Tests will run for %d second%s", pad_test_duration,
		pad_test_duration > 1 ? "s" : "");
	ptext(temp);
}
