/***************************************************************************
                                  client.h
                             -------------------
    revision             : $Id: client.h,v 1.3 2002-10-17 18:03:14 tellini Exp $
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
#include "bitfield.h"

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
	TcpSocket			*UserData;
	TcpSocket			*ServerData;
	string				Command;
	string				Args;
	BitField			Flags;

	virtual void		WaitRequest( void );

	bool				RecvCommand( void );
	int					RecvStatus( void );

	void				ForwardReply( void );
	bool				ForwardCmd( void );

	virtual void		Dispatch( void );
	void				ConnectToServer( void );
	bool				ServerLogin( const string& user );

	void				CmdFeat( void );
	void				CmdPbsz( void );
	void				CmdProt( void );
	void				CmdPasv( void );
};

#define FTPF_CONNECTED		(1 << 0)
#define FTPF_LOGGED_IN		(1 << 1)
#define FTPF_DATA_TLS		(1 << 2)	// TLS required for data channel
#define FTPF_PBSZ			(1 << 3)
#define FTPF_TLS			(1 << 4)
#define FTPF_CLIENT_PASV	(1 << 5)

#endif
