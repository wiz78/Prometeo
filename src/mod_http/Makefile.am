## Makefile.am -- Process this file with automake to produce Makefile.in
INCLUDES        = -I$(top_builddir) -I$(top_srcdir) \
				  -I$(top_builddir)/src -I$(top_srcdir)/src \
				  -I$(top_builddir)/src/include -I$(top_srcdir)/src/include

CPPFLAGS = @CPPFLAGS@
CXXFLAGS = @CXXFLAGS@

pkglib_LTLIBRARIES = mod_http.la

mod_http_la_SOURCES = mod_http.cpp url.cpp connectionmgr.cpp netdate.cpp \
					  http.cpp md5.c cache.cpp cacheobj.cpp \
					  gzipcodec.cpp hostmapper.cpp filter.cpp \
					  filtermgr.cpp
mod_http_la_LDFLAGS = -no-undefined -module -avoid-version
mod_http_la_LIBADD  =

