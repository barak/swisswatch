#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([swisswatch],[0.7])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_CONFIG_SRCDIR([swisswatch.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AX_CFLAGS_WARN_ALL

# Checks for libraries.
AC_SEARCH_LIBS([sqrt], [m])

PKG_CHECK_MODULES([GTK4], [gtk4])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_MEMBERS([struct tm.tm_gmtoff],[AC_DEFINE([HAVE_GMTOFF],[1],[Define to 1 if tm_gmtoff is a member of struct tm])],,[
#include <time.h>
])

AM_MISSING_PROG([HELP2MAN], [help2man])
AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])
AS_IF([test -z "$GLIB_COMPILE_RESOURCES"],
    [AC_MSG_ERROR([glib-compile-resources not found; install libglib2.0-dev])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
