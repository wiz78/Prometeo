/***************************************************************************
                                 mod_ftp.h
                             -------------------
	revision             : $Id: mod_ftp.h,v 1.1 2002-10-17 18:06:35 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : FTP proxy with TSL support
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_FTP_H
#define MOD_FTP_H

#include <string>

using namespace std;

#include "main.h"
#include "procpool.h"

class TcpSocket;

class Proxy
{
public:
					Proxy( const char *key );

	bool			Cleanup( void );
	void			ReloadCfg( void );

	void			Accept( TcpSocket *sock );

	void			OnFork( void );
	void			OnTimer( time_t now );

private:
	string			Key;
	short			Port;
	TcpSocket		*ListeningSocket;
	ProcPool		Children;

	void			Setup( void );
};

#endif
