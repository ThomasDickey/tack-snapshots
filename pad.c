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
#include <string.h>
#include "tac.h"

/* test the pad counts on the terminal */

static void pad_standard(struct test_list *, int *, int *);
static void init_xon_xoff(struct test_list *, int *, int *);
static void init_cup(struct test_list *, int *, int *);
static void pad_rmxon(struct test_list *, int *, int *);
static void pad_home1(struct test_list *, int *, int *);
static void pad_home2(struct test_list *, int *, int *);
static void pad_clear(struct test_list *, int *, int *);
static void pad_ech(struct test_list *, int *, int *);
static void pad_el1(struct test_list *, int *, int *);
static void pad_el(struct test_list *, int *, int *);
static void pad_smdc(struct test_list *, int *, int *);
static void pad_dch(struct test_list *, int *, int *);
static void pad_dch1(struct test_list *, int *, int *);
static void pad_smir(struct test_list *, int *, int *);
static void pad_ich(struct test_list *, int *, int *);
static void pad_ich1(struct test_list *, int *, int *);
static void pad_xch1(struct test_list *, int *, int *);
static void pad_rep(struct test_list *, int *, int *);
static void pad_cup(struct test_list *, int *, int *);
static void pad_hd(struct test_list *, int *, int *);
static void pad_hu(struct test_list *, int *, int *);
static void pad_rin(struct test_list *, int *, int *);
static void pad_il(struct test_list *, int *, int *);
static void pad_indn(struct test_list *, int *, int *);
static void pad_dl(struct test_list *, int *, int *);
static void pad_xl(struct test_list *, int *, int *);
static void pad_scrc(struct test_list *, int *, int *);
static void pad_csrind(struct test_list *, int *, int *);
static void pad_sccsrrc(struct test_list *, int *, int *);
static void pad_csr_nel(struct test_list *, int *, int *);
static void pad_csr_cup(struct test_list *, int *, int *);
static void pad_ht(struct test_list *, int *, int *);
static void pad_smso(struct test_list *, int *, int *);
static void pad_smacs(struct test_list *, int *, int *);
static void pad_crash(struct test_list *, int *, int *);

extern struct test_menu change_pad_menu;

struct test_list pad_test_list[] = {
	{0, 0, 0, 0, "e) edit terminfo", 0, &edit_menu},
	{0, 0, 0, 0, "p) change padding", 0, &change_pad_menu},
	{0, 0, 0, 0, "@) Display statistics about the last test", dump_test_stats, 0},
	{0, 0, 0, 0, "c) Clear screen", menu_clear_screen, 0},
	{0, 0, 0, 0, "i) Send reset and init", menu_reset_init, 0},
	{0, 0, 0, 0, txt_longer_test_time, longer_test_time, 0},
	{0, 0, 0, 0, txt_shorter_test_time, shorter_test_time, 0},
	/***
	   Phase 1: Test initialization and reset strings.
	
	   (rs1) (rs2) (rs3) (is1) (is2) (is3) are very difficult to test.
	   They have no defined output.  To make matters worse, the cap
	   builder could partition (rs1) (rs2) (rs3) by length, leaving the
	   terminal in some unknown state between (rs1) and (rs2) or between
	   (r2) and (rs3).  Some reset strings clear the screen when done.
	
	   We have no control over this.  The only thing we can do for
	   certain is to test the pad times by checking for overruns.
	***/
	{MENU_NEXT, 3, "rs1", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "rs2", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "rs3", 0, 0, pad_standard, 0},
	{MENU_NEXT | MENU_INIT, 0, 0, 0, 0, init_xon_xoff, 0},
	{MENU_NEXT, 3, "is1", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "is2", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "is3", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "rmxon", "smxon", 0, pad_rmxon, 0},
	{MENU_NEXT | MENU_INIT, 0, 0, 0, 0, init_cup, 0},
	/*
	   Phase 2: Test home, screen clears and erases.
	*/
	{MENU_NEXT, 0, "home", 0, 0, pad_home1, 0},
	{MENU_NEXT, 0, "home) (nel", 0, 0, pad_home2, 0},
	{MENU_NEXT + 1, 0, "clear", 0, 0, pad_clear, 0},
	{MENU_NEXT, 0, "ed", 0, 0, pad_clear, 0},
	{MENU_NEXT, 0, "ech", 0, 0, pad_ech, 0},
	{MENU_NEXT, 0, "el1", "cub1 nel", 0, pad_el1, 0},
	{MENU_NEXT, 0, "el", "nel", 0, pad_el, 0},
	/*
	   Phase 3: Character deletions and insertions
	*/
	{MENU_NEXT, 0, "smdc) (rmdc", 0, 0, pad_smdc, 0},
	{MENU_NEXT, 0, "dch", "smdc rmdc", 0, pad_dch, 0},
	{MENU_NEXT, 0, "dch1", "smdc rmdc", 0, pad_dch1, 0},
	{MENU_NEXT, 0, "smir) (rmir", 0, 0, pad_smir, 0},
	{MENU_NEXT, 0, "ich) (ip", "smir rmir", 0, pad_ich, 0},
	{MENU_NEXT, 0, "ich1) (ip", "smir rmir", 0, pad_ich1, 0},
	{MENU_NEXT, 4, "ich1) (dch1", "smir rmir", 0, pad_xch1, 0},
	{MENU_NEXT, 0, "rep", 0, 0, pad_rep, 0},
	/*
	   Phase 4: Test cursor addressing pads.
	*/
	{MENU_NEXT, 0, "cup", 0, 0, pad_cup, 0},
	/*
	   Phase 5: Test scrolling and cursor save/restore.
	*/
	{MENU_NEXT, 0, "hd", 0, 0, pad_hd, 0},
	{MENU_NEXT, 0, "hu", 0, 0, pad_hu, 0},
	{MENU_NEXT + 1, 0, "rin", 0, 0, pad_rin, 0},
	{MENU_NEXT, 0, "ri", 0, 0, pad_rin, 0},
	{MENU_NEXT + 1, 0, "il", 0, 0, pad_il, 0},
	{MENU_NEXT, 0, "il1", 0, 0, pad_il, 0},
	{MENU_NEXT + 1, 0, "indn", 0, 0, pad_indn, 0},
	{MENU_NEXT, 0, "ind", 0, 0, pad_indn, 0},
	{MENU_NEXT + 1, 0, "dl", 0, 0, pad_dl, 0},
	{MENU_NEXT, 0, "dl1", 0, 0, pad_dl, 0},
	{MENU_NEXT, 0, "il1) (dl1", 0, 0, pad_xl, 0},
	{MENU_NEXT, 0, "sc) (rc", 0, 0, pad_scrc, 0},
	{MENU_NEXT, 0, "csr) (ind", 0, 0, pad_csrind, 0},
	{MENU_NEXT, 0, "sc) (csr) (rc", 0, 0, pad_sccsrrc, 0},
	{MENU_NEXT, 0, "csr) (nel", "sc rc", 0, pad_csr_nel, 0},
	{MENU_NEXT, 0, "csr) (cup", 0, 0, pad_csr_cup, 0},
	/*
	   Phase 6: Test tabs.
	*/
	{MENU_NEXT, 0, "ht", 0, 0, pad_ht, 0},
	/*
	   Phase 7: Test character-set-switch pads.
	*/
	{MENU_NEXT, 0, "smso) (rmso", 0, 0, pad_smso, 0},
	{MENU_NEXT, 0, "smacs) (rmacs", 0, 0, pad_smacs, 0},
	/*
	   Phase 8: Tests for miscellaneous mode-switch pads.
	*/
	{MENU_NEXT, 3, "flash", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "smkx", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "rmkx", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "smm", 0, 0, pad_standard, 0},
	{MENU_NEXT, 3, "rmm", 0, 0, pad_standard, 0},
	/*
	   Phase 9: Test crash-and-burn properties of unpadded (clear).
	*/
	{0, 0, "clear", "xon", "k) run clear test with no padding", pad_crash, 0},
	{MENU_LAST, 0, 0, 0, 0, 0, 0}
};

extern int test_complete;	/* counts number of tests completed */

#define SLOW_TERMINAL_EXIT if (!test_complete && !no_alarm_event) { break; }

/* globals */
int hzcc;			/* horizontal character count */
char letter;			/* current character being displayed */
int letter_number;		/* points into letters[] */
int augment, reps;		/* number of characters (or lines) effected */
int clear_select;		/* points into clr_test_value[] */
int clr_test_value[CLEAR_TEST_MAX];
int clr_test_reps[CLEAR_TEST_MAX];
char letters[] = "AbCdefghiJklmNopQrStuVwXyZ";

static char *clr_test_name[CLEAR_TEST_MAX] = {
	"full page", "sparse page", "short lines", "one full line",
"one short line"};

static char every_line[] = "This text should be on every line.";
static char all_lines[] = "Each char on any line should be the same.  ";
static char above_line[] = "The above lines should be all Xs.  ";
static char no_visual[] = "This loop test has no visual failure indicator.  ";

/*
**	pad_standard(test_list, status, ch)
**
**	Run a single cap pad test.
*/
static void
pad_standard(
	struct test_list *t,
	int *state,
	int *ch)
{
	char *long_name;
	char *cap;
	int full_page = 0;
	char tbuf[128];

	if ((cap = get_string_cap_byname(t->caps_done, &long_name))) {
		sprintf(tbuf, "(%s) %s, start testing [n] > ", t->caps_done,
			long_name);
		if (skip_pad_test(t, state, ch, tbuf)) {
			return;
		}
		pad_test_startup(1);
		do {
			if (char_count >= columns) {
				page_loop();
				full_page = TRUE;
			}
			tt_putp(cap);
			putchp(letter);
		} while(no_alarm_event);
		pad_test_shutdown(0);
		if (full_page) {
			home_down();
		} else {
			put_crlf();
		}
		ptextln(no_visual);
	} else {
		/* Note: get_string_cap_byname() always sets long_name */
		sprintf(temp, "(%s) %s, not present.  ", t->caps_done,
			long_name);
		ptext(temp);
	}
	pad_done_message(t, state, ch);
}

/*
**	init_xon_xoff(test_list, status, ch)
**
**	Initialize the xon_xoff values
*/
static void
init_xon_xoff(
	struct test_list *t,
	int *state,
	int *ch)
{
	/* the reset strings may dink with the XON/XOFF modes */
	if (select_xon_xoff == 0 && exit_xon_mode) {
		tc_putp(exit_xon_mode);
	}
	if (select_xon_xoff == 1 && enter_xon_mode) {
		tc_putp(enter_xon_mode);
	}
}

/*
**	pad_rmxon(test_list, status, ch)
**
**	Test (rmxon) exit XON/XOFF mode
*/
static void
pad_rmxon(
	struct test_list *t,
	int *state,
	int *ch)
{
	if (select_xon_xoff == 0 && exit_xon_mode) {
		pad_standard(t, state, ch);
	}
}

/*
**	init_cup(test_list, status, ch)
**
**	Send the initialization strings for XON/XOFF and (smcup)
**	Stop pad testing if clear screen is missing.
*/
static void
init_cup(
	struct test_list *t,
	int *state,
	int *ch)
{
	hzcc = columns * 8 / 10;	/* horizontal character count */
	init_xon_xoff(t, state, ch);
	if (enter_ca_mode) {
		tc_putp(enter_ca_mode);
	}
	if (!can_clear_screen) {
		ptext("(clear) clear screen not present,");
		ptext(" pad processing terminated.  ");
		pad_done_message(t, state, ch);
		if (*ch == 0 || *ch == 'n' || *ch == 's' || *ch == 'r') {
			*ch = '?';
		}
		return;
	}
}

/*
**	pad_home1(test_list, status, ch)
**
**	Test (home) when (am) is set.
*/
static void
pad_home1(
	struct test_list *t,
	int *state,
	int *ch)
{
	int j, k;

	if (can_go_home && auto_right_margin) {
		/*
		   truly brain damaged terminals will fail this test because
		   they cannot accept data at full rate
		*/
		if (skip_pad_test(t, state, ch, "(home) Home start testing [n] > ")) {
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
		pad_test_shutdown(1);
		ptext("All the dots should line up.  ");
		pad_done_message(t, state, ch);
		put_clear();
	}
}

/*
**	pad_home2(test_list, status, ch)
**
**	Test (home) and (nel).  (am) is reset.
*/
static void
pad_home2(
	struct test_list *t,
	int *state,
	int *ch)
{
	int j, k;

	if (can_go_home) {
		if (skip_pad_test(t, state, ch,
			"(home) Home, (nel) newline start testing [n] > ")) {
			return;
		}
		pad_test_startup(1);
		for ( ; no_alarm_event; test_complete++) {
			go_home();
			for (j = 1; j < lines; j++) {
				for (k = 2; k < columns; k++) {
					if (k & 0xF) {
						put_this(letter);
					} else {
						put_this('.');
					}
				}
				put_crlf();	/* this does the (nel) */
				SLOW_TERMINAL_EXIT;
			}
			NEXT_LETTER;
		}
		pad_test_shutdown(0);
		ptext("All the dots should line up.  ");
		pad_done_message(t, state, ch);
		put_clear();
	}
}

/*
**	pad_clear(test_list, status, ch)
**
**	Test (clear) and (ed)
**	run the clear screen tests (also clear-to-end-of-screen)
*/
static void
pad_clear(
	struct test_list *t,
	int *state,
	int *ch)
{
	char *end_message = (char *) NULL, *txt;
	int j, k, is_clear;

	is_clear = t->flags & 1;
	for (j = 0; j < CLEAR_TEST_MAX; j++) {
		clr_test_value[j] = 32767;
	}
	clear_select = auto_right_margin ? 0 : 1;
	augment = is_clear ? lines : lines - 1;
	if (is_clear) {
		txt = "(clear) clear-screen start testing [n] > ";
	} else {
		txt = "(ed) erase-to-end-of-display start testing [n] > ";
	}
	if (skip_pad_test(t, state, ch, txt)) {
		return;
	}
	if (enter_am_mode) {
		tc_putp(enter_am_mode);
		clear_select = 0;
	}
	for (; clear_select < CLEAR_TEST_MAX; clear_select++) {
		if (augment > lines || is_clear || !cursor_address) {
			augment = lines;
		} else {
			if (augment <= 1)
				augment = 2;
			if (augment < lines) {
				put_clear();
				ptextln("This line should not be erased (ed)");
			}
		}
		reps = augment;
		switch (clear_select) {
		case 0:
			end_message = "Clear full screen.  ";
			break;
		case 1:
			end_message = "Clear sparse screen.  ";
			if (cursor_down)
				break;
			clear_select++;
		case 2:
			end_message = "Clear one character per line.  ";
			if (newline)
				break;
			clear_select++;
		case 3:
			end_message = "Clear one full line.  ";
			break;
		case 4:
			end_message = "Clear single short line.  ";
			break;
		}
		pad_test_startup(0);
		for ( ; no_alarm_event; test_complete++) {
			switch (clear_select) {
			case 0:	/* full screen test */
				for (j = 1; j < reps; j++) {
					for (k = 0; k < columns; k++) {
						if (k & 0xF) {
							put_this(letter);
						} else {
							put_this('.');
						}
					}
					SLOW_TERMINAL_EXIT;
				}
				break;
			case 1:	/* sparse screen test */
				for (j = columns - reps; j > 2; j--) {
					put_this(letter);
				}
				for (j = 2; j < reps; j++) {
					tt_putp(cursor_down);
					put_this(letter);
				}
				break;
			case 2:	/* short lines */
				for (j = 2; j < reps; j++) {
					put_this(letter);
					tt_putp(newline);
				}
				put_this(letter);
				break;
			case 3:	/* one full line */
				for (j = columns - 5; j > 1; j--) {
					put_this(letter);
				}
				break;
			case 4:	/* one short line */
				put_str("Erase this!");
				break;
			}
			if (is_clear) {
				put_clear();
			} else {
				if (augment == lines) {
					go_home();
				} else {
					tt_putparm(cursor_address,
						lines - reps,
						lines - reps, 0);
				}
				tt_tputs(clr_eos, reps);
			}
			NEXT_LETTER;
		}
		pad_test_shutdown(1);
		ptext(end_message);

		pad_done_message(t, state, ch);

		if (*ch != 0 && *ch != 'n') {
			return;
		}

		if (clear_select < CLEAR_TEST_MAX && time_pad)
			enq_ack();
	}
	k = 0;
	for (j = 0; j < CLEAR_TEST_MAX; j++) {
		if (clr_test_value[j] != 32767) {
			k++;
			sprintf(temp, "(%s) %s", "help me", clr_test_name[j]);
			ptext(temp);
			put_dec(" %d.%d milliseconds", clr_test_value[j]);
			sprintf(temp, " %d reps", clr_test_reps[j]);
			ptext(temp);
			put_crlf();
		}
	}
	if (k) {
		pad_done_message(t, state, ch);
	}
}

/*
**	pad_ech(test_list, status, ch)
**
**	Test (ech) erase characters
*/
static void
pad_ech(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!erase_chars) {
		ptext("(ech) Erase-characters, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = hzcc;
	if (skip_pad_test(t, state, ch,
		"(ech) Erase-characters start testing [n] > ")) {
		return;
	}
	if (augment > columns - 2) {
		augment = columns - 2;
	}
	reps = augment;
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		for (i = 2; i < lines; i++) {
			for (j = 0; j <= reps; j++) {
				putchp(letter);
			}
			put_cr();
			tt_putparm(erase_chars, reps, reps, 0);
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		for (i = 1; i <= reps; i++) {
			putchp(' ');
		}
		putchp(letter);
		put_crlf();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	ptext(all_lines);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_el1(test_list, status, ch)
**
**	Test (el1) erase to start of line also (cub1) and (nel)
*/
static void
pad_el1(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!clr_bol) {
		ptext("(el1) Erase-to-beginning-of-line, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = hzcc;
	if (skip_pad_test(t, state, ch,
		"(el1) Erase-to-beginning-of-line start testing [n] > ")) {
		return;
	}
	if (augment > columns - 2) {
		augment = columns - 2;
	}
	reps = augment;
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		if (augment > columns - 2) {
			augment = columns - 2;
		}
		reps = augment;
		for (i = 2; i < lines; i++) {
			for (j = 0; j <= reps; j++) {
				putchp(letter);
			}
			tt_putp(cursor_left);
			tt_putp(cursor_left);
			tt_tputs(clr_bol, reps);
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		for (i = 1; i <= reps; i++) {
			putchp(' ');
		}
		putchp(letter);
		put_crlf();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	ptext(all_lines);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_el(test_list, status, ch)
**
**	Test (el) clear to end of line also (nel)
*/
static void
pad_el(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!clr_eol) {
		ptext("(el) Clear-to-end-of-line, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = hzcc / 10;
	if (skip_pad_test(t, state, ch,
		"(el) Clear-to-end-of-line start testing [n] > ")) {
		return;
	}
	if (augment > hzcc) {
		augment = hzcc;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		for (i = 2; i < lines; i++) {
			for (j = -1; j < augment; j++) {
				putchp(letter);
			}
			put_cr();
			putchp(letter);
			tt_putp(clr_eol);
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		putchp(letter);
		put_crlf();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	ptext(all_lines);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_smdc(test_list, status, ch)
**
**	Test (smdc) (rmdc) Delete mode
*/
static void
pad_smdc(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;

	if (!enter_delete_mode) {
		ptext("(smdc) Enter-delete-mode");
		if (!exit_delete_mode) {
			ptext(", (rmdc) Exit-delete-mode");
		}
		ptext(", not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(smdc) (rmdc) Enter/Exit-delete-mode start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		page_loop();
		for (i = 1; i < columns; i++) {
			tt_putp(enter_delete_mode);
			tt_putp(exit_delete_mode);
			putchp(letter);
		}
	}
	pad_test_shutdown(0);
	home_down();
	ptext(no_visual);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_dch(test_list, status, ch)
**
**	Test (smdc) (rmdc) Delete mode and (dch)
*/
static void
pad_dch(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!parm_dch) {
		ptext("(dch) Delete-characters, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = hzcc;
	if (skip_pad_test(t, state, ch,
		"(dch) Delete-characters start testing [n] > ")) {
		return;
	}
	if (augment > hzcc) {
		augment = hzcc;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		reps = augment;
		go_home();
		for (i = 2; i < lines; i++) {
			for (j = 0; j <= reps; j++) {
				putchp(letter);
			}
			put_cr();
			tt_putp(enter_delete_mode);
			tt_putparm(parm_dch, reps, reps, 0);
			tt_putp(exit_delete_mode);
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		putchp(letter);
		put_crlf();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	home_down();
	ptext(all_lines);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_dch1(test_list, status, ch)
**
**	Test (smdc) (rmdc) Delete mode and (dch1)
*/
static void
pad_dch1(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!delete_character) {
		if (parm_dch) {
			/* if the other one is defined then its OK */
			return;
		}
		ptext("(dch1) Delete-character, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = hzcc;
	if (skip_pad_test(t, state, ch,
		"(dch1) Delete-character start testing [n] > ")) {
		return;
	}
	if (augment > hzcc) {
		augment = hzcc;
	}
	if (augment < 1) {
		augment = 1;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		for (i = 2; i < lines; i++) {
			for (j = -1; j < augment; j++) {
				putchp(letter);
			}
			put_cr();
			tt_putp(enter_delete_mode);
			for (j = 0; j < augment; j++) {
				tt_putp(delete_character);
			}
			tt_putp(exit_delete_mode);
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		putchp(letter);
		put_crlf();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	ptext(all_lines);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_smir(test_list, status, ch)
**
**	Test (smir) (rmir) Insert mode
*/
static void
pad_smir(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;

	if (!enter_insert_mode) {
		ptext("(smir) Enter-insert-mode");
		if (!exit_insert_mode) {
			ptext(", (rmir) Exit-insert-mode");
		}
		ptext(", not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(smir) (rmir) Enter/Exit-insert-mode start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		page_loop();
		for (i = 1; i < columns; i++) {
			tt_putp(enter_insert_mode);
			tt_putp(exit_insert_mode);
			putchp(letter);
		}
	}
	pad_test_shutdown(0);
	home_down();
	ptext(no_visual);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_ich(test_list, status, ch)
**
**	Test (smir) (rmir) Insert mode and (ich) and (ip)
*/
static void
pad_ich(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!parm_ich) {
		ptext("(ich) Insert-characters, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = j = columns * 9 / 10;
	if (skip_pad_test(t, state, ch,
		"(ich) Insert-characters, (ip) Insert-padding start testing [n] > ")) {
		return;
	}
	if (augment > j) {
		augment = j;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		for (i = 2; i < lines; i++) {
			putchp(letter);
			put_cr();
			tt_putp(enter_insert_mode);
			replace_mode = 0;
			tt_putparm(parm_ich, reps, reps, 0);
			tt_putp(exit_insert_mode);
			replace_mode = 1;
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		for (i = 0; i < reps; i++) {
			putchp(' ');
		}
		putchp(letter);
		NEXT_LETTER;
		put_crlf();
	}
	pad_test_shutdown(0);
	ptext(all_lines);
	pad_done_message(t, state, ch);
	tc_putp(exit_insert_mode);
}

/*
**	pad_ich1(test_list, status, ch)
**
**	Test (smir) (rmir) Insert mode and (ich1) and (ip)
*/
static void
pad_ich1(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j, l;

	if (!insert_character) {
		ptext("(ich1) Insert-character, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(ich1) Insert-character, (ip) Insert-padding start testing [n] > ")) {
		return;
	}
	l = columns * 9 / 10;
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		put_clear();
		for (i = 2; i < lines; i++) {
			putchp(letter);
			put_cr();
			tt_putp(enter_insert_mode);
			replace_mode = 0;
			if (!insert_padding && !insert_character) {
				/* only enter/exit is needed */
				for (j = 0; j < l; j++) {
					putchp('.');
				}
			} else {
				for (j = 0; j < l; j++) {
					tt_putp(insert_character);
					putchp('.');
					tt_putp(insert_padding);
				}
			}
			tt_putp(exit_insert_mode);
			replace_mode = 1;
			put_crlf();
			SLOW_TERMINAL_EXIT;
		}
		for (j = 0; j < l; j++) {
			putchp('.');
		}
		putchp(letter);
		NEXT_LETTER;
		put_crlf();
	}
	pad_test_shutdown(0);
	ptext(all_lines);
	pad_done_message(t, state, ch);
	tc_putp(exit_insert_mode);
}

/*
**	pad_xch1(test_list, status, ch)
**
**	Test (ich1) (ip) (dch1)
*/
static void
pad_xch1(
	struct test_list *t,
	int *state,
	int *ch)
{
	static char xch1[] =
	"This line should not be garbled. It should be left justified.";

	if (enter_insert_mode || exit_insert_mode ||
		enter_delete_mode || exit_delete_mode ||
		!insert_character || !delete_character) {
		/* this test is quitely ignored */
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(ich1) Insert-character, (dch1) Delete-character start testing [n] > ")) {
		return;
	}
	put_crlf();
	ptext(xch1);
	put_cr();
	pad_test_startup(0);
	for ( ; no_alarm_event; test_complete++) {
		tt_putp(insert_character);
		tt_putp(delete_character);
	}
	pad_test_shutdown(1);
	ptextln(xch1);
	ptext("The preceeding two lines should be the same.  ");
	pad_done_message(t, state, ch);
}

/*
**	pad_rep(test_list, status, ch)
**
**	Test (rep) repeat character
*/
static void
pad_rep(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!repeat_char) {
		ptext("(rep) Repeat-character, not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = columns * 9 / 10;
	if (skip_pad_test(t, state, ch,
		"(rep) Repeat-character start testing [n] > ")) {
		return;
	}
	if (augment > columns - 2) {
		augment = columns - 2;
	}
	if (augment < 2) {
		augment = 2;
	}
	reps = augment;
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		go_home();
		for (i = 2; i < lines; i++) {
			tt_putparm(repeat_char, reps, letter, reps);
			put_crlf();
		}
		for (j = 0; j < reps; j++) {
			putchp(letter);
		}
		put_crlf();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	ptextln(all_lines);
	pad_done_message(t, state, ch);
}

/*
**	pad_cup(test_list, status, ch)
**
**	Test (cup) Cursor address
*/
static void
pad_cup(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j, l, r, c;

	if (!cursor_address) {
		ptext("(cup) Cursor-address not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(cup) Cursor-address start testing [n] > ")) {
		return;
	}
	put_clear();
	ptext("Each line should be filled with the same letter.  There should");
	ptext(" be no gaps, or single letters scattered over the screen.  ");
	if (char_count + 15 > columns) {
		put_crlf();
	}
	if (((lines - line_count) & 1) == 0) {
		/* this removes the gap in the middle of the test when the
		number of lines is odd.  */
		put_crlf();
	}
	r = line_count;
	c = char_count;
	l = (columns - 4) >> 1;
	pad_test_startup(0);
	for ( ; no_alarm_event; test_complete++) {
		for (i = 1; i + i + r < lines; i++) {
			for (j = 0; j <= l; j++) {
				tt_putparm(cursor_address, 1, r + i, j);
				putchp(letter);
				tt_putparm(cursor_address, 1, r + i, l + l + 1 - j);
				putchp(letter);
				tt_putparm(cursor_address, 1, lines - i, j);
				putchp(letter);
				tt_putparm(cursor_address, 1, lines - i, l + l + 1 - j);
				putchp(letter);
			}
			SLOW_TERMINAL_EXIT;
		}
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	tt_putparm(cursor_address, 1, line_count = r, char_count = c);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_hd(test_list, status, ch)
**
**	Test (hd) Half down
*/
static void
pad_hd(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j, k;

	if (!down_half_line) {
		ptext("(hd) Half-line-down not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(hd) Half-line-down start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		for (i = 1; i < columns; i += 2) {
			for (j = 1; j < i; ++j) {
				putchp(' ');
			}
			tt_putp(down_half_line);
			for (k = lines + lines; k > 4; k--) {
				if (j++ >= columns) {
					break;
				}
				tt_putp(down_half_line);
				putchp(letter);
			}
			go_home();
			SLOW_TERMINAL_EXIT;
		}
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	pad_done_message(t, state, ch);
}

/*
**	pad_hu(test_list, status, ch)
**
**	Test (hu) Half line up
*/
static void
pad_hu(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j, k;

	if (!up_half_line) {
		ptext("(hu) Half-line-up not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(hu) Half-line-up start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		for (i = 1; i < columns; i += 2) {
			home_down();
			for (j = 1; j < i; ++j) {
				putchp(' ');
			}
			tt_putp(up_half_line);
			for (k = lines + lines; k > 4; k--) {
				if (j++ >= columns) {
					break;
				}
				tt_putp(up_half_line);
				putchp(letter);
			}
			SLOW_TERMINAL_EXIT;
		}
		go_home();
		NEXT_LETTER;
	}
	pad_test_shutdown(0);
	pad_done_message(t, state, ch);
}

/*
**	pad_rin(test_list, status, ch)
**
**	Test (rin) and (ri) Reverse index
*/
static void
pad_rin(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;
	char *start_message;

	if (t->flags & 1) {
		/* rin */
		if (!parm_rindex) {
			ptext("(rin) Scroll-reverse-n-lines not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = lines - 1;
		start_message = "(rin) Scroll-reverse-n-lines start testing [n] > ";
	} else {
		/* ri */
		if (!scroll_reverse) {
			ptext("(ri) Scroll-reverse not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = 1;
		start_message = "(ri) Scroll-reverse start testing [n] > ";
	}
	if (skip_pad_test(t, state, ch, start_message)) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d\r", test_complete);
		put_str(temp);
		if (scroll_reverse && augment == 1) {
			tt_putp(scroll_reverse);
		} else {
			tt_putparm(parm_rindex, reps, reps, 0);
		}
	}
	put_str("This line should be on the bottom.\r");
	if (scroll_reverse && augment == 1) {
		for (i = 1; i < lines; i++) {
			tt_putp(scroll_reverse);
		}
	} else {
		tt_putparm(parm_rindex, lines - 1, lines - 1, 0);
	}
	putln("The screen should have text on the bottom line.");
	sprintf(temp, "Scroll reverse %d line%s.  ", augment,
		augment == 1 ? "" : "s");
	put_str(temp);
	pad_test_shutdown(0);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_il(test_list, status, ch)
**
**	Test (il) and (il1) Insert line
*/
static void
pad_il(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;
	char *start_message;

	if (t->flags & 1) {
		/* rin */
		if (!parm_insert_line) {
			ptext("(il) Insert-lines not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = lines - 1;
		start_message = "(il) Insert-lines start testing [n] > ";
	} else {
		/* ri */
		if (!insert_line) {
			ptext("(il1) Insert-line not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = 1;
		start_message = "(il1) Insert-line start testing [n] > ";
	}
	if (skip_pad_test(t, state, ch, start_message)) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d\r", test_complete);
		put_str(temp);
		if (insert_line && reps == 1) {
			tt_putp(insert_line);
		} else {
			tt_putparm(parm_insert_line, reps, reps, 0);
		}
	}
	put_str("This line should be on the bottom.\r");
	if (scroll_reverse && augment == 1) {
		for (i = 1; i < lines; i++) {
			tt_putp(insert_line);
		}
	} else {
		tt_putparm(parm_insert_line, lines - 1, lines - 1, 0);
	}
	putln("The screen should have text on the bottom line.");
	sprintf(temp, "Insert %d line%s.  ", augment,
		augment == 1 ? "" : "s");
	put_str(temp);
	pad_test_shutdown(0);
	pad_done_message(t, state, ch);
	put_clear();
}

/*
**	pad_indn(test_list, status, ch)
**
**	Test (indn) and (ind) Scroll forward
*/
static void
pad_indn(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;
	char *start_message;

	if (t->flags & 1) {
		/* indn */
		if (!parm_index) {
			ptext("(indn) Scroll-forward-n-lines not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = lines - 1;
		start_message = "(indn) Scroll-forward-n-lines start testing [n] > ";
	} else {
		/* ind */
		augment = 1;
		start_message = "(ind) Scroll-forward start testing [n] > ";
	}
	if (skip_pad_test(t, state, ch, start_message)) {
		return;
	}
	pad_test_startup(1);
	/* go to the bottom of the screen */
	home_down();
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d\r", test_complete);
		put_str(temp);
		if (augment > 1) {
			tt_putparm(parm_index, reps, reps, 0);
		} else {
			put_ind();
		}
	}
	put_str("This line should be on the top.\r");
	if (augment == 1) {
		for (i = 1; i < lines; i++) {
			put_ind();
		}
	} else {
		tt_putparm(parm_index, lines - 1, lines - 1, 0);
	}
	go_home();
	sprintf(temp, "\nScroll forward %d line%s.  ", augment,
		augment == 1 ? "" : "s");
	put_str(temp);
	pad_test_shutdown(0);
	pad_done_message(t, state, ch);
}

/*
**	pad_dl(test_list, status, ch)
**
**	Test (dl) and (dl1) Delete lines
*/
static void
pad_dl(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;
	char *start_message;

	if (t->flags & 1) {
		/* dl */
		if (!parm_delete_line) {
			ptext("(dl) Delete-lines not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = lines - 1;
		start_message = "(dl) Delete-lines start testing [n] > ";
	} else {
		/* dl1 */
		if (!delete_line) {
			ptext("(dl1) Delete-line not present.  ");
			pad_done_message(t, state, ch);
			return;
		}
		augment = 1;
		start_message = "(dl1) Delete-line start testing [n] > ";
	}
	if (skip_pad_test(t, state, ch, start_message)) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d\r", test_complete);
		if ((i & 0x7f) == 0 && augment < lines - 1) {
			go_home();
			putln(temp);
		}
		put_str(temp);
		if (reps || !delete_line) {
			tt_putparm(parm_delete_line, reps, reps, 0);
		} else {
			tt_putp(delete_line);
		}
	}
	home_down();
	put_str("This line should be on the top.");
	go_home();
	if (reps || !delete_line) {
		tt_putparm(parm_delete_line, lines - 1, lines - 1, 0);
	} else {
		for (i = 1; i < lines; i++) {
			tt_putp(delete_line);
		}
	}
	sprintf(temp, "\nDelete %d line%s.  ", augment,
		augment == 1 ? "" : "s");
	put_str(temp);
	pad_test_shutdown(0);
	pad_done_message(t, state, ch);
}

/*
**	pad_xl(test_list, status, ch)
**
**	Test (il1) Insert and (dl1) Delete lines
*/
static void
pad_xl(
	struct test_list *t,
	int *state,
	int *ch)
{
	if (!insert_line && !delete_line) {
		/* quietly skip this test */
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(il1) Insert-line, (dl1) Delete-line start testing [n] > ")) {
		return;
	}
	put_clear();
	putln("\rThis text is written on the first line.");
	ptext("This sentence begins on the second line.  As this");
	ptext(" test runs the bottom part of this paragraph will");
	ptext(" jump up and down.  Don't worry, that's normal.  When");
	ptext(" the jumping stops, the entire paragraph should");
	ptext(" still be on the screen and in the same place as when");
	ptext(" the test started.  If this paragraph has scrolled");
	ptext(" off the top or bottom of the screen then the test");
	ptext(" has failed.  Scrolling off the top of the screen");
	ptext(" usually means that the delete line capability is");
	ptext(" working better than the insert line capability.  If");
	ptext(" the text scrolls off the bottom then delete line may");
	ptext(" be broken.  If parts of the text are missing then");
	ptext(" you should get professional help.");
	put_crlf();
	go_home();
	put_newlines(2);
	pad_test_startup(0);
	for ( ; no_alarm_event; test_complete++) {
		tt_putp(insert_line);
		put_cr();
		tt_putp(delete_line);
	}
	pad_test_shutdown(0);
	home_down();
	ptext("The top of the screen should have a paragraph of text.  ");
	pad_done_message(t, state, ch);
}

/*
**	pad_scrc(test_list, status, ch)
**
**	Test (sc) (rc) Save/restore cursor
*/
static void
pad_scrc(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;

	if (!save_cursor || !restore_cursor) {
		if (save_cursor) {
			ptext("(rc) Restore-cursor");
		} else
		if (restore_cursor) {
			ptext("(sc) Save-cursor");
		} else {
			ptext("(sc) Save-cursor, (rc) Restore-cursor");
		}
		ptext(" not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(sc) (rc) Save/Restore-cursor start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		page_loop();
		for (i = 1; i < columns; i++) {
			tt_putp(save_cursor);
			putchp(letter);
			tt_putp(restore_cursor);
			putchp('X');
		}
	}
	pad_test_shutdown(0);
	home_down();
	ptext(above_line);
	pad_done_message(t, state, ch);
}

/*
**	pad_csrind(test_list, status, ch)
**
**	Test (csr) and (ind) Change scroll region and index.
*/
static void
pad_csrind(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;

	if (!change_scroll_region) {
		ptext("(csr) Change-scroll-region not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	augment = (lines + 1) / 2;
	if (skip_pad_test(t, state, ch,
		"(csr) Save/Restore-cursor, (ind) index start testing [n] > ")) {
		return;
	}
	if (augment < 2) {
		augment = 2;
	}
	put_clear();
	ptext("This text is on the top line.");
	tt_putparm(change_scroll_region, 1, lines - augment, lines - 1);
	/* go to the bottom of the screen */
	home_down();
	pad_test_startup(0);
	for ( ; no_alarm_event; test_complete++) {
		sprintf(temp, "%d\r", test_complete);
		put_str(temp);
		put_ind();
	}
	ptextln("(csr) is broken.");
	for (i = augment; i > 1; i--) {
		put_ind();
	}
	pad_test_shutdown(0);
	ptext("All but top and bottom lines should be blank.  ");
	pad_done_message(t, state, ch);
	tt_putparm(change_scroll_region, 1, 0, lines - 1);
	put_clear();
}

/*
**	pad_sccsrrc(test_list, status, ch)
**
**	Test (sc) (csr) and (rc) Save/Change/Restore scroll region
*/
static void
pad_sccsrrc(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i;

	if (!save_cursor || !change_scroll_region || !restore_cursor) {
		/* quietly ignore this test */
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(sc) (csr) (rc) Save/Change/Restore-cursor, start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		page_loop();
		for (i = 1; i < columns; i++) {
			tt_putp(save_cursor);
			putchp(letter);
			tt_putparm(change_scroll_region, 1, 0, lines - 1);
			tt_putp(restore_cursor);
			putchp('X');
		}
	}
	pad_test_shutdown(0);
	home_down();
	ptext(above_line);
	pad_done_message(t, state, ch);
	tt_putparm(change_scroll_region, 1, 0, lines - 1);
}

/*
**	pad_csr_nel(test_list, status, ch)
**
**	Test (sc) (csr) (nel) and (rc) Save/Change/Restore scroll region
*/
static void
pad_csr_nel(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!save_cursor || !change_scroll_region || !restore_cursor) {
		/* quietly ignore this test */
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(csr) Change-scroll-region, (nel) newline start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		for (i = 0; i < lines; i++) {
			for (j = lines - i; j > 0; j--) {
				put_crlf();
			}
			tt_putp(save_cursor);
			tt_putparm(change_scroll_region, 1, i, lines - 1);
			tt_putp(restore_cursor);
			put_str(every_line);
		}
		tt_putp(save_cursor);
		tt_putparm(change_scroll_region, 1, 0, lines - 1);
		tt_putp(restore_cursor);
	}
	pad_test_shutdown(0);
	put_str("  ");
	pad_done_message(t, state, ch);
	tt_putparm(change_scroll_region, 1, 0, lines - 1);
}

/*
**	pad_csr_cup(test_list, status, ch)
**
**	Test (csr) (cup) Change scroll region and cursor address
*/
static void
pad_csr_cup(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!change_scroll_region || !cursor_address) {
		/* quietly ignore this test */
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(csr) Change-scroll-region, (cup) cursor-address start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		for (i = 0; i < lines; i++) {
			for (j = lines - i; j > 0; j--) {
				put_crlf();
			}
			tt_putparm(change_scroll_region, 1, i, lines - 1);
			tt_putparm(cursor_address, lines, lines - 1, 0);
			put_str(every_line);
		}
		tt_putparm(change_scroll_region, 1, 0, lines - 1);
		tt_putparm(cursor_address, lines, lines - 1, strlen(every_line));
	}
	pad_test_shutdown(0);
	put_str("  ");
	pad_done_message(t, state, ch);
	tt_putparm(change_scroll_region, 1, 0, lines - 1);
}

/*
**	pad_ht(test_list, status, ch)
**
**	Test (ht) Tabs
*/
static void
pad_ht(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (skip_pad_test(t, state, ch,
		"(ht) Tab start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		/*
		   it is not always possible to test tabs with caps
		   that do not already have padding. The following
		   test uses a mixed bag of tests in order to avoid
		   this problem. Note: I do not scroll
		*/
		if (auto_right_margin && can_go_home)
			for (i = 1, go_home(); i < lines - 2; i++) {
				for (j = 8; j < columns; j += 8) {
					putchp('\t');
				}
				put_str("A        ");
			}
		if (cursor_down && can_go_home)
			for (i = 1, go_home(); i < lines - 2; i++) {
				for (j = 8; j < columns; j += 8) {
					putchp('\t');
				}
				put_str("D\r");
				tt_putp(cursor_down);
			}
		if (cursor_address)
			for (i = 1; i < lines - 2; i++) {
				tt_putparm(cursor_address, lines, i - 1, 0);
				for (j = 8; j < columns; j += 8) {
					putchp('\t');
				}
				put_str("C");
			}
		go_home();
		for (i = 1; i < lines - 2; i++) {
			for (j = 8; j < columns; j += 8) {
				putchp('\t');
			}
			putln("N");
		}
	}
	pad_test_shutdown(0);
	ptextln("Letters on the screen other than Ns at the right margin indicate failure.");
	ptext("A-(am) D-(cud1) C-(cup) N-(nel)  ");
	pad_done_message(t, state, ch);
}

/*
**	pad_smso(test_list, status, ch)
**
**	Test (smso) (rmso) Enter/exit mode
*/
static void
pad_smso(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	if (!enter_standout_mode || !exit_standout_mode) {
		ptext("(smso) (rmso) Enter/Exit-standout-mode not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(smso) (rmso) Enter/Exit-standout-mode start testing [n] > ")) {
		return;
	}
	/*
	   In terminals that emulate non-hidden attributes with hidden
	   attributes, the amount of time that it takes to fill the screen
	   with an attribute is nontrivial. The following test is designed to
	   catch those delays
	*/
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		page_loop();
		j = magic_cookie_glitch > 0 ? magic_cookie_glitch : 0;
		for (i = 2 + j + j; i < columns;) {
			put_mode(enter_standout_mode);
			i += j + j + 2;
			putchp('X');
			put_mode(exit_standout_mode);
			putchp('X');
		}
	}
	pad_test_shutdown(0);
	home_down();
	ptext(above_line);
	pad_done_message(t, state, ch);
	put_mode(exit_standout_mode);
}

/*
**	pad_smacs(test_list, status, ch)
**
**	Test (smacs) (rmacs) Enter/exit altcharset mode
*/
static void
pad_smacs(
	struct test_list *t,
	int *state,
	int *ch)
{
	int i, j;

	/* test enter even if exit is missing */
	if (!enter_alt_charset_mode) {
		ptext("(smacs) Enter-altcharset-mode not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	if (skip_pad_test(t, state, ch,
		"(smacs) (rmacs) Enter/Exit-altcharset-mode start testing [n] > ")) {
		return;
	}
	pad_test_startup(1);
	for ( ; no_alarm_event; test_complete++) {
		page_loop();
		j = magic_cookie_glitch > 0 ? magic_cookie_glitch : 0;
		for (i = 2 + j + j; i < columns;) {
			put_mode(enter_alt_charset_mode);
			i += j + j + 2;
			putchp(letter);
			put_mode(exit_alt_charset_mode);
			putchp(letter);
		}
	}
	pad_test_shutdown(0);
	home_down();
	ptext("Every other character is from the alternate character set.  ");
	pad_done_message(t, state, ch);
	put_mode(exit_alt_charset_mode);
}

/*
**	pad_crash(test_list, status, ch)
**
**	Test (clear) without padding
*/
static void
pad_crash(
	struct test_list *t,
	int *state,
	int *ch)
{
	int save_xon_xoff;

	if (!clear_screen) {
		ptext("(clear) Clear-screen not present.  ");
		pad_done_message(t, state, ch);
		return;
	}
	ptext("If you would like to see if the terminal will really lock up.");
	ptextln("  I will send the clear screen sequence without the pads.");
	if (skip_pad_test(t, state, ch,
		"(clear) Clear-screen start crash testing [n] > ")) {
		return;
	}
	save_xon_xoff = xon_xoff;
	xon_xoff = 1;
	pad_test_startup(0);
	for ( ; no_alarm_event; test_complete++) {
		put_str("Erase this!");
		tt_putp(clear_screen);
	}
	xon_xoff = save_xon_xoff;
	pad_test_shutdown(1);
	pad_done_message(t, state, ch);
}
