/***************************************************************************
                                cacheobj.cpp
                             -------------------
    revision             : $Id: cacheobj.cpp,v 1.3 2002-10-30 14:48:50 tellini Exp $
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

#include "main.h"
#include "cache.h"
#include "cacheobj.h"
#include "storage.h"

//---------------------------------------------------------------------------
static void ErrorCB( StoreObj *obj, Prom_Storage_Reason reason, void *userdata )
{
	((CacheObj *)userdata )->WriteError( reason, obj );
}
//---------------------------------------------------------------------------
CacheObj::CacheObj( Cache *cache )
{
	RefCount = 0;
	CacheMgr = cache;
	ObjFile  = NULL;
	Size     = 0;
}
//---------------------------------------------------------------------------
CacheObj::CacheObj( Cache *cache, StoreObj *sto, const char *name )
{
	RefCount = 1;
	CacheMgr = cache;
	ObjFile  = sto;
	FileName = name;
	Expires  = (time_t)0;
	MIMEType = "text/html";
	Size     = 0;

	LastModified.Set();

	time( &Created );
	time( &LastAccess );

	Flags.Set( PROM_CACHEOBJF_NEW );
	
	sto->SetErrorCallback( ErrorCB, this );
}
//---------------------------------------------------------------------------
CacheObj::~CacheObj()
{
}
//---------------------------------------------------------------------------
void CacheObj::Write( const char *data, int len )
{
	if( Flags.IsSet( PROM_CACHEOBJF_NEW )) {

		ObjFile->Write( data, len );

		// if the object is going to be deleted, the Write above
		// won't actually write anything to disk, thus updating Size
		// would be misleading
		if( !Flags.IsSet( PROM_CACHEOBJF_DELETE ))
			Size += len;
	}
}
//---------------------------------------------------------------------------
void CacheObj::WriteError( Prom_Storage_Reason reason, StoreObj *obj )
{
	bool	ok = false;

	DBG( App->Log->Log( LOG_ERR, "CacheObj::WriteError( %d, - ) errno = %d, %s", 
						reason, errno, strerror( errno )));

	if( reason == PROM_STORE_FULL ) {

		if( CacheMgr->Prune() ) {
			obj->ResumeWrite();
			ok = true;
		}
	}

	if( !ok )
		Delete();
}
//---------------------------------------------------------------------------
bool CacheObj::IsFresh( void )
{
	bool	fresh = true;

	if( IsExpiresValid() ) {
		NetDate	dt;

		dt.Set();

		if( dt >= Expires )
			fresh = false;

	} else if( GetAge() > ( 3600 * 12 ))
		fresh = false;

	return( fresh );
}
//---------------------------------------------------------------------------
StoreObj *CacheObj::GetStoreObj( bool allowopen )
{
	if( !ObjFile ) {
		Storage	*store = CacheMgr->GetStorage();

		// first check whether it's already open
		ObjFile = store->Find( FileName.c_str() );

		// if not, try to open it
		if( !ObjFile && allowopen ) {

			ObjFile = store->Open( FileName.c_str(), false );

			if( ObjFile )
				ObjFile->Load();

		} else if( ObjFile )
			ObjFile->SetErrorCallback( ErrorCB, this );
	}

	return( ObjFile );
}
//---------------------------------------------------------------------------
bool CacheObj::IsComplete( void )
{
	bool	ret = true;

	if( !ObjFile ) {
		StoreObj	*obj;

		if( obj = GetStoreObj( false )) {
				
			if( obj->IsInWriteMode() )
				ret = false;
		
			obj->Release();

			ObjFile = NULL;
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool CacheObj::Release( void )
{
	bool ret;
	
	if( RefCount > 0 )
		RefCount--;

	ret = RefCount <= 0;

	if( ret ) {
			
		if( ObjFile ) {

			if( Flags.IsSet( PROM_CACHEOBJF_DELETE ))
				ObjFile->FlushWriteBuffers();

			ObjFile->Release();

			ObjFile = NULL;
		}

		if( Flags.IsSet( PROM_CACHEOBJF_DELETE ))
			delete this;
	}

	return( ret );
}
//---------------------------------------------------------------------------
int CacheObj::GetAge( void )
{
	int	age;

	age = time( NULL ) - Created;

	if( age < 0 )
		age = 0;

	return( age );
}
//---------------------------------------------------------------------------
void CacheObj::SetMaxAge( int secs )
{
	Expires.Set();

	Expires += secs;
}
//---------------------------------------------------------------------------
void CacheObj::SetLastAccess( void )
{
	time( &LastAccess );
}
//---------------------------------------------------------------------------
void CacheObj::Delete( void )
{
	Acquire();
		
	Flags.Set( PROM_CACHEOBJF_DELETE );

	if( ObjFile )
		ObjFile->StopWriting();

	// this will trigger an immediate deletion if the object wasn't in use
	Release();
}
//---------------------------------------------------------------------------
void CacheObj::LoadFromIdx( StoreObj *sto, int version )
{
	time_t	tm;

	sto->Read( ID );
	sto->Read( FileName );
	sto->Read( URI );
	sto->Read( MIMEType );
	sto->Read( ETag );

	sto->Read( &Size,         sizeof( Size         ));
	sto->Read( &Created,      sizeof( Created      ));
	sto->Read( &LastAccess,   sizeof( LastAccess   ));

	sto->Read( &tm, sizeof( tm ));
	Expires = tm;

	sto->Read( &tm, sizeof( tm ));
	LastModified = tm;
}
//---------------------------------------------------------------------------
void CacheObj::SaveToIdx( StoreObj *sto )
{
	time_t	tm;

	sto->Write( ID );
	sto->Write( FileName );
	sto->Write( URI );
	sto->Write( MIMEType );
	sto->Write( ETag );

	sto->Write( &Size,         sizeof( Size         ));
	sto->Write( &Created,      sizeof( Created      ));
	sto->Write( &LastAccess,   sizeof( LastAccess   ));

	tm = Expires.ToEpoch();
	sto->Write( &tm, sizeof( tm ));

	tm = LastModified.ToEpoch();
	sto->Write( &tm, sizeof( tm ));
}
//---------------------------------------------------------------------------
