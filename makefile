# Makefile for terminfo/termcap test program
#
# The following pre-processor variables may be set.
# 
# SELECT	Use this define if your operating system has the select
#		system call.
#
# WAIT_MODE	Use this define if your operating system cannot tell if
#		a chracter is ready in the input queue.  Funtion keys
#		must be padded with blanks.
#
# 	If generated without pre-processor variables, a terminfo test
#	program will be generated for System V Release 3.

BINDIR=/usr/local/bin

# PATH for .c and .h files
DIR=.

CC=gcc

DEBUG=-g

#				use this to build on LINUX
CFLAGS=$(DEBUG) -ansi -Wall -DSELECT -D_POSIX_SOURCE -I/usr/include/ncurses
LDFLAGS=-lncurses
#
#				use this to build on sun
#CFLAGS=$(DEBUG) -Wall -DSELECT -I/usr/include/ncurses -I/usr/local/include -I/usr/5include
#LDFLAGS=-lncurses

CMD=tack

OFILES= ansi.o charset.o color.o control.o crum.o edit.o fun.o init.o \
	menu.o modes.o output.o pad.o scan.o sync.o sysdep.o tack.o

CFILES= $(DIR)/ansi.c $(DIR)/charset.c $(DIR)/color.c $(DIR)/control.c \
	$(DIR)/crum.c $(DIR)/edit.c $(DIR)/fun.c $(DIR)/init.c \
	$(DIR)/menu.c $(DIR)/modes.c $(DIR)/output.c $(DIR)/pad.c \
	$(DIR)/scan.c $(DIR)/sync.c $(DIR)/sysdep.c $(DIR)/tack.c

#CFILES=$(OFILES:.o=.c) 

$(CMD): $(OFILES)
	$(CC) -g -o $(CMD) $(OFILES) $(LDFLAGS)

$(OFILES): $(DIR)/tac.h

lint:
	lint $(CFILES)

clean:
	rm -f $(OFILES) $(CMD) TAGS tack.tar tack.tar.gz

# vi tags
tags: $(CFILES) $(DIR)/tac.h
	ctags $(>)

# emacs tags
TAGS: $(CFILES)
	etags $(CFILES)

install: $(CMD)
	cp $(CMD) $(BINDIR)

list: $(DIR)/tac.h $(CFILES)
	for name in $(>) ; \
	do \
		ucb cat -n $$name | ucb pr -h "$$name" | ucb lpr ; \
	done

#	archive a backup copy
arch: $(DIR)/Makefile $(DIR)/tac.h $(CFILES)
	rm -f tack.a
	ar q tack.a $(>)

tack.tar.gz: tack.tar
	gzip tack.tar

#tack.tar: README tack.1 GRIPES Makefile *.[ch] 
#	tar -cvf tack.tar README tack.1 GRIPES Makefile *.[ch]

tack.tar: makefile *.[ch] 
	tar -cvf tack.tar makefile *.[ch]
