## Makefile.am -- Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.4 2002-12-08 16:54:09 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

pkglib_LTLIBRARIES = mod_ftp.la
noinst_HEADERS     = *.h

mod_ftp_la_SOURCES = mod_ftp.cpp procpool.cpp client.cpp
mod_ftp_la_LDFLAGS = -no-undefined -module -avoid-version
mod_ftp_la_LIBADD  =

