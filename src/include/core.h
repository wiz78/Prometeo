/***************************************************************************
                                   core.h
                             -------------------
	revision             : $Id: core.h,v 1.3 2002-11-03 17:28:46 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CORE_H
#define CORE_H

#include <signal.h>

class Registry;
class Logger;
class CtrlIPC;
class Loader;
class DNSCache;
class IODispatcher;
class Acl;

class Core
{
public:
	Registry		*Cfg;
	Logger			*Log;
	IODispatcher	*IO;
	DNSCache		*DNS;
	Loader			*Mods;
	Acl				*ACL;

					Core();
					~Core();

	void 			Run( void );
	void			Quit( void ) { Running = false; }

	void			CfgReload( void );

	/*
	 * If you need to fork, you need this function.
	 *
	 * Do NOT use fork() directly, as it can lead to problems. For instance,
	 * listening sockets are inherited by the child, causing the parent to fail
	 * if it ever needs to close them and reopen them before the child exits
	 * (it'll get an EADDRINUSE error).
	 *
	 * The function cares to destroy some internal objects which shouldn't
	 * be accessed by children (such as the controller ipc object) and free
	 * some resources.
	 */
	pid_t			Fork( char *ident );

	bool			InitWait( void );
	void			Wait( void );
	void			WakeUpParent( void );

private:
	CtrlIPC			*IPC;
	bool			Running;
	sigset_t		NewMask;
	sigset_t		OldMask;
	sigset_t		ZeroMask;

	void			MainLoop( void );
	bool			Daemonize( void );
	bool			CheckPid( void );
};

#endif
