/*
** Copyright 2017-2020,2021 Thomas E. Dickey
** Copyright 1997-2015,2017 Free Software Foundation, Inc.
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

/* $Id: tack.h,v 1.89 2021/03/24 20:41:32 tom Exp $ */

#ifndef NCURSES_TACK_H_incl
#define NCURSES_TACK_H_incl 1

/* terminfo action checker include file */

#define MAJOR_VERSION 1
#define MINOR_VERSION 9
#define PATCH_VERSION 20210324

#ifdef HAVE_CONFIG_H
#include <ncurses_cfg.h>
#else
#define HAVE_GETTIMEOFDAY 1
#define HAVE_SELECT 1
#define HAVE_SYS_TIME_H 1
#endif

#ifndef BROKEN_LINKER
#define BROKEN_LINKER 0
#endif

#ifndef GCC_NORETURN
#define GCC_NORETURN		/*nothing */
#endif

#ifndef GCC_UNUSED
#define GCC_UNUSED		/*nothing */
#endif

#ifndef HAVE_LONG_FILE_NAMES
#define HAVE_LONG_FILE_NAMES 0
#endif

#ifndef NCURSES_CONST
#ifdef NCURSES_VERSION
#define NCURSES_CONST const
#else
#define NCURSES_CONST		/*nothing */
#endif
#endif

#ifndef NO_LEAKS
#define NO_LEAKS 0
#endif

#ifndef USE_DATABASE
#define USE_DATABASE 0
#endif

#ifndef USE_TERMCAP
#define USE_TERMCAP 0
#endif

#ifndef USE_RCS_IDS
#define USE_RCS_IDS 0
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>		/* include before curses.h to work around glibc bug */

#include <curses.h>

#if defined(NCURSES_VERSION) && defined(HAVE_TERM_ENTRY_H)
#include <term_entry.h>
#define TACK_CAN_EDIT 1
#else
#define TACK_CAN_EDIT 0
#include <term.h>
#include <termios.h>
#define TTY struct termios
#define TERMIOS 1
#define GET_TTY(fd, buf) tcgetattr(fd, buf)
#endif

#if USE_RCS_IDS
#define MODULE_ID(id) static const char Ident[] = id;
#else
#define MODULE_ID(id)		/*nothing */
#endif

#if defined(__GNUC__) && defined(_FORTIFY_SOURCE)
#define IGNORE_RC(func) ignore_unused = (int) func
#else
#define IGNORE_RC(func) (void) func
#endif /* gcc workarounds */

#ifndef down_half_line
#define down_half_line 0	/* NetBSD bug */
#endif
#ifndef hue_lightness_saturation
#define hue_lightness_saturation 0	/* NetBSD bug */
#endif
#ifndef plab_norm
#define plab_norm 0		/* NetBSD bug */
#endif
#ifndef prtr_off
#define prtr_off 0		/* NetBSD bug */
#endif
#ifndef prtr_on
#define prtr_on 0		/* NetBSD bug */
#endif

#if NO_LEAKS
extern void tack_edit_leaks(void);
extern void tack_fun_leaks(void);
#ifdef HAVE__NC_FREE_TINFO
extern GCC_NORETURN void _nc_free_tinfo(int);
#endif
#ifndef ExitProgram
extern GCC_NORETURN void ExitProgram(int);
#endif
#else
#define ExitProgram(code) exit(code)
#undef  NO_LEAKS
#define NO_LEAKS 0
#endif

#ifdef NCURSES_VERSION
#define ChangeTermInfo(name,value) name = value
#else
#define ChangeTermInfo(name,value)	/* nothing */
#endif

#define FreeIfNeeded(p) if (p) { free(p); p = 0; }

#include <tackgen.h>

#ifdef DECL_CURSES_DATA_BOOLNAMES
#undef boolnames
extern char *boolnames[];
extern size_t max_booleans;

#undef numnames
extern char *numnames[];
extern size_t max_numbers;

#undef strnames
extern char *strnames[];
extern size_t max_strings;

#undef boolfnames
extern char *boolfnames[];

#undef numfnames
extern char *numfnames[];

#undef strfnames
extern char *strfnames[];

#endif

#define ABSENT_STRING		(char *)0
#define CANCELLED_STRING	(char *)(-1)
#define VALID_STRING(s)  ((s) != CANCELLED_STRING && (s) != ABSENT_STRING)

#if TACK_CAN_EDIT
#define CUR_TP      ((TERMTYPE *)(cur_term))
#define MAX_BOOLEAN BOOLCOUNT	/* NUM_BOOLEANS(CUR_TP) */
#define MAX_NUMBERS NUMCOUNT	/* NUM_NUMBERS(CUR_TP) */
#define MAX_STRINGS NUM_STRINGS(CUR_TP)
#define STR_NAME(n) ExtStrname(CUR_TP,n,strnames)
#elif defined(BOOLCOUNT)
#define MAX_BOOLEAN BOOLCOUNT
#define MAX_NUMBERS NUMCOUNT
#define MAX_STRINGS STRCOUNT
#define STR_NAME(n) strnames[n]
#else
#define MAX_BOOLEAN max_booleans
#define MAX_NUMBERS max_numbers
#define MAX_STRINGS max_strings
#define STR_NAME(n) strnames[n]
#endif

typedef enum {
    BOOLEAN,
    NUMBER,
    STRING
} NAME_TYPE;

/* see ncurses' nc_tparm.h */
#define TPARM_FMT NCURSES_CONST char *
#define TPARM_ARG long
#define TPARM_N(n) (TPARM_ARG)(n)

#define TPARM_9(a,b,c,d,e,f,g,h,i,j) tparm((TPARM_FMT)a,TPARM_N(b),TPARM_N(c),TPARM_N(d),TPARM_N(e),TPARM_N(f),TPARM_N(g),TPARM_N(h),TPARM_N(i),TPARM_N(j))
#define TPARM_8(a,b,c,d,e,f,g,h,i) TPARM_9(a,b,c,d,e,f,g,h,i,0)
#define TPARM_7(a,b,c,d,e,f,g,h) TPARM_8(a,b,c,d,e,f,g,h,0)
#define TPARM_6(a,b,c,d,e,f,g) TPARM_7(a,b,c,d,e,f,g,0)
#define TPARM_5(a,b,c,d,e,f) TPARM_6(a,b,c,d,e,f,0)
#define TPARM_4(a,b,c,d,e) TPARM_5(a,b,c,d,e,0)
#define TPARM_3(a,b,c,d) TPARM_4(a,b,c,d,0)
#define TPARM_2(a,b,c) TPARM_3(a,b,c,0)
#define TPARM_1(a,b) TPARM_2(a,b,0)
#define TPARM_0(a) TPARM_1(a,0)

#define UChar(c)    ((unsigned char)(c))

#define NAME_SIZE 32
#define TEMP_SIZE 1024

#define LOG_FILENAME "tack.log"
#define DBG_FILENAME "debug.log"

extern FILE *log_fp;
extern FILE *debug_fp;
extern int debug_level;
extern char temp[TEMP_SIZE];
extern char *tty_basename;
extern char tty_shortname[];

#if defined(__GNUC__) && defined(_FORTIFY_SOURCE)
extern int ignore_unused;
#endif

#define SYNC_FAILED	0
#define SYNC_TESTED	1
#define SYNC_NOT_TESTED	2
#define SYNC_NEEDED	3

extern int tty_can_sync;
extern int total_pads_sent;	/* count pad characters sent */
extern int total_caps_sent;	/* count caps sent */
extern int total_printing_characters;	/* count printing characters sent */
extern SIG_ATOMIC_T no_alarm_event;	/* TRUE if the alarm has not gone off yet */
extern unsigned long usec_run_time;	/* length of last test in microseconds */
extern int raw_characters_sent;	/* Total output characters */

/* Stopwatch event timers */
#define TIME_TEST 0
#define TIME_SYNC 1
#define TIME_FLUSH 2
#define MAX_TIMERS 3

/* definitions for pad.c */

#define ENSURE_DELAY  if (!tt_delay_used) napms(10)
#define EXIT_CONDITION (no_alarm_event && (tt_delay_used < tt_delay_max))
#define SLOW_TERMINAL_EXIT if (!test_complete && !EXIT_CONDITION) { break; }
#define CAP_NOT_FOUND if (auto_pad_mode) return

extern char letters[26 + 1];
#define NEXT_LETTER letter = letters[letter_number =\
	letters[letter_number + 1] ? letter_number + 1 : 0]

extern int test_complete;	/* counts number of tests completed */
extern char letter;
extern int letter_number;
extern int augment, repeats;
extern long char_sent;
extern const char *pad_repeat_test;	/* commands that force repeat */

extern int replace_mode;
extern int char_count, line_count, expand_chars;
extern int can_go_home, can_clear_screen;

extern int translate_mode, scan_mode;
extern int auto_pad_mode;	/* TRUE for auto time tests */
extern int char_mask;
extern int hex_out;		/* Display output in hex */

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
extern unsigned tty_baud_rate;
extern unsigned long tty_cps;	/* The number of characters per second */
extern SIG_ATOMIC_T not_a_tty;
extern int nodelay_read;
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
typedef struct default_string_list {
    const char *name;		/* terminfo name */
    const char *value;		/* value of default string */
    int index;			/* index into the strfname[] array */
} DefaultStringList;

#define TM_last 8
extern struct default_string_list TM_string[TM_last];

/* attribute structure definition */
typedef struct mode_list {
    const char *name;
    const char *begin_mode;
    const char *end_mode;
    int number;
} ModeList;

extern const struct mode_list alt_modes[];
extern const int mode_map[];

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
extern const char *tt_cap[TT_MAX];	/* value of string */
extern int tt_affected[TT_MAX];	/* lines or columns effected (repetition
				   factor) */
extern int tt_count[TT_MAX];	/* Number of times sent */
extern int tt_delay[TT_MAX];	/* Number of milliseconds delay */
extern int ttp;			/* number of entries used */

extern const char *tx_cap[TT_MAX];	/* value of string */
extern int tx_affected[TT_MAX];	/* lines or columns effected (repetition
				   factor) */
extern int tx_count[TT_MAX];	/* Number of times sent */
extern int tx_delay[TT_MAX];	/* Number of milliseconds delay */
extern int tx_index[TT_MAX];	/* String index */
extern int txp;			/* number of entries used */
extern int tx_characters;	/* printing characters sent by test */
extern unsigned long tx_cps;	/* characters per second */

/*
	Menu control for tack.
*/

struct test_list;
typedef void (TestFunc) (struct test_list * t, int *state, int *ch);

struct test_menu;
typedef void (MenuFunc) (struct test_menu *);

typedef struct test_results {
    struct test_results *next;	/* point to next entry */
    struct test_list *test;	/* Test which got these results */
    int reps;			/* repeat count */
    int delay;			/* delay times 10 */
} TestResults;

typedef struct test_list {
    int flags;			/* Test description flags */
    int lines_needed;		/* Lines needed for test (0->no action) */
    const char *caps_done;	/* Caps shown in Done message */
    const char *caps_tested;	/* Other caps also being tested */
    const char *menu_entry;	/* Menu entry text (optional) */
    TestFunc *test_procedure;	/* Function that does testing */
    struct test_menu *sub_menu;	/* Nested sub-menu */
} TestList;

typedef struct test_menu {
    int flags;			/* Menu feature flag */
    int default_action;		/* Default command if <cr> <lf> entered */
    const char *menu_text;	/* Describe this test_menu */
    const char *menu_title;	/* Title for the menu */
    const char *ident;		/* short menu name */
    const char *standard_tests;	/* Standard test text */
    MenuFunc *menu_function;	/* print current settings (optional) */
    TestList *tests;		/* Pointer to the menu/function pairs */
    TestList *resume_tests;	/* Standard test resume point */
    int resume_state;		/* resume state of test group */
    int resume_char;		/* resume ch of test group */
} TestMenu;

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

/* tack.c */
#if TACK_CAN_EDIT
extern TestMenu edit_menu;
#define MY_EDIT_MENU	{0, 0, 0, 0, "e) edit terminfo", 0, &edit_menu},
#else
#define MY_EDIT_MENU		/* nothing */
#endif
#ifdef DEBUG
#define TACKMSG(p) p
extern void TackMsg(const char *, ...) GCC_PRINTFLIKE(1,2);
#else
#define TACKMSG(p)		/* nothing */
#endif
extern void show_usage(char *);
extern void print_version(void);

/* output.c */
extern char *expand(const char *);
extern char *expand_command(const char *);
extern char *expand_to(char *, int);
extern char *hex_expand_to(char *, int);
extern char *print_expand(char *);
extern int getchp(int);
extern int getnext(int);
extern int log_chr(FILE *, int, int);
extern int tc_putp(const char *);
extern int wait_here(void);
extern void go_home(void);
extern void home_down(void);
extern void log_str(FILE *, const char *);
extern void maybe_wait(int);
extern void ptext(const char *);
extern void ptextln(const char *);
extern void put_clear(void);
extern void put_columns(const char *, int, int);
extern void put_cr(void);
extern void put_crlf(void);
extern void put_ind(void);
extern void put_lf(void);
extern void put_newlines(int);
extern void put_str(const char *);
extern void put_this(int);
extern void putchp(int);
extern void putln(const char *);
extern void read_string(char *, size_t);
extern void tt_putp(const char *);
extern void tt_putparm(NCURSES_CONST char *, int, int, int);
extern void tt_tputs(const char *, int);

/* Solaris is out of step - humor it */
#if defined(__EXTENSIONS__) && !defined(NCURSES_VERSION)
#define TC_PUTCH char
#else
#define TC_PUTCH int
#endif
extern int tc_putch(TC_PUTCH);

#define put_that(n) put_this((int) (n))

/* control.c */
extern TestList color_test_list[];
extern char *liberated(const char *);
extern char txt_longer_augment[80];
extern char txt_longer_test_time[80];
extern char txt_shorter_augment[80];
extern char txt_shorter_test_time[80];
extern int msec_cost(const char *const, int);
extern int skip_pad_test(TestList *, int *, int *, const char *);
extern int sliding_scale(int, int, unsigned long);
extern int still_testing(void);
extern long event_time(int);
extern void control_init(void);
extern void dump_test_stats(TestList *, int *, int *);
extern void event_start(int);
extern void longer_augment(TestList *, int *, int *);
extern void longer_test_time(TestList *, int *, int *);
extern void pad_test_shutdown(TestList *, int);
extern void pad_test_startup(int);
extern void page_loop(void);
extern void set_augment_txt(void);
extern void shorter_augment(TestList *, int *, int *);
extern void shorter_test_time(TestList *, int *, int *);

/* charset.c */
extern TestList acs_test_list[];
extern void set_attr(int);
extern void eat_cookie(void);
extern void put_mode(const char *);

/* crum.c */
extern TestList crum_test_list[];

/* ansi.c */
extern void tools_status(TestList *, int *, int *);
extern void tools_charset(TestList *, int *, int *);
extern void tools_sgr(TestList *, int *, int *);

/* edit.c */
#if TACK_CAN_EDIT
extern TestMenu change_pad_menu;
extern TestList edit_test_list[];
#define MY_PADS_MENU	{0, 0, 0, 0, "p) change padding", 0, &change_pad_menu},
#else
#define MY_PADS_MENU		/* nothing */
#endif
extern const char *get_string_cap_byname(const char *, const char **);
extern int cap_match(const char *names, const char *cap);
extern int get_string_cap_byvalue(const char *);
extern int user_modified(void);
extern void can_test(const char *, int);
extern void cap_index(const char *, int *);
extern void edit_init(void);
extern void save_info(TestList *, int *, int *);
extern void show_report(TestList *, int *, int *);

/* fun.c */
extern TestList funkey_test_list[];
extern TestList printer_test_list[];
extern void enter_key(const char *, char *, char *);
extern int tty_meta_prep(void);
extern void tools_report(TestList *, int *, int *);

/* init.c */
extern const char *safe_tgets(NCURSES_CONST char *);
extern void reset_init(void);
extern void display_basic(void);
extern void charset_can_test(void);
extern void curses_setup(char *);
extern void bye_kids(int);

/* scan.c */
extern char **scan_up, **scan_down, **scan_name;
extern int scan_key(void);
extern size_t scan_max;		/* length of longest scan code */
extern size_t *scan_tested, *scan_length;
extern void scan_init(char *fn);

/* sysdep.c */
extern int initial_stty_query(int);
extern int stty_query(int);
extern void ignoresig(void);
extern void read_key(char *, size_t);
extern void set_alarm_clock(int);
extern void spin_flush(void);
extern void tty_init(void);
extern void tty_raw(int, int);
extern void tty_reset(void);
extern void tty_set(void);

/* menu.c */
extern char prompt_string[80];	/* menu prompt storage */
extern int subtest_menu(TestList *, int *, int *);
extern TestList *augment_test;
extern void generic_done_message(TestList *, int *, int *);
extern void menu_can_scan(const TestMenu *);
extern void menu_clear_screen(TestList *, int *, int *);
extern void menu_display(TestMenu *, int *);
extern void menu_prompt(void);
extern void menu_reset_init(TestList *, int *, int *);
extern void pad_done_message(TestList *, int *, int *);

/* modes.c */
extern TestList mode_test_list[];

/* pad.c */
extern TestList pad_test_list[];

/* sync.c */
extern TestMenu sync_menu;
extern int tty_sync_error(void);
extern void flush_input(void);
extern void sync_handshake(TestList *, int *, int *);
extern void sync_test(TestMenu *);
extern void verify_time(void);

#endif /* NCURSES_TACK_H_incl */
