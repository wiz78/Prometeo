## Makefile.am -- Process this file with automake to produce Makefile.in

# $Id: Makefile.am,v 1.1.1.1 2002-10-10 09:59:30 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

libexec_LTLIBRARIES = mod_ftp.la

mod_ftp_la_SOURCES = mod_ftp.cpp
mod_ftp_la_LDFLAGS = -no-undefined -module -avoid-version
mod_ftp_la_LIBADD  =

