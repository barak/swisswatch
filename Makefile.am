ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = swisswatch

swisswatch_SOURCES = \
 swisswatch.c \
 Hand.c HandOrMark.c Mark.c SwissWatch.c TableConvert.c \
 Hand.h HandOrMark.h Mark.h SwissWatch.h TableConvert.h \
 SwissWatcP.h HandOrMarP.h HandP.h MarkP.h

AM_CPPFLAGS = -D_GNU_SOURCE

swisswatch.1: swisswatch.man
	cp $< $@
man_MANS = swisswatch.1

appdefaultsdir = $(sysconfdir)/X11/app-defaults

SwissWatch: SWatch.ad
	cp $< $@
SwissWatch-color: SWatch-co.ad
	cp $< $@
appdefaults_DATA = SwissWatch SwissWatch-color

doc_DATA = README ChangeLog
