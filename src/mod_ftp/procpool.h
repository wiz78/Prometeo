/***************************************************************************
                                 procpool.h
                             -------------------
    revision             : $Id: procpool.h,v 1.1 2002-10-14 19:36:17 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : handles the pool of processes which serve the ftp
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

class ProcPool : public ProcessGroup
{
public:
					ProcPool( string key );

	void			Setup();
	void			OnFork();

	void			ServeClient( TcpSocket *sock );

	bool			AreIdle( void );

private:
	string			Key;
	int				MaxChildren;
};

#endif
