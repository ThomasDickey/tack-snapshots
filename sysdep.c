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
/*
 * Operating system dependant functions.  We assume the POSIX API.
 * Note: on strict-POSIX systems (including BSD/OS) the select_delay_type
 * global has no effect.
 */

#include <tack.h>

#ifdef sun
#include <time.h>
#endif

#if HAVE_SELECT
#if HAVE_SYS_TIME_H && HAVE_SYS_TIME_SELECT
#include <sys/time.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif

#include <signal.h>
#include <termios.h>
#include <errno.h>

MODULE_ID("$Id: sysdep.c,v 1.5 1997/12/28 20:15:20 tom Exp $")

#if DECL_ERRNO
extern int errno;
#endif

/* globals */
int tty_frame_size;		/* asynch frame size times 2 */
unsigned long tty_baud_rate;	/* baud rate - bits per second */
int not_a_tty;			/* TRUE if output is not a tty (i.e. pipe) */
int nodelay_read;		/* TRUE if NDELAY is set */

#define TTY_IS_NOECHO	!(new_modes.c_lflag & ECHO)
#define TTY_IS_OUT_TRANS (new_modes.c_oflag & OPOST)
#define TTY_IS_CHAR_MODE !(new_modes.c_lflag & ICANON)
#define TTY_WAS_CS8 ((old_modes.c_cflag & CSIZE) == CS8)
#define TTY_WAS_XON_XOFF (old_modes.c_iflag & (IXON|IXOFF))

struct termios old_modes, new_modes;

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
	new_modes = old_modes;
#if HAVE_SELECT
	new_modes.c_cc[VMIN] = 1;
#else
	new_modes.c_cc[VMIN] = minch;
#endif
	new_modes.c_cc[VTIME] = 2;
	new_modes.c_lflag &=
		~(ISIG | ICANON | XCASE | ECHO | ECHOE | ECHOK | ECHONL);
#ifdef LOBLK
	new_modes.c_lflag &= ~LOBLK;
#endif
	new_modes.c_oflag &= ~(OPOST | OLCUC | TABDLY);
	if (mask == ALLOW_PARITY) {
		new_modes.c_cflag &= ~(CSIZE | PARENB | HUPCL);
		new_modes.c_cflag |= CS8;
	}
	new_modes.c_iflag &=
		~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL |
		IUCLC | IXON | IXANY | IXOFF);
	if (not_a_tty)
		return;
	tcsetattr(fileno(stdin), TCSAFLUSH, &new_modes);
}

void 
tty_set(void)
{				/* set tty to special modes */
	new_modes = old_modes;
	new_modes.c_cc[VMIN] = 1;
	new_modes.c_cc[VTIME] = 1;
	new_modes.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
#if defined(ONLCR) && defined(OCRNL) && defined(ONLRET) && defined(OFILL)
	new_modes.c_oflag &= ~(ONLCR | OCRNL | ONLRET | OFILL);
#else
	new_modes.c_oflag &= ~(OPOST);
#endif
	if (char_mask == ALLOW_PARITY)
		new_modes.c_iflag &= ~ISTRIP;
	switch (select_xon_xoff) {
	case 0:
		new_modes.c_iflag &= ~(IXON | IXOFF);
		break;
	case 1:
#if sequent
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
			~(NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY);
#endif	/* NLDLY */
		break;
	case 1:
#ifdef NLDLY
		new_modes.c_oflag &=
			~(NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY);
#endif	/* NLDLY */
#ifdef NL1
		new_modes.c_oflag |= NL1 | CR2;
#endif	/* NL1 */
		break;
	}
	if (!(new_modes.c_oflag & ~OPOST))
		new_modes.c_oflag &= ~OPOST;
	if (not_a_tty)
		return;
	tcsetattr(fileno(stdin), TCSAFLUSH, &new_modes);
}


void 
tty_reset(void)
{				/* reset the tty to the original modes */
	fflush(stdout);
	if (not_a_tty)
		return;
	tcsetattr(fileno(stdin), TCSAFLUSH, &old_modes);
}


void 
tty_init(void)
{				/* ATT terminal init */
#ifdef F_GETFL
	int flags;

	flags = fcntl(fileno(stdin), F_GETFL, 0);
	nodelay_read = flags & O_NDELAY;
#else
	   nodelay_read = FALSE;
#endif
	not_a_tty = FALSE;
	if (tcgetattr(fileno(stdin), &old_modes) == -1) {
		if (errno == ENOTTY) {
			tty_frame_size = 20;
			not_a_tty = TRUE;
			return;
		}
		printf("tcgetattr error: %d\n", errno);
		exit(1);
	}
	/* if TAB3 is set then setterm() wipes out tabs (ht) */
	new_modes = old_modes;
#ifdef TABDLY
	new_modes.c_oflag &= ~TABDLY;
#endif	/* TABDLY */
	if (tcsetattr(fileno(stdin), TCSAFLUSH, &new_modes) == -1) {
		printf("tcsetattr error: %d\n", errno);
		exit(1);
	}
#ifdef sequent
	/* the sequent ATT emulation is broken soooo. */
	old_modes.c_cflag &= ~(CSIZE | CSTOPB);
	old_modes.c_cflag |= CS7 | PARENB;
#endif
	catchsig();
	switch (old_modes.c_cflag & CSIZE) {
	case CS5:
		tty_frame_size = 10;
		break;
	case CS6:
		tty_frame_size = 12;
		break;
	case CS7:
		tty_frame_size = 14;
		break;
	case CS8:
		tty_frame_size = 16;
		break;
	}
	tty_frame_size += 2 +
		((old_modes.c_cflag & PARENB) ? 2 : 0) +
		((old_modes.c_cflag & CSTOPB) ? 4 : 2);
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
		return TTY_IS_OUT_TRANS;
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
		return TTY_WAS_XON_XOFF;
	}
	return (-1);
}

#if HAVE_SELECT
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
	n = select(fileno(stdin)+1, &ifds, NULL, NULL, &tv);
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
#define char_ready() 1
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
			(void) read(fileno(stdin), &buf, sizeof(buf));
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
read_key(char *buf, int max)
{
	int got, ask, i, l;
	char *s;

	*buf = '\0';
	s = buf;
	fflush(stdout);
	/* ATT unix may return 0 or 1, Berkeley Unix should be 1 */
	while (read(fileno(stdin), s, 1) == 0);
	++s;
	--max;
	while (max > 0 && (ask = char_ready())) {
		if (ask > max) {
			ask = max;
		}
		if ((got = read(fileno(stdin), s, ask))) {
			s += got;
		} else {
			break;
		}
		max -= got;
	}
	*s = '\0';
	l = s - buf;
	for (s = buf, i = 0; i < l; i++) {
		if ((*s & 0x7f) == 0) {
			/* convert nulls to 0x80 */
			*s = 128;
		} else {
			/* strip high order bits (if any) */
			*s &= char_mask;
		}
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
    onintr( )
 
 is the interrupt handling routine onintr turns off interrupts while doing
    clean-up
 
 onintr always exits fatally
 */


static RETSIGTYPE 
onintr(int sig GCC_UNUSED)
{
	ignoresig();
	tty_reset();
	exit(1);
}


 /*
    catchsig( )
 
 set up to field interrupts (via function onintr( )) so that if interrupted
    we can restore the correct terminal modes
 
 catchsig simply returns
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
	(void) alarm(seconds);
}
