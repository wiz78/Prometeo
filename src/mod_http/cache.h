/***************************************************************************
                                  cache.h
                             -------------------
    revision             : $Id: cache.h,v 1.2 2002-10-22 14:31:27 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles the disk-based cache for
                           mod_http
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CACHE_H
#define CACHE_H

#include "storage.h"
#include "strhash.h"
#include "linkedlist.h"

#include "cacheobj.h"

class Cache
{
public:
						Cache();
						~Cache();

	CacheObj			*Create( const char *id );
	CacheObj			*Find( const char *id );

	void				Delete( const char *id );
	void				Delete( CacheObj *obj );

	bool				Prune( void );

	void				Flush( void ) { Store.Flush(); }

	void				LoadIndex( void );
	void				WriteIndex( void );

	Storage				*GetStorage( void ) { return( &Store ); }

	void				SetMaxSize( unsigned long size ) { MaxSize = size * 1024 * 1024; }
	unsigned long		GetMaxSize( void ) const { return( MaxSize / ( 1024 * 1024 )); }

private:
	Storage				Store;
	StrHash				Hash;
	LinkedList			Objects;
	unsigned long long	MaxSize;
	bool				Modified;
};

#endif

