# $Id: Makefile.am,v 1.1.1.1 2002-10-10 09:59:19 tellini Exp $

INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

noinst_LTLIBRARIES = libprom.la

libprom_la_SOURCES = fd.cpp socket.cpp unixsocket.cpp iodispatcher.cpp buffer.cpp
libprom_la_LDFLAGS = -export-dynamic
libprom_la_LIBADD  =

