/***************************************************************************
                                 procpool.h
                             -------------------
    revision             : $Id: procpool.h,v 1.1 2003-05-24 12:28:53 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : handles the pool of processes which serve the
                           clients
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROCPOOL_H
#define PROCPOOL_H

using namespace std;

#include <string>

#include "processgroup.h"

class TcpSocket;
class IODispatcher;

namespace mod_pop3
{

class ProcPool : public ProcessGroup
{
public:
					ProcPool( string key, IODispatcher *io );

	void			Setup();
	void			OnFork();

	void			ServeClient( TcpSocket *sock );

	bool			AreIdle( void );

private:
	string			Key;
	int				MaxChildren;
};

}; // namespace

using namespace mod_pop3;

#endif
