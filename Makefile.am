## Makefile.am -- Process this file with automake to produce Makefile.in
## $Id: Makefile.am,v 1.3 2002-12-06 17:06:36 tellini Exp $

auxdir			= @ac_aux_dir@
AUX_DIST		= $(auxdir)/install-sh $(auxdir)/missing \
			      $(auxdir)/mkinstalldirs
AUX_DIST_EXTRA	= $(auxdir)/expat.m4

EXTRA_DIST			= bootstrap
AUTOMAKE_OPTIONS 	= foreign

SUBDIRS 		= libltdl src

MAINTAINERCLEANFILES 	= Makefile.in aclocal.m4 configure src/include/config-h.in \
						  $(AUX_DIST)

sysconf_DATA	= prometeo.xml
ACLOCAL			= aclocal -I $(auxdir)
LIBTOOL_DEPS	= @LIBTOOL_DEPS@

libtool: $(LIBTOOL_DEPS)
	        $(SHELL) ./config.status --recheck

dist-hook:
	(cd $(distdir) && mkdir $(auxdir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  cp $$file $(distdir)/$$file; \
	done
