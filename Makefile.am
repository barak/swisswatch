ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swisswatch

swisswatch_SOURCES = swisswatch.c

AM_CPPFLAGS = -D_GNU_SOURCE $(GTK4_CFLAGS)
swisswatch_LDADD = $(GTK4_LIBS) -lm

swisswatch.1: swisswatch$(EXEEXT) $(srcdir)/swisswatch.h2m
	LC_ALL=C $(HELP2MAN) \
	  --name="Swiss Railway analog clock" \
	  --help-option=--help-all \
	  --no-info \
	  --include=$(srcdir)/swisswatch.h2m \
	  ./swisswatch$(EXEEXT) \
	| sed 's/\[OPTION???\]/[OPTION...]/g' > $@

man_MANS = swisswatch.1
CLEANFILES = swisswatch.1

desktopdir = $(datadir)/applications
dist_desktop_DATA = swisswatch.desktop

EXTRA_DIST = swisswatch.h2m

doc_DATA = README ChangeLog
