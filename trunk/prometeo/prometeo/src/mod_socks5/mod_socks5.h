/***************************************************************************
                                mod_socks5.h
                             -------------------
	revision             : $Id: mod_socks5.h,v 1.1 2003-10-23 17:27:17 tellini Exp $
	copyright            : (C) 2003 by Simone Tellini
	email                : tellini@users.sourceforge.net

	description          : SOCKS5 module
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_SOCKS5_H
#define MOD_SOCKS5_H

#include <string>

using namespace std;

#include "main.h"
#include "procpool.h"

class TcpSocket;

namespace mod_socks5
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

using namespace mod_socks5;

#endif
