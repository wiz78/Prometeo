/***************************************************************************
                                 mod_cfg.h
                             -------------------
	revision             : $Id: mod_cfg.h,v 1.1.1.1 2002-10-10 09:59:27 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : configuration editor via web
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_CFG_H
#define MOD_CFG_H

#define MOD_VERSION	"1.0"

#include <string>

#include "api.h"
#include "linkedlist.h"

class TcpSocket;

class CfgEditor
{
public:
					CfgEditor( const char *key );

	bool			Cleanup();
	void			ReloadCfg( void );
	
	void			Accept( TcpSocket *sock );

private:
	string			Key;
	short			Port;
	LinkedList		Sockets;
	TcpSocket		*ListeningSocket;

	void			Setup( void );
};

#endif
