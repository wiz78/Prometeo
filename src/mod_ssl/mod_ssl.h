/***************************************************************************
                                 mod_ssl.h
                             -------------------
    revision             : $Id: mod_ssl.h,v 1.1 2002-11-09 18:25:12 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : simple TCP tunnel with SSL transport
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_SSL_H
#define MOD_SSL_H

#include <string>

using namespace std;

#include "main.h"
#include "procpool.h"

class TcpSocket;

class SSLProxy
{
public:
					SSLProxy( const char *key );

	bool			Cleanup( void );
	void			ReloadCfg( void );

	void			Accept( TcpSocket *sock );

	void			OnFork( void );
	void			OnTimer( time_t now );

private:
	string			Key;
	short			Port;
	TcpSocket		*ListeningSocket;
	SSLProcPool		Children;

	void			Setup( void );
};

#endif
