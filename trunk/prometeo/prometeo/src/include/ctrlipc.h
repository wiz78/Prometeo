/***************************************************************************
                                  ctrlipc.h
                             -------------------
	revision             : $Id: ctrlipc.h,v 1.2 2002-11-13 16:45:58 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : this class handles the administration interface
	                       of the server. Basicly, it parses and dispacthes
						   commands received by prometeoctl
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CTRLIPC_H
#define CTRLIPC_H

#include "main.h"
#include "unixsocket.h"
#include "stringlist.h"

class CtrlIPC : public UnixSocket
{
public:
					CtrlIPC();
					~CtrlIPC();

	void			Cleanup( void );

	void			SocketEvent( Socket *sock, Prom_SC_Reason reason, int data );

private:
	void			NewConnection( UnixSocket *sock );
	bool			CheckAuth( int uid, char *auth );
	void			HandleRequest( UnixSocket *sock );
	bool			ReadLine( UnixSocket *sock, char *req, int maxlen );

	bool			SplitArgs( UnixSocket *sock, char *req, StringList& args );

	void			CmdLoad( UnixSocket *sock, char *req );
	void			CmdUnload( UnixSocket *sock, char *req );
	void			CmdLsMod( UnixSocket *sock );
	void			CmdMod( UnixSocket *sock, char *req );
	void			CmdHelp( UnixSocket *sock );
};

#define PROM_IPC_MAX_REQ_LEN	512

#endif
