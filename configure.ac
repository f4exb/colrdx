AC_INIT([colrdx], [1.0.4])
AC_CONFIG_SRCDIR([src/colrdx.c])
AM_INIT_AUTOMAKE

AC_PROG_CC

AC_CHECK_HEADER(ncurses.h, , AC_MSG_ERROR([You must install libncurses development package]))
AC_CHECK_LIB(ncurses, initscr, , AC_MSG_ERROR([You must install libncurses library]))

AC_OUTPUT(Makefile src/Makefile)
