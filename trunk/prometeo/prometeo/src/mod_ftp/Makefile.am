## Makefile.am -- Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.3 2002-10-14 19:36:17 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

pkglib_LTLIBRARIES = mod_ftp.la

mod_ftp_la_SOURCES = mod_ftp.cpp procpool.cpp client.cpp
mod_ftp_la_LDFLAGS = -no-undefined -module -avoid-version
mod_ftp_la_LIBADD  =

