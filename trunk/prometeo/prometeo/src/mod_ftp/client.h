/***************************************************************************
                                  client.h
                             -------------------
    revision             : $Id: client.h,v 1.2 2002-10-15 13:03:42 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles an FTP user connection
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

using namespace std;

#include <string>

#include "process.h"

class TcpSocket;

class Client : public Process
{
public:
						Client();
						~Client();

	virtual void		OnFork( void );

	void				Serve( TcpSocket *sock, bool forked );

private:
	TcpSocket			*User;
	TcpSocket			*Server;
	string				Command;
	string				Args;

	virtual void		WaitRequest( void );

	bool				RecvCommand( void );

	virtual void		Dispatch( void );
	void				ConnectToServer( void );
};

#endif
