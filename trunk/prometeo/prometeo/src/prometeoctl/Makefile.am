# $Id: Makefile.am,v 1.1.1.1 2002-10-10 09:59:55 tellini Exp $

MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS		= prometeoctl

CPPFLAGS 			= @CPPFLAGS@
CXXFLAGS 			= @CXXFLAGS@

INCLUDES			= -I$(top_srcdir)/src
prometeoctl_SOURCES	= prometeoctl.cpp
prometeoctl_LDADD   = ../libprom/libprom.la

