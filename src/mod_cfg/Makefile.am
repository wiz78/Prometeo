## Makefile.am -- Process this file with automake to produce Makefile.in
# $Id: Makefile.am,v 1.2 2002-10-13 23:22:08 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

SUBDIRS = . data

pkglib_LTLIBRARIES = mod_cfg.la

mod_cfg_la_SOURCES = mod_cfg.cpp cfgdata.cpp base64.cpp pagemaker.cpp
mod_cfg_la_LDFLAGS = -no-undefined -module -avoid-version
mod_cfg_la_LIBADD  = -lsablot 

