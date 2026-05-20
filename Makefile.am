ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swisswatch

swisswatch_SOURCES = swisswatch.c

AM_CPPFLAGS = -D_GNU_SOURCE $(GTK4_CFLAGS)
swisswatch_LDADD = $(GTK4_LIBS) -lm

swisswatch.1: swisswatch.man
	cp $< $@
man_MANS = swisswatch.1

doc_DATA = README ChangeLog
