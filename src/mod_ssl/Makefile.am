## Makefile.am -- Process this file with automake to produce Makefile.in
# $Id: Makefile.am,v 1.3 2002-12-08 16:54:09 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

if BUILD_WITH_SSL
pkglib_LTLIBRARIES = mod_ssl.la
endif

noinst_HEADERS     = *.h

mod_ssl_la_SOURCES = mod_ssl.cpp procpool.cpp client.cpp
mod_ssl_la_LDFLAGS = -no-undefined -module -avoid-version
mod_ssl_la_LIBADD  =

