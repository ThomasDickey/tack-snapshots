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
/* initialization and wrapup code */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curses.h>
#include "tac.h"

FILE *debug_fp;
char temp[512];
char tty_basename[64];
char tty_shortname[64];

void
put_name(char *cap, char *name)
{				/* send the cap name followed by the cap */
	if (cap) {
		ptext(name);
		tc_putp(cap);
	}
}

void
report_cap(char *tag, char *s)
{				/* expand the cap or print *** missing *** */
	int i;

	ptext(tag);
	for (i = char_count; i < 13; i++)
		putchp(' ');
	put_str(" = ");
	if (s)
		putln(expand(s));
	else
		putln("*** missing ***");
}


void
reset_init(int sending)
{				/* send the reset and init strings */
	int i;

	can_test("(lines)(cols)(cr)", FLAG_CAN_TEST);
	if (!sending)
		return;

	if (init_prog)
		(void) system(init_prog);

	ptext("Terminal reset");
	i = char_count;
	put_name(reset_1string, " (rs1)");
	put_name(reset_2string, " (rs2)");
	put_name(reset_3string, " (rs3)");
	if (i != char_count)
		put_crlf();

	ptext(" init");
	put_name(init_1string, " (is1)");
	put_name(init_2string, " (is2)");
	if (set_tab && clear_all_tabs && init_tabs != 8) {
		put_crlf();
		tc_putp(clear_all_tabs);
		for (char_count = 0; char_count < columns; char_count++) {
			put_this(' ');
			if ((char_count & 7) == 7)
				tc_putp(set_tab);
		}
		put_cr();
	}
	/* run the initialization file */
	if (init_file && init_file[0]) {
		FILE *fp;
		int ch;

		if ((fp = fopen(init_file, "r"))) {	/* send the init file */
			sprintf(temp, " (if) %s", init_file);
			ptextln(temp);
			while (1) {
				ch = getc(fp);
				if (ch == EOF)
					break;
				put_this(ch);
			}
			fclose(fp);
		} else {
			sprintf(temp, "\nCannot open init file (if) %s", init_file);
			ptextln(temp);
		}
	}
	put_name(init_3string, " (is3)");

	fflush(stdout);
	sleep(1);
}

void
display_basic(void)
{				/* display the basic terminal definitions */
	put_str("Name: ");
	putln(ttytype);

	report_cap("\\r ^M (cr)", carriage_return);
	report_cap("\\n ^J (ind)", scroll_forward);
	report_cap("\\b ^H (cub1)", cursor_left);
	report_cap("\\t ^I (ht)", tab);
/*      report_cap("\\f ^L (ff)", form_feed);	*/
	report_cap("      (clear)", clear_screen);
	report_cap("      (home)", cursor_home);
	report_cap("      (nel)", newline);
	report_cap("ENQ   (u9)", user9);
	report_cap("ACK   (u8)", user8);

	sprintf(temp, "\nTerminal size: %d x %d.  Baud rate: %ld.  Frame size: %d.%d", columns, lines, tty_baud_rate, tty_frame_size >> 1, (tty_frame_size & 1) * 5);
	putln(temp);
}


void
bye_kids(int n)
{				/* reset the tty and exit */
	ignoresig();
	if (send_reset_init) {
		if (exit_ca_mode) {
			tc_putp(exit_ca_mode);
		}
		if (initial_stty_query(TTY_XON_XOFF)) {
			if (enter_xon_mode) {
				tc_putp(enter_xon_mode);
			}
		} else if (exit_xon_mode) {
			tc_putp(exit_xon_mode);
		}
	}
	if (debug_fp)
		fclose(debug_fp);
	tty_reset();
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	if (not_a_tty)
		sleep(1);
	exit(n);
}
