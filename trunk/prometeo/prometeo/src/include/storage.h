/***************************************************************************
                                  storage.h
                             -------------------
    revision             : $Id: storage.h,v 1.1.1.1 2002-10-10 09:59:17 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles a disk-based storage
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_STORAGE_H
#define PROM_STORAGE_H

#include <string>

#include "linkedlist.h"
#include "strhash.h"

class StoreObj;

class Storage
{
public:
							Storage( const char *cacheid );
							~Storage();

	StoreObj				*Open( const char *name, bool create = false, bool async = true );
	void					Close( StoreObj *obj );

	StoreObj				*Find( const char *name ); // searches among open files

	void					Delete( const char *name );

	void					Flush( void );

	void					UpdateSize( long len ) { Size += len; }
	unsigned long long		GetSize( void ) const { return( Size ); }

private:
	string					Directory;
	LinkedList				OpenObjs;
	StrHash					OpenHash;
	unsigned long long		Size;
};

#endif
