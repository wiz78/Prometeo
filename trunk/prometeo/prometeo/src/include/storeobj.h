/***************************************************************************
                                 storeobj.h
                             -------------------
    revision             : $Id: storeobj.h,v 1.1.1.1 2002-10-10 09:59:17 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles a disk-based storage object
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_STOREOBJ_H
#define PROM_STOREOBJ_H

#include <string>

#include "file.h"
#include "buffer.h"
#include "linkedlist.h"

class StoreObj;

typedef enum {
					PROM_STORE_ERROR	= 1,	// generic error
					PROM_STORE_FULL		= 2,	// storage full, make room and retry
			 } Prom_Storage_Reason;

typedef void ( *Prom_Storage_Callback )( StoreObj *obj, Prom_Storage_Reason reason, void *userdata );
typedef void ( *Prom_Listener )( void *userdata, char *data, int len );

#define PROM_STOREF_CLOSING		(1 << 0)
#define PROM_STOREF_READING		(1 << 1)
#define PROM_STOREF_DONT_WRITE	(1 << 2)

class Storage;

class StoreObj : public LinkedListNode, protected File
{
public:
							StoreObj( Storage *store, const char *name, bool create, bool async );
							~StoreObj();

	bool					Close( void ) { return( File::Close() ); }

	// load the whole object into memory
	// notifiea the listeners as soon as partial data is ready
	void					Load( void );

	void					Write( const void *data, int len );
	void					ResumeWrite( void ) { File::ResumeWrite(); }
	void					WriteComplete( bool ok = true );
	void					StopWriting( void ) { StoreObj::Flags.Set( PROM_STOREF_DONT_WRITE ); }

	int						Read( void *data, int len ) { return( File::Read( data, len )); }

	void					Write( const string& str );
	void					Read( string& str ) { File::Read( str ); }

	void					FlushWriteBuffers( void ) { File::FlushWriteBuffers( true ); }

	void					Acquire( void ) { RefCount++; }
	bool					Release( void ); // returns true if the obj is deleted

	void					SetErrorCallback( Prom_Storage_Callback callback, void *userdata );

	// listeners are notified whenever new data is ready, so they
	// can send it to clients or process it
	void					AddListener( Prom_Listener callback, void *userdata );
	void					RemListener( Prom_Listener callback, void *userdata );

	// get information about the data currently in memory
	// note that it couldn't be the whole object - check IsReading()
	char					*GetData( void ) { return( Data.GetData() ); }
	unsigned int			GetSize( void ) { return( AvailSize ); }

	void					SetClosing( void ) { StoreObj::Flags.Set( PROM_STOREF_CLOSING ); }

	const char				*GetName( void ) const { return( Name.c_str() ); }

	// is the object still loading?
	bool					IsReading( void ) const { return( StoreObj::Flags.IsSet( PROM_STOREF_READING )); }
	bool					IsClosing( void ) const { return( StoreObj::Flags.IsSet( PROM_STOREF_CLOSING )); }
	bool					IsValid( void ) const { return( File::IsValid() ); }
	bool					IsInWriteMode( void ) const { return( File::Flags.IsSet( PROM_FILEF_WRITE )); }

private:
	Storage					*Store;
	string					Name;
	unsigned int			RefCount;
	Buffer					Data;
	unsigned int			AvailSize;
	unsigned int			ReadReq;
	Prom_Storage_Callback	ErrorCallback;
	void					*ErrorUserdata;
	BitField				Flags;
	LinkedList				Listeners;

	virtual void			Callback( Prom_FC_Reason reason, int data );

	void					NotifyListeners( char *data, int len );
};

class Listener : public LinkedListNode
{
public:
	Prom_Listener	Callback;
	void			*Userdata;
};

#endif
