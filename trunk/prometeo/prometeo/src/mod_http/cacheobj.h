/***************************************************************************
                                 cacheobj.h
                             -------------------
    revision             : $Id: cacheobj.h,v 1.1.1.1 2002-10-10 09:59:33 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class holds the info about cached objects
                           metadata is kept in memory, while the actual
                           object content is loaded from disk on request
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CACHEOBJ_H
#define CACHEOBJ_H

#include <string>
#include <sys/types.h>

#include "storeobj.h"
#include "netdate.h"
#include "bitfield.h"
#include "linkedlist.h"

#include "main.h"

#define PROM_CACHEOBJF_DELETE		(1 << 0)
#define PROM_CACHEOBJF_NEW			(1 << 1)

class Cache;

class CacheObj : public LinkedListNode
{
public:
					CacheObj( Cache *cache );
					CacheObj( Cache *cache, StoreObj *sto, const char *name );
					~CacheObj();

	void			Write( const char *data, int len );
	void			WriteError( Prom_Storage_Reason reason, StoreObj *obj );

	bool			IsFresh( void );

	void			SetID( const char *id )              { ID = id; }
	void			SetURI( const char *uri )            { URI = uri; }
	void			SetMIMEType( const char *mime )      { MIMEType = mime; }
	void			SetLastModified( const NetDate& dt ) { LastModified = dt; }
	void			SetExpires( const NetDate& dt )      { Expires = dt; }
	void			SetETag( const char *tag )           { ETag = tag; }
	void			SetCreated( time_t dt )              { Created = dt; }
	void			SetMaxAge( int secs );
	void			SetLastAccess( void );

	const char		*GetFileName( void ) const { return( FileName.c_str() ); }
	const char		*GetID( void ) const       { return( ID.c_str() ); }
	const char		*GetURI( void ) const      { return( URI.c_str() ); }
	const char		*GetMIMEType( void ) const { return( MIMEType.c_str() ); }
	const char		*GetETag( void ) const     { return( ETag.c_str() ); }
	NetDate&		GetLastModified( void )    { return( LastModified ); }
	NetDate&		GetExpires( void )         { return( Expires ); }
	unsigned int	GetSize( void ) const      { return( Size ); }
	int				GetAge( void );

	bool			IsExpiresValid( void )    { return( Expires > NetDate( 0 )); }
	bool			IsInUse( void ) const     { return( RefCount > 0 ); }
	bool			IsComplete( void );
	bool			IsDeleted( void ) const   { return( Flags.IsSet( PROM_CACHEOBJF_DELETE )); }

	// allowopen = false -> it doesn't try to open the object if it isn't already
	StoreObj		*GetStoreObj( bool allowopen = true );

	void			Acquire( void ) { RefCount++; }
	bool			Release( void );

	void			Delete( void );

	void			LoadFromIdx( StoreObj *sto, int version );
	void			SaveToIdx( StoreObj *sto );

private:
	unsigned int	RefCount;
	Cache			*CacheMgr;
	StoreObj		*ObjFile;
	string			FileName;
	string			ID;
	string			URI;
	string			MIMEType;
	unsigned int	Size;
	NetDate			LastModified;
	NetDate			Expires;
	time_t			LastAccess;
	time_t			Created;
	string			ETag;
	BitField		Flags;
};

#endif
