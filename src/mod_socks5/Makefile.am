## Makefile.am -- Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.1 2003-10-23 17:27:17 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

pkglib_LTLIBRARIES = mod_socks5.la
noinst_HEADERS     = *.h

mod_socks5_la_SOURCES = mod_socks5.cpp procpool.cpp client.cpp
mod_socks5_la_LDFLAGS = -no-undefined -module -avoid-version
mod_socks5_la_LIBADD  =

