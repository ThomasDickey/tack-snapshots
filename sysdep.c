/*
** Copyright 2017-2021,2025 Thomas E. Dickey
** Copyright 1997-2010,2012 Free Software Foundation, Inc.
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
/*
 * Operating system dependent functions.  We assume the POSIX API.
 * Note: on strict-POSIX systems (including BSD/OS) the select_delay_type
 * global has no effect.
 */

#include <tack.h>

#include <term.h>
#include <errno.h>
#include <regex.h>

#if defined(__BEOS__)
#undef false
#undef true
#include <OS.h>
#endif

#if HAVE_SELECT
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif

MODULE_ID("$Id: sysdep.c,v 1.42 2025/05/03 17:34:24 tom Exp $")

#ifdef TERMIOS
#define PUT_TTY(fd, buf) tcsetattr(fd, TCSAFLUSH, buf)
#else
#define PUT_TTY(fd, buf) stty(fd, buf)
#endif

/* globals */
int tty_frame_size;		/* asynch frame size times 2 */
unsigned tty_baud_rate;		/* baud rate - bits per second */
SIG_ATOMIC_T not_a_tty;		/* TRUE if output is not a tty (i.e. pipe) */
int nodelay_read;		/* TRUE if NDELAY is set */

#ifdef TERMIOS
#define TTY_IS_NOECHO	!(new_modes.c_lflag & ECHO)
#define TTY_IS_OUT_TRANS (new_modes.c_oflag & OPOST)
#define TTY_IS_CHAR_MODE !(new_modes.c_lflag & ICANON)
#define TTY_WAS_CS8 ((old_modes.c_cflag & CSIZE) == CS8)
#define TTY_WAS_XON_XOFF (old_modes.c_iflag & (IXON|IXOFF))
#else
#define TTY_IS_NOECHO	!(new_modes.sg_flags & (ECHO))
#define TTY_IS_OUT_TRANS (new_modes.sg_flags & (CRMOD))
#define TTY_IS_CHAR_MODE (new_modes.sg_flags & (RAW|CBREAK))
#define TTY_WAS_CS8	 (old_modes.sg_flags & (PASS8))
#define TTY_WAS_XON_XOFF (old_modes.sg_flags & (TANDEM|MDMBUF|DECCTQ))
#endif

static TTY old_modes, new_modes;

void catchsig(void);

/*
 * These are a sneaky way of conditionalizing bit unsets so strict-POSIX
 * systems won't see them.
 */
#ifndef XCASE
#define XCASE	0
#endif
#ifndef OLCUC
#define OLCUC	0
#endif
#ifndef IUCLC
#define IUCLC	0
#endif
#ifndef TABDLY
#define	TABDLY	0
#endif
#ifndef IXANY
#define	IXANY	0
#endif

void
tty_raw(int minch GCC_UNUSED, int mask)
{				/* set tty to raw noecho */
    if (debug_fp) {
	fprintf(debug_fp, "tty_raw:\n");
    }
    new_modes = old_modes;
#ifdef TERMIOS
#if HAVE_SELECT
    new_modes.c_cc[VMIN] = 1;
#else
    new_modes.c_cc[VMIN] = minch;
#endif
    new_modes.c_cc[VTIME] = 2;
    new_modes.c_lflag &=
	(tcflag_t) ~(ISIG | ICANON | XCASE | ECHO | ECHOE | ECHOK | ECHONL);
#ifdef LOBLK
    new_modes.c_lflag &= (tcflag_t) ~LOBLK;
#endif
    new_modes.c_oflag &= (tcflag_t) ~(OPOST | OLCUC | TABDLY);
    if (mask == ALLOW_PARITY) {
	new_modes.c_cflag &= (tcflag_t) ~(CSIZE | PARENB | HUPCL);
	new_modes.c_cflag |= CS8;
    }
    new_modes.c_iflag &=
	(tcflag_t) ~(IGNBRK
		     | BRKINT
		     | IGNPAR
		     | PARMRK
		     | INPCK
		     | ISTRIP
		     | INLCR
		     | IGNCR
		     | ICRNL
		     | IUCLC
		     | IXON
		     | IXANY
		     | IXOFF);
#else
    new_modes.sg_flags |= RAW;
#endif
    if (not_a_tty) {
	if (debug_fp) {
	    fprintf(debug_fp, "...tty_raw: not a tty\n");
	}
	return;
    }
    PUT_TTY(fileno(stdin), &new_modes);
    if (debug_fp) {
	fprintf(debug_fp, "...tty_raw: done\n");
    }
}

void
tty_set(void)
{				/* set tty to special modes */
    if (debug_fp) {
	fprintf(debug_fp, "tty_set:\n");
    }
    new_modes = old_modes;
#ifdef TERMIOS
    new_modes.c_cc[VMIN] = 1;
    new_modes.c_cc[VTIME] = 0;
    new_modes.c_lflag &= (tcflag_t) ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
#if defined(ONLCR) && defined(OCRNL) && defined(ONLRET) && defined(OFILL)
    new_modes.c_oflag &= (tcflag_t) ~(ONLCR | OCRNL | ONLRET | OFILL);
#else
    new_modes.c_oflag &= (tcflag_t) ~(OPOST);
#endif
    if (char_mask == ALLOW_PARITY)
	new_modes.c_iflag &= (tcflag_t) ~ISTRIP;
    switch (select_xon_xoff) {
    case 0:
	new_modes.c_iflag &= (tcflag_t) ~(IXON | IXOFF);
	break;
    case 1:
#if defined(sequent) && sequent
	/* the sequent System V emulation is broken */
	new_modes = old_modes;
	new_modes.c_cc[VEOL] = 6;	/* control F  (ACK) */
#endif
	new_modes.c_iflag |= IXON | IXOFF;
	break;
    }
    switch (select_delay_type) {
    case 0:
#ifdef NLDLY
	new_modes.c_oflag &=
	    (tcflag_t) ~(NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY);
#endif /* NLDLY */
	break;
    case 1:
#ifdef NLDLY
	new_modes.c_oflag &=
	    (tcflag_t) ~(NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY);
#endif /* NLDLY */
#ifdef NL1
	new_modes.c_oflag |= NL1 | CR2;
#endif /* NL1 */
	break;
    }
    if ((new_modes.c_oflag & (tcflag_t) ~OPOST) == 0)
	new_modes.c_oflag &= (tcflag_t) ~OPOST;
#else
    new_modes.sg_flags |= RAW;
    if (not_a_tty) {
	if (debug_fp) {
	    fprintf(debug_fp, "...tty_set: not a tty\n");
	}
	return;
    }
#endif
    PUT_TTY(fileno(stdin), &new_modes);
    if (debug_fp) {
	fprintf(debug_fp, "...tty_set: done\n");
    }
}

void
tty_reset(void)
{				/* reset the tty to the original modes */
    if (debug_fp) {
	fprintf(debug_fp, "tty_reset:\n");
    }
    fflush(stdout);
    if (not_a_tty) {
	if (debug_fp) {
	    fprintf(debug_fp, "...tty_reset: not a tty\n");
	}
	return;
    }
    PUT_TTY(fileno(stdin), &old_modes);
    if (debug_fp) {
	fprintf(debug_fp, "...tty_reset: done\n");
    }
}

void
tty_init(void)
{				/* ATT terminal init */
#if defined(F_GETFL) && defined(O_NDELAY)
    int flags;

    flags = fcntl(fileno(stdin), F_GETFL, 0);
    nodelay_read = flags & O_NDELAY;
#else
    nodelay_read = FALSE;
#endif
    not_a_tty = FALSE;
    if (GET_TTY(fileno(stdin), &old_modes) == -1) {
	if (errno == ENOTTY) {
	    tty_frame_size = 20;
	    not_a_tty = TRUE;
	    return;
	}
	printf("tcgetattr error: %d\n", errno);
	ExitProgram(EXIT_FAILURE);
    }
    /* if TAB3 is set then setterm() wipes out tabs (ht) */
    new_modes = old_modes;
#ifdef TERMIOS
#ifdef TABDLY
    new_modes.c_oflag &= (tcflag_t) ~TABDLY;
#endif /* TABDLY */
#endif
    if (PUT_TTY(fileno(stdin), &new_modes) == -1) {
	printf("tcsetattr error: %d\n", errno);
	ExitProgram(EXIT_FAILURE);
    }
#ifdef sequent
    /* the sequent ATT emulation is broken soooo. */
    old_modes.c_cflag &= ~(CSIZE | CSTOPB);
    old_modes.c_cflag |= CS7 | PARENB;
#endif
    catchsig();
#ifdef TERMIOS
    switch (old_modes.c_cflag & CSIZE) {
#if defined(CS5) && (CS5 != 0)
    case CS5:
	tty_frame_size = 10;
	break;
#endif
#if defined(CS6) && (CS6 != 0)
    case CS6:
	tty_frame_size = 12;
	break;
#endif
#if defined(CS7) && (CS7 != 0)
    case CS7:
	tty_frame_size = 14;
	break;
#endif
#if defined(CS8) && (CS8 != 0)
    case CS8:
	tty_frame_size = 16;
	break;
#endif
    }
    tty_frame_size += 2 +
	((old_modes.c_cflag & PARENB) ? 2 : 0) +
	((old_modes.c_cflag & CSTOPB) ? 4 : 2);
#else
    tty_frame_size = 6 +
	(old_modes.sg_flags & PASS8) ? 16 : 14;
#endif
}

/*
**	stty_query(question)
**
**	Does the current driver settings have this property?
*/
int
stty_query(int q)
{
    switch (q) {
    case TTY_NOECHO:
	return TTY_IS_NOECHO;
    case TTY_OUT_TRANS:
	return (int) TTY_IS_OUT_TRANS;
    case TTY_CHAR_MODE:
	return TTY_IS_CHAR_MODE;
    }
    return (-1);
}

/*
**	initial_stty_query(question)
**
**	Did the initial driver settings have this property?
*/
int
initial_stty_query(int q)
{
    switch (q) {
    case TTY_8_BIT:
	return TTY_WAS_CS8;
    case TTY_XON_XOFF:
	return (int) TTY_WAS_XON_XOFF;
    }
    return (-1);
}

#if HAVE_SELECT && defined(FD_ZERO)
static int
char_ready(void)
{
    int n;
    fd_set ifds;
    struct timeval tv;

    FD_ZERO(&ifds);
    FD_SET(fileno(stdin), &ifds);
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    n = select(fileno(stdin) + 1, &ifds, NULL, NULL, &tv);
    return (n != 0);
}

#else
#ifdef FIONREAD
int
char_ready(void)
{
    int i, j;

    /* the following loop has to be tuned for each computer */
    for (j = 0; j < 1000; j++) {
	ioctl(fileno(stdin), FIONREAD, &i);
	if (i)
	    return i;
    }
    return i;
}

#else
#if defined(__BEOS__)
int
char_ready(void)
{
    int n = 0;
    int howmany = ioctl(0, 'ichr', &n);
    return (howmany >= 0 && n > 0);
}
#else
#define char_ready() 1
#endif
#endif
#endif

/*
**	spin_flush()
**
**	Wait for the input stream to stop.
**	Throw away all input characters.
*/
void
spin_flush(void)
{
    unsigned char buf[64];

    fflush(stdout);
    event_start(TIME_FLUSH);	/* start the timer */
    do {
	if (char_ready()) {
	    if (read(fileno(stdin), &buf, sizeof(buf)) != 0)
		break;
	}
    } while (event_time(TIME_FLUSH) < 400000);
}

/*
**	read_key(input-buffer, length-of-buffer)
**
**	read one function key from the input stream.
**	A null character is converted to 0x80.
*/
void
read_key(char *buf, size_t max)
{
    int ask, i, l;
    char *s;

    if (debug_fp) {
	fprintf(debug_fp, "read_key: max=%lu\n", (unsigned long) max);
    }
    *buf = '\0';
    s = buf;
    fflush(stdout);
    /* ATT unix may return 0 or 1, Berkeley Unix should be 1 */
    while (read(fileno(stdin), s, (size_t) 1) <= 0) {
	;			/* EMPTY */
    }
    ++s;
    --max;
    while ((int) max > 0 && (ask = char_ready()) > 0) {
	int got;

	if (ask > (int) max) {
	    ask = (int) max;
	}
	if ((got = (int) read(fileno(stdin), s, (size_t) ask)) > 0) {
	    s += got;
	} else {
	    break;
	}
	max -= (size_t) got;
    }
    *s = '\0';
    l = (int) (s - buf);
    for (s = buf, i = 0; i < l; i++) {
	if ((*s & 0x7f) == 0) {
	    /* convert nulls to 0x80 */
	    *(unsigned char *) s = 128;
	} else {
	    /* strip high order bits (if any) */
	    *s = (char) (*s & char_mask);
	}
    }
    if (debug_fp) {
	fprintf(debug_fp, "...read_key: result=");
	log_str(debug_fp, buf);
	fprintf(debug_fp, "\n");
    }
}

void
ignoresig(void)
{
    /* ignore signals */
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
}

/*
 * onintr( )
 *
 * is the interrupt handling routine.
 * onintr turns off interrupts while doing clean-up.
 *
 * onintr always exits fatally
 */
static void
onintr(int sig GCC_UNUSED)
{
    ignoresig();
    tty_reset();
    ExitProgram(EXIT_FAILURE);
}

/*
 * catchsig( )
 *
 * set up to field interrupts (via function onintr( )) so that if interrupted
 * we can restore the correct terminal modes
 *
 * catchsig simply returns
 */
void
catchsig(void)
{
    if ((signal(SIGINT, SIG_IGN)) == SIG_DFL)
	signal(SIGINT, onintr);

    if ((signal(SIGHUP, SIG_IGN)) == SIG_DFL)
	signal(SIGHUP, onintr);

    if ((signal(SIGQUIT, SIG_IGN)) == SIG_DFL)
	signal(SIGQUIT, onintr);

    if ((signal(SIGTERM, SIG_IGN)) == SIG_DFL)
	signal(SIGTERM, onintr);

}

/*
**	alarm_event(sig)
**
**	Come here for an alarm event
*/
static void
alarm_event(
	       int sig GCC_UNUSED)
{
    no_alarm_event = 0;
}

/*
**	set_alarm_clock(seconds)
**
**	Set the alarm clock to fire in <seconds>
*/
void
set_alarm_clock(
		   int seconds)
{
    signal(SIGALRM, alarm_event);
    no_alarm_event = 1;
    (void) alarm((unsigned) seconds);
}

typedef enum {
    ESCAPE = '\033',
    PERCENT = '%',
    BAKSLSH = '\\',
    L_BLOCK = '[',
    R_BLOCK = ']',
    ONETIME = '?'
} CHARS;

/*
**	Try to match a regular expression, reporting mismatches.
*/
int
compare_regex(const char *name, const char *terminfo, const char *actual)
{
    int rc = 0;
    int code;
    regex_t regex;
    char pattern[1024];
    char buffer[2 * sizeof(pattern)];

    /*
     * Transform the terminfo "pattern" into a regular expression:
     *
     * ncurses terminfo has several response patterns.  The point of this code
     * is to formalize the syntax.
     *
     * a) %[xxx] -> [xxx]+
     * b) meta -> \meta, e.g., "[" or "?" without preceding "%" or "\\"
     * c) trim %p[1-9] for now, possible use for substrings
     */
    if (terminfo == NULL || strlen(terminfo) >= (sizeof(pattern) / 2)) {
	rc = -1;
	if (debug_fp)
	    fprintf(debug_fp, "ABSENT %s\n", terminfo);
    } else {
	int escape = 0;
	int use_plus = 0;
	int nest_block = 0;
	const char *s = terminfo;
	char *d = pattern;

	while (*s != '\0') {
	    char ch = *s++;
	    if (escape) {
		escape = 0;
		*d++ = BAKSLSH;
	    } else if (ch == BAKSLSH) {
		escape = 1;
		continue;
	    } else if (ch == ONETIME) {
		*d++ = BAKSLSH;
	    } else if (ch == PERCENT) {
		if (*s == L_BLOCK) {
		    if (!nest_block++) {
			use_plus = 1;
			ch = *s++;
		    }
		} else if (*s == 'p' && isdigit(UChar(s[1]))) {
		    s += 2;
		    continue;
		} else if (*s == ONETIME) {
		    *d++ = ch;
		    ch = *s++;
		}
	    } else if (ch == L_BLOCK) {
		if ((s - terminfo) == 2
		    && *terminfo == ESCAPE
		    && (strchr(s + 1, L_BLOCK) != NULL
			|| strchr(s + 1, R_BLOCK) == NULL)) {
		    *d++ = BAKSLSH;
		} else if (!nest_block++) {
		    use_plus = 1;
		}
	    } else if (ch == R_BLOCK) {
		if (use_plus && --nest_block == 0) {
		    *d++ = ch;
		    ch = '+';
		}
	    }
	    *d++ = ch;
	}
	*d = '\0';
    }

    if (rc < 0) {
	sprintf(buffer, "Unexpected pattern (%s): %.30s", name, terminfo);
	putln(expand(buffer));
	if (debug_fp) {
	    fprintf(debug_fp, "Bad regex(%s): %s\n", name, expand(buffer));
	}
	return rc;
    }

    /*
     * Now we can use POSIX regex.
     */
    if (debug_fp) {
	sprintf(buffer, "%s", pattern);
	fprintf(debug_fp, "Regex(%s): %s\n", name, expand(buffer));
    }
#define NPARAM 10
    if ((code = regcomp(&regex, pattern, REG_EXTENDED)) == 0) {
	if (actual != NULL) {
	    regmatch_t pmatch[NPARAM];
	    memset(pmatch, 0, sizeof(pmatch));
	    if (regexec(&regex, actual, NPARAM, pmatch, 0) == 0) {
		int want_1st = 0;
		int want_end = (int) strlen(actual);
		if (pmatch[0].rm_so == want_1st &&
		    pmatch[0].rm_eo == want_end) {
		    int argc;
		    rc = 1;
		    for (argc = 1; argc < NPARAM; ++argc) {
			size_t len;
			if (pmatch[argc].rm_so < 0)
			    continue;
			if (pmatch[argc].rm_eo < pmatch[argc].rm_so)
			    continue;
			sprintf(buffer, "  arg%d %d..%d = ",
				argc,
				(int) pmatch[argc].rm_so,
				(int) pmatch[argc].rm_eo);
			ptext(buffer);
			len = (size_t) (pmatch[argc].rm_eo - pmatch[argc].rm_so);
			strncpy(buffer, actual + pmatch[argc].rm_so, len);
			buffer[len] = '\0';
			putln(expand(buffer));
		    }
		} else {
		    sprintf(buffer, "Matched %d..%d vs %d..%d\n",
			    (int) pmatch[0].rm_so,
			    (int) pmatch[0].rm_eo,
			    want_1st,
			    want_end);
		    putln(buffer);
		}
	    }
	} else {
	    rc = 1;
	}
    } else {
	regerror(code, &regex, buffer, sizeof(buffer));
	putln(expand(buffer));
    }
    regfree(&regex);
    return rc;
}
