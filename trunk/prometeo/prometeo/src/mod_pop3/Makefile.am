## Makefile.am -- Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.2 2004-04-24 13:51:48 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

pkglib_LTLIBRARIES = mod_pop3.la
noinst_HEADERS     = *.h

mod_pop3_la_SOURCES = mod_pop3.cpp procpool.cpp client.cpp cmdfilter.cpp spamdfilter.cpp
mod_pop3_la_LDFLAGS = -no-undefined -module -avoid-version
mod_pop3_la_LIBADD  =

