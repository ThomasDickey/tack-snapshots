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
/* terminfo action checker include file */

#include "term.h"

extern FILE *debug_fp;
extern int debug_level;
extern char temp[];
extern char tty_basename[];
extern char tty_shortname[];

#define SYNC_NOT_TESTED	2

extern int ACK_char, junk_chars, tty_can_sync;
extern int total_pads_sent;	/* count pad characters sent */
extern int total_caps_sent;	/* count caps sent */
extern int total_printing_characters;	/* count printing characters sent */
extern int no_alarm_event;	/* TRUE if the alarm has not gone off yet */
extern int usec_run_time;	/* length of last test in microseconds */

/* definitions for pad.c */

#define SLOW_TERMINAL_EXIT if (!test_complete && !no_alarm_event) { break; }

#define NEXT_LETTER letter = letters[letter_number =\
	letters[letter_number + 1] ? letter_number + 1 : 0]

extern int test_complete;	/* counts number of tests completed */
extern char letter;
extern int letter_number;
extern int augment, reps;
extern long char_sent;

#define CLEAR_TEST_MAX 5
extern int clear_select;
extern int clr_test_value[CLEAR_TEST_MAX];
extern int clr_test_reps[CLEAR_TEST_MAX];

extern int replace_mode;
extern int char_count, line_count, expand_chars;
extern int can_go_home, can_clear_screen;

extern int translate_mode, scan_mode;
extern int time_pad;
extern int char_mask;

/* Parity bit macros */
#define STRIP_PARITY 0x7f
#define ALLOW_PARITY 0xff

/* select_delay_type:	0 -> reset all delays
			1 -> force long delays
			2 -> do not change the delays */
extern int select_delay_type;

/* select_xon_xoff:	0 -> reset xon/xoff
			1 -> set xon/xoff
			2 -> do not change xon/xoff */
extern int select_xon_xoff;

extern int tty_frame_size;
extern unsigned long tty_baud_rate;
extern int not_a_tty, nodelay_read;
extern int send_reset_init;

/* definitions for stty_query() and initial_stty_query() */
#define TTY_CHAR_MODE	0
#define TTY_NOECHO	1
#define TTY_OUT_TRANS	2
#define TTY_8_BIT	3
#define TTY_XON_XOFF	4

/* scan code definitions */
#define MAX_SCAN 256

/* translate mode default strings */
extern char *TM_carriage_return;
extern char *TM_cursor_down;
extern char *TM_scroll_forward;
extern char *TM_newline;
extern char *TM_cursor_left;
extern char *TM_bell;
extern char *TM_form_feed;
extern char *TM_tab;

/* attribute structure definition */
struct mode_list {
	char *name;
	char *begin_mode, *end_mode;
	int number;
};

/* Test data base */

#define FLAG_CAN_TEST	1
#define FLAG_TESTED	2
#define FLAG_LABEL	4
#define FLAG_FUNCTION_KEY	8

/* caps under test data base */

#define TT_MAX	8
#define MAX_CHANGES (TT_MAX+2)

extern char *tt_cap[TT_MAX];	/* value of string */
extern int tt_affected[TT_MAX];	/* lines or columns effected (repitition
				   factor) */
extern int tt_count[TT_MAX];	/* Number of times sent */
extern int ttp;			/* number of entries used */

extern char *tx_cap[TT_MAX];	/* value of string */
extern int tx_affected[TT_MAX];	/* lines or columns effected (repitition
				   factor) */
extern int tx_count[TT_MAX];	/* Number of times sent */
extern int tx_index[TT_MAX];	/* String index */
extern int txp;			/* number of entries used */
extern int tx_characters;	/* printing characters sent by test */
extern int tx_cps;		/* characters per second */

/*
	Menu control for tack.
*/

struct test_list {
	int flags;		/* Test description flags */
	int lines_needed;	/* Lines needed for test (0->no action) */
	char *caps_done;	/* Caps shown in Done message */
	char *caps_tested;	/* Other caps also being tested */
	char *menu_entry;	/* Menu entry text (optional) */
				/* Function that does testing */
	void (*test_procedure)(struct test_list *, int *, int *);
	struct test_menu *sub_menu;	/* Nested sub-menu */
};

/* test_procedure(flags, return-of-last-call) */

struct test_menu {
	int flags;		/* Menu feature flag */
	char *prompt;		/* Prompt for input */
	char *menu_text;	/* Describe this test_menu */
	char *menu_title;	/* Title for the menu */
	char *ident;		/* short menu name */
	char *standard_tests;	/* Standard test text */
				/* print current settings (optional) */
	void (*menu_function)(struct test_menu *);
	struct test_list *tests;	/* Pointer to the menu/function pairs */
	struct test_list *resume_tests;	/* Standard test resume point */
	int resume_state;	/* resume state of test group */
	int resume_char;	/* resume ch of test group */
};


/* menu flags */
#define MENU_CLEAR	0x000100	/* clear screen */
#define MENU_INIT	0x000200	/* Initialization function */
#define MENU_NEXT	0x000400	/* Next test in sequence */
#define MENU_LAST	0x000800	/* End of menu list */
#define MENU_STOP	0x001000	/* Stop testing next-in-sequence */

extern struct test_menu edit_menu;

/* output.c */
extern void tt_tputs(const char *, int);
extern void tt_putp(const char *);
extern void tt_putparm(const char *, int, int, int);
extern int tc_putp(const char *);
extern int tc_putch(int);
extern void putchp(char);
extern void put_cr(void);
extern void put_crlf(void);
extern void put_clear(void);
extern void put_dec(char *, int);
extern void put_str(char *);
extern void put_lf(void);
extern void put_ind(void);
extern void put_newlines(int);
extern void put_columns(char *, int, int);
extern void put_this(int);
extern void putln(char *);
extern void ptext(char *);
extern void ptextln(char *);
extern void home_down(void);
extern void go_home(void);
extern void three_digit(char *, int);
extern int getchp(int);
extern char *expand(char *);
extern char *expand_to(char *, int);
extern char *expand_command(char *);
extern char *hex_expand_to(char *, int);
extern char *print_expand(char *);
extern void maybe_wait(int);
extern int wait_here(void);
extern void read_string(char *, int);
extern int getnext(int);

/* control.c */
extern char *liberated(char *);
extern int run_test(char *, char *, int);
extern int run_mode(char *);
extern int begin_test(char **, char *, char *, char *, int);
extern int repeat_pad_test(char *, char *, int);
extern void short_subject(int);
extern void page_loop(void);
extern void control_init(void);
extern int multi_test( /* this is actually variadic */ );
extern int acco_pad(char *, char *);
extern void pad_time(char *, int *, int *);
extern int skip_pad_test(struct test_list *, int *, int *, char *);
extern void pad_test_startup(int);
extern void pad_test_shutdown(int);
extern void dump_test_stats(struct test_list *, int *, int *);
extern void longer_test_time(struct test_list *, int *, int *);
extern void shorter_test_time(struct test_list *, int *, int *);
extern char txt_longer_test_time[80];
extern char txt_shorter_test_time[80];
extern int sliding_scale(int, int, int);

/* sync.c */
extern void verify_time(void);
extern int tty_sync_error(int);
extern int enq_ack(void);
extern void flush_input(void);
extern void sync_test(struct test_menu *);

/* charset.c */
extern void set_attr(int);
extern void eat_cookie(void);
extern void put_mode(char *);

/* init.c */
extern void reset_init(int);
extern void bye_kids(int);
extern void display_basic(void);
extern void put_name(char *, char *);
extern void charset_can_test(void);

/* scan.c */
extern int scan_key(void);
extern void scan_init(char *fn);

/* ansi.c */
extern int test_ansi_reports(void);
extern int test_ansi_graphics(void);
extern int test_ansi_sgr(void);

/* pad.c */

/* fun.c */
extern void enter_key(char *, char *, char *);
extern int enter_cap(char *, char *);
extern int tty_meta_prep(void);
extern void test_report(int, int);

/* sysdep.c */
extern void tty_set(void);
extern void tty_raw(int, int);
extern void read_key(char *, int);
extern void set_alarm_clock(int);
extern void tty_init(void);
extern void tty_reset(void);
extern void ignoresig(void);
extern int stty_query(int);
extern int initial_stty_query(int);

/* edit.c */
extern int user_modified(void);
extern void save_info(struct test_list *, int *, int *);
extern void can_test(char *, int);
extern void edit_init(void);
extern char *get_string_cap_byname(char *, char **);
extern int get_string_cap_byvalue(char *);
extern void show_report(struct test_list *, int *, int *);

/* menu.c */
extern void menu_can_scan(struct test_menu *);
extern void menu_display(struct test_menu *, int *);
extern void generic_done_message(struct test_list *, int *, int *);
extern void pad_done_message(struct test_list *, int *, int *);
extern void menu_clear_screen(struct test_list *, int *, int *);
extern void menu_reset_init(struct test_list *, int *, int *);
extern int subtest_menu(struct test_list *, int *, int *);

