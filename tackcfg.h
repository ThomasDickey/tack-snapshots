/*
** Copyright 2025 Thomas E. Dickey
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

/* $Id: tackcfg.h,v 1.1 2025/04/27 20:43:21 tom Exp $ */

#ifndef TACK_CFG_H_incl
#define TACK_CFG_H_incl 1

/* terminfo action checker configuration file */

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

#if defined(HAVE_CURSES_DATA_BOOLNAMES) || defined(DECL_CURSES_DATA_BOOLNAMES)
#define USE_CURSES_ARRAYS 1
#else
#define USE_CURSES_ARRAYS 0
#endif

#endif /* TACK_CFG_H_incl */
