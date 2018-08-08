#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([swisswatch],[0.6])
AM_INIT_AUTOMAKE([foreign -Wall])
AC_CONFIG_SRCDIR([swisswatch.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AX_CFLAGS_WARN_ALL

# Checks for libraries.
AC_SEARCH_LIBS([sqrt], [m])
AC_SEARCH_LIBS([sincos], [m])
AC_SEARCH_LIBS([XFillArc], [X11])
AC_SEARCH_LIBS([XShapeCombineMask], [Xext])
AC_SEARCH_LIBS([XmuCopyISOLatin1Lowered], [Xmu])
AC_SEARCH_LIBS([XtDisplay], [Xt])

# Checks for header files.
# AC_CHECK_HEADERS([stdlib.h])
# AC_CHECK_HEADERS([time.h])
# AC_CHECK_HEADERS([X11/extensions/shape.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_MEMBERS([struct tm.tm_gmtoff],[AC_DEFINE([HAVE_GMTOFF],[1],[Define to 1 if `tm_gmtoff' is a member of `struct tm'])],,[
#include <time.h>
])

# Checks for library functions.
# AC_CHECK_FUNCS([gettimeofday sqrt])
AC_CHECK_FUNCS([sincos])
AC_CHECK_FUNC([XShapeQueryExtension],[AC_DEFINE([SHAPE],[1],[Define to 1 shape extension is available])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
