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

#define MAJOR_VERSION 0
#define MINOR_VERSION 2

extern FILE *log_fp;
extern FILE *debug_fp;
extern int debug_level;
extern char temp[];
extern char tty_basename[];
extern char tty_shortname[];

#define SYNC_FAILED	0
#define SYNC_TESTED	1
#define SYNC_NOT_TESTED	2
#define SYNC_NEEDED	3

extern int tty_can_sync;
extern int total_pads_sent;	/* count pad characters sent */
extern int total_caps_sent;	/* count caps sent */
extern int total_printing_characters;	/* count printing characters sent */
extern int no_alarm_event;	/* TRUE if the alarm has not gone off yet */
extern int usec_run_time;	/* length of last test in microseconds */
extern int raw_characters_sent;	/* Total output characters */

/* Stopwatch event timers */
#define TIME_TEST 0
#define TIME_SYNC 1
#define TIME_FLUSH 2
#define MAX_TIMERS 3

/* definitions for pad.c */

#define EXIT_CONDITION (no_alarm_event && (tt_delay_used < tt_delay_max))
#define SLOW_TERMINAL_EXIT if (!test_complete && !EXIT_CONDITION) { break; }
#define CAP_NOT_FOUND if (auto_pad_mode) return

extern char letters[26];
#define NEXT_LETTER letter = letters[letter_number =\
	letters[letter_number + 1] ? letter_number + 1 : 0]

extern int test_complete;	/* counts number of tests completed */
extern char letter;
extern int letter_number;
extern int augment, reps;
extern long char_sent;
extern char *pad_repeat_test;	/* commands that force repeat */

extern int replace_mode;
extern int char_count, line_count, expand_chars;
extern int can_go_home, can_clear_screen;

extern int translate_mode, scan_mode;
extern int auto_pad_mode;		/* TRUE for auto time tests */
extern int char_mask;
extern int hex_out;			/* Display output in hex */

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
extern int tty_cps;		/* The number of characters per second */
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
struct default_string_list {
	char *name;	/* terminfo name */
	char *value;	/* value of default string */
	int index;	/* index into the strfname[] array */
};

#define TM_last 8
extern struct default_string_list TM_string[TM_last];

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

extern int tt_delay_max;	/* max number of milliseconds we can delay */
extern int tt_delay_used;	/* number of milliseconds consumed in delay */
extern char *tt_cap[TT_MAX];	/* value of string */
extern int tt_affected[TT_MAX];	/* lines or columns effected (repitition
				   factor) */
extern int tt_count[TT_MAX];	/* Number of times sent */
extern int tt_delay[TT_MAX];	/* Number of milliseconds delay */
extern int ttp;			/* number of entries used */

extern char *tx_cap[TT_MAX];	/* value of string */
extern int tx_affected[TT_MAX];	/* lines or columns effected (repitition
				   factor) */
extern int tx_count[TT_MAX];	/* Number of times sent */
extern int tx_delay[TT_MAX];	/* Number of milliseconds delay */
extern int tx_index[TT_MAX];	/* String index */
extern int txp;			/* number of entries used */
extern int tx_characters;	/* printing characters sent by test */
extern int tx_cps;		/* characters per second */

/*
	Menu control for tack.
*/

struct test_results {
	struct test_results *next;	/* point to next entry */
	struct test_list *test;	/* Test which got these results */
	int reps;		/* repeat count */
	int delay;		/* delay times 10 */
};

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

struct test_menu {
	int flags;		/* Menu feature flag */
	int default_action;	/* Default command if <cr> <lf> entered */
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
#define MENU_100c	0x00001a00	/* Augment 100% of columns */
#define MENU_90c	0x00001900	/* Augment 90% of columns */
#define MENU_80c	0x00001800	/* Augment 80% of columns */
#define MENU_70c	0x00001700	/* Augment 70% of columns */
#define MENU_60c	0x00001600	/* Augment 60% of columns */
#define MENU_50c	0x00001500	/* Augment 50% of columns */
#define MENU_40c	0x00001400	/* Augment 40% of columns */
#define MENU_30c	0x00001300	/* Augment 30% of columns */
#define MENU_20c	0x00001200	/* Augment 20% of columns */
#define MENU_10c	0x00001100	/* Augment 10% of columns */
#define MENU_LM1	0x00002e00	/* Augment lines - 1 */
#define MENU_100l	0x00002a00	/* Augment 100% of lines */
#define MENU_90l	0x00002900	/* Augment 90% of lines */
#define MENU_50l	0x00002500	/* Augment 50% of lines */
#define MENU_lines	0x00002000	/* Augment of lines */
#define MENU_columns	0x00001000	/* Augment of columns */
#define MENU_LC_MASK	0x00003000	/* Augment mask for lines and columns */
#define MENU_1L		0x00002f00	/* Augment == one */
#define MENU_1C		0x00001f00	/* Augment == one */
#define MENU_ONE	0x00000f00	/* Augment == one */
#define MENU_ONE_MASK	0x00000f00	/* Augment == one mask */
#define MENU_REP_MASK	0x00003f00	/* Augment mask */

#define MENU_CLEAR	0x00010000	/* clear screen */
#define MENU_INIT	0x00020000	/* Initialization function */
#define MENU_NEXT	0x00040000	/* Next test in sequence */
#define MENU_LAST	0x00080000	/* End of menu list */
#define MENU_STOP	0x00100000	/* Stop testing next-in-sequence */
#define MENU_COMPLETE	0x00200000	/* Test complete after this */
#define MENU_MENU	0x00400000	/* Pass the menu name not test name */

#define REQUEST_PROMPT 256

extern char prompt_string[80];	/* menu prompt storage */
extern struct test_menu edit_menu;
extern struct test_list *augment_test;

/* tack.c */
extern void show_usage(char *);

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
extern void event_start(int);
extern long event_time(int);
extern char *liberated(char *);
extern void page_loop(void);
extern void control_init(void);
extern int msec_cost(const char *const, int);
extern int skip_pad_test(struct test_list *, int *, int *, char *);
extern void pad_test_startup(int);
extern int still_testing(void);
extern void pad_test_shutdown(struct test_list *, int);
extern void dump_test_stats(struct test_list *, int *, int *);
extern void longer_test_time(struct test_list *, int *, int *);
extern void shorter_test_time(struct test_list *, int *, int *);
extern char txt_longer_test_time[80];
extern char txt_shorter_test_time[80];
extern void set_augment_txt(void);
extern void longer_augment(struct test_list *, int *, int *);
extern void shorter_augment(struct test_list *, int *, int *);
extern char txt_longer_augment[80];
extern char txt_shorter_augment[80];
extern int sliding_scale(int, int, int);

/* sync.c */
extern void verify_time(void);
extern int tty_sync_error(void);
extern void flush_input(void);
extern void sync_test(struct test_menu *);
extern void sync_handshake(struct test_list *, int *, int *);

/* charset.c */
extern void set_attr(int);
extern void eat_cookie(void);
extern void put_mode(char *);

/* init.c */
extern void reset_init(void);
extern void display_basic(void);
extern void put_name(char *, char *);
extern void charset_can_test(void);
extern void curses_setup(char *);
extern void bye_kids(int);

/* scan.c */
extern int scan_key(void);
extern void scan_init(char *fn);

/* ansi.c */
extern void tools_status(struct test_list *, int *, int *);
extern void tools_charset(struct test_list *, int *, int *);
extern void tools_sgr(struct test_list *, int *, int *);

/* pad.c */

/* fun.c */
extern void enter_key(char *, char *, char *);
extern int tty_meta_prep(void);
extern void tools_report(struct test_list *, int *, int *);

/* sysdep.c */
extern void tty_set(void);
extern void tty_raw(int, int);
extern void tty_init(void);
extern void tty_reset(void);
extern void spin_flush(void);
extern void read_key(char *, int);
extern void set_alarm_clock(int);
extern void ignoresig(void);
extern int stty_query(int);
extern int initial_stty_query(int);

/* edit.c */
extern int user_modified(void);
extern void save_info(struct test_list *, int *, int *);
extern void can_test(char *, int);
extern void cap_index(char *, int *);
extern int cap_match(char *names, char *cap);
extern void edit_init(void);
extern char *get_string_cap_byname(char *, char **);
extern int get_string_cap_byvalue(char *);
extern void show_report(struct test_list *, int *, int *);

/* menu.c */
extern void menu_prompt(void);
extern void menu_can_scan(struct test_menu *);
extern void menu_display(struct test_menu *, int *);
extern void generic_done_message(struct test_list *, int *, int *);
extern void pad_done_message(struct test_list *, int *, int *);
extern void menu_clear_screen(struct test_list *, int *, int *);
extern void menu_reset_init(struct test_list *, int *, int *);
extern int subtest_menu(struct test_list *, int *, int *);

/* definitions found in ncurses(3X) */
extern int _nc_nulls_sent;		/* Add one for every null sent */
extern char _nc_trans_string(char *);
