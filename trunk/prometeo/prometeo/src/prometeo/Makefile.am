# $Id: Makefile.am,v 1.1 2002-10-10 10:22:59 tellini Exp $

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS				= libprom . include prometeoctl mod_tunnel mod_http \
			 		  mod_cfg mod_ftp

sbin_PROGRAMS		= prometeo

CPPFLAGS 			= @CPPFLAGS@
CXXFLAGS 			= @CXXFLAGS@ -pg

INCLUDES			= -I$(top_srcdir) @INCLTDL@
prometeo_LDFLAGS	= -export-dynamic
prometeo_SOURCES	= main.cpp api.cpp core.cpp registry.cpp logger.cpp \
				      regnode.cpp regkey.cpp reginteger.cpp regstring.cpp regnote.cpp \
					  list.cpp stringlist.cpp mystring.cpp \
					  loader.cpp module.cpp ctrlipc.cpp tcpsocket.cpp \
					  resolver.cpp dnscache.cpp setproctitle.cpp \
					  process.cpp processgroup.cpp \
					  strhash.cpp linkedlist.cpp file.cpp \
					  storage.cpp storeobj.cpp
prometeo_LDADD		= @EXPAT_LIBS@ @LIBLTDL@ ../libprom/libprom.la
prometeo_LDFLAGS	= -export-dynamic

