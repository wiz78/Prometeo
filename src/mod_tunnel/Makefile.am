## Makefile.am -- Process this file with automake to produce Makefile.in
INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

libexec_LTLIBRARIES = mod_tunnel.la

mod_tunnel_la_SOURCES = mod_tunnel.cpp
mod_tunnel_la_LDFLAGS = -no-undefined -module -avoid-version
mod_tunnel_la_LIBADD  =

