## Makefile.am -- Process this file with automake to produce Makefile.in
# $Id: Makefile.am,v 1.4 2002-12-08 16:54:09 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

SUBDIRS = . data

pkglib_LTLIBRARIES = mod_cfg.la
noinst_HEADERS     = *.h

mod_cfg_la_SOURCES = mod_cfg.cpp cfgdata.cpp base64.cpp pagemaker.cpp misc.cpp \
					 option.cpp option_string.cpp option_text.cpp option_bool.cpp \
					 option_integer.cpp option_list.cpp option_select.cpp
mod_cfg_la_LDFLAGS = -no-undefined -module -avoid-version
mod_cfg_la_LIBADD  = -lsablot 

