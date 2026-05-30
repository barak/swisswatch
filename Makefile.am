ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swisswatch

swisswatch_SOURCES = swisswatch.c swisswatch_resources.c

AM_CPPFLAGS = -D_GNU_SOURCE $(GTK4_CFLAGS)
swisswatch_LDADD = $(GTK4_LIBS) -lm

BUILT_SOURCES = swisswatch_resources.c
swisswatch_resources.c: $(srcdir)/swisswatch.gresource.xml $(srcdir)/swisswatch.svg
	$(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --target=$@ --generate-source \
	  $(srcdir)/swisswatch.gresource.xml

swisswatch.1: swisswatch$(EXEEXT) $(srcdir)/swisswatch.h2m
	LC_ALL=C $(HELP2MAN) \
	  --name="Swiss Railway analog clock" \
	  --help-option=--help-all \
	  --no-info \
	  --include=$(srcdir)/swisswatch.h2m \
	  ./swisswatch$(EXEEXT) \
	| sed 's/\[OPTION???\]/[OPTION...]/g' > $@

man_MANS = swisswatch.1
CLEANFILES = swisswatch.1 swisswatch_resources.c

desktopdir = $(datadir)/applications
dist_desktop_DATA = org.debian.swisswatch.desktop

iconsdir = $(datadir)/icons/hicolor/scalable/apps
dist_icons_DATA = swisswatch.svg

install-data-hook:
	gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor || :

EXTRA_DIST = swisswatch.h2m swisswatch.gresource.xml

doc_DATA = README.md ChangeLog
