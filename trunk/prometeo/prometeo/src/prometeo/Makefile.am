# $Id: Makefile.am,v 1.4 2002-11-03 17:28:46 tellini Exp $

MAINTAINERCLEANFILES = Makefile.in

sbin_PROGRAMS		= prometeo

CPPFLAGS 			= @CPPFLAGS@
CXXFLAGS 			= @CXXFLAGS@ -pg

INCLUDES			= -I$(top_srcdir) @INCLTDL@

prometeo_SOURCES	= main.cpp api.cpp core.cpp registry.cpp logger.cpp \
				      regnode.cpp regkey.cpp reginteger.cpp regstring.cpp regnote.cpp \
					  list.cpp stringlist.cpp mystring.cpp \
					  loader.cpp module.cpp ctrlipc.cpp \
					  resolver.cpp dnscache.cpp setproctitle.cpp \
					  acl.cpp process.cpp processgroup.cpp \
					  strhash.cpp linkedlist.cpp file.cpp \
					  storage.cpp storeobj.cpp

prometeo_LDADD		= @EXPAT_LIBS@ @LIBLTDL@ ../libprom/libprom.la
prometeo_LDFLAGS	= -export-dynamic

