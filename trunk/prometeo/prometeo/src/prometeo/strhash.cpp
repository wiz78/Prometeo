/***************************************************************************
                                 strhash.cpp
                             -------------------
	revision             : $Id: strhash.cpp,v 1.2 2002-11-18 17:46:47 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : an hash table for string-keyed items
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

#include <ctype.h>

#include "strhash.h"

//---------------------------------------------------------------------------
HashItem::HashItem()
{
	String = NULL;
}
//---------------------------------------------------------------------------
HashItem::~HashItem()
{
	free( String );
}
//---------------------------------------------------------------------------
StrHash::StrHash( unsigned int size )
{
	Size     = size;
	Items    = new LinkedList[ size ];
	NumItems = 0;
}
//---------------------------------------------------------------------------
StrHash::~StrHash()
{
	Clear();

	delete[] Items;
}
//---------------------------------------------------------------------------
void StrHash::Clear( void )
{
	for( int i = 0; i < Size; i++ )
		while( HashItem *item = (HashItem *)Items[ i ].RemTail() )
			delete item;
}
//---------------------------------------------------------------------------
unsigned int StrHash::ComputeHash( const char *str ) const
{
	unsigned int code = 0;

	while( *str )
		code = ( code << 5 ) + tolower( *str++ );

	return( code % Size );
}
//---------------------------------------------------------------------------
HashItem *StrHash::Find( const char *str ) const
{
	unsigned int hash = ComputeHash( str );
	LinkedList	*list = &Items[ hash ];

	for( HashItem *node = (HashItem *)list->GetHead();
		 list->IsNode( node );
		 node = (HashItem *)node->GetSucc() )
		if( node->String && !strcasecmp( node->String, str ))
			return( node );

	return( NULL );
}
//---------------------------------------------------------------------------
bool StrHash::Add( const char *str, void *data )
{
	bool ret;

	if( ret = !Find( str )) {
		unsigned int	hash = ComputeHash( str );
		HashItem		*item = new HashItem();

		item->String = strdup( str );
		item->Data   = data;

		Items[ hash ].AddHead( item );

		NumItems++;
	}

	return( ret );
}
//---------------------------------------------------------------------------
void StrHash::Replace( const char *str, void *data )
{
	unsigned int	hash = ComputeHash( str );
	HashItem		*item = Find( str );

	if( item )
		item->Data = data;
	else
		Add( str, data );
}
//---------------------------------------------------------------------------
void StrHash::Remove( const char *str )
{
	HashItem	*item;

	if( item = Find( str ))
		Remove( item );
}
//---------------------------------------------------------------------------
void StrHash::Remove( HashItem *item )
{
	item->Unlink();

	delete item;

	NumItems--;
}
//---------------------------------------------------------------------------
void *StrHash::FindData( const char *str ) const
{
    HashItem	*item;
    void		*ret = NULL;

    if( item = Find( str ))
        ret = item->Data;

    return( ret );
}
//---------------------------------------------------------------------------
void *StrHash::GetData( int index ) const
{
	void *ret = NULL;

	if( index < NumItems ) {
		int 		n = 0;
		HashItem 	*node = NULL;

		while( index >= 0 ) {
			LinkedList	*list = &Items[ n++ ];

			node = (HashItem *)list->GetHead();

			while(( index >= 0 ) && list->IsNode( node )) {

				if( --index >= 0 )
					node = (HashItem *)node->GetSucc();
			}
		}

		if( node )
			ret = node->Data;
	}

	return( ret );
}
//---------------------------------------------------------------------------
