## Makefile.am -- Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.2 2002-10-13 23:22:08 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

pkglib_LTLIBRARIES = mod_ftp.la

mod_ftp_la_SOURCES = mod_ftp.cpp
mod_ftp_la_LDFLAGS = -no-undefined -module -avoid-version
mod_ftp_la_LIBADD  =

