/***************************************************************************
                                 mod_pop3.h
                             -------------------
	revision             : $Id: mod_pop3.h,v 1.1 2003-05-24 12:28:53 tellini Exp $
	copyright            : (C) 2003 by Simone Tellini
	email                : tellini@users.sourceforge.net

	description          : POP3 proxy
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_POP3_H
#define MOD_POP3_H

#include <string>

using namespace std;

#include "main.h"
#include "procpool.h"

class TcpSocket;

namespace mod_pop3
{

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

}; // namespace

using namespace mod_pop3;

#endif
