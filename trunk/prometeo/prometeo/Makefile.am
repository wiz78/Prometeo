## Makefile.am -- Process this file with automake to produce Makefile.in
## $Id: Makefile.am,v 1.6 2003-05-24 12:44:50 tellini Exp $

auxdir			= @ac_aux_dir@
AUX_DIST		= $(auxdir)/install-sh $(auxdir)/missing \
			      $(auxdir)/mkinstalldirs
AUX_DIST_EXTRA	= $(auxdir)/expat.m4

EXTRA_DIST			= bootstrap
AUTOMAKE_OPTIONS 	= foreign

SUBDIRS 			= libltdl src

MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure src/include/config-h.in \
						  $(AUX_DIST)

sysconf_DATA	= prometeo.xml.sample
ACLOCAL		= aclocal -I $(auxdir)
LIBTOOL_DEPS	= @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	        $(SHELL) ./config.status --recheck

dist-hook:
	(cd $(distdir) && mkdir -p $(auxdir))
	
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  cp $$file $(distdir)/$$file; \
	done

	cp prometeo.xml.sample $(distdir)

	(cd $(distdir) && mkdir -p docs)

	cp docs/*.txt $(distdir)/docs
