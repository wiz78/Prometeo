/***************************************************************************
                                 RegKey.cpp
                             -------------------
	revision             : $Id: regkey.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
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

#include <string.h>
#include <fstream>

#include "regkey.h"

//--------------------------------------------------------------------------
RegKey::RegKey( const char *name ) : RegNode( name, REG_KEY )
{
}
//--------------------------------------------------------------------------
RegKey::~RegKey()
{
	for( int i = 0; i < Children.Count(); i++ )
		delete (RegNode *)Children[ i ];

	if( Parent )
		Parent->RemoveChild( this );
}
//--------------------------------------------------------------------------
void RegKey::AddChild( RegNode *key )
{
	Children.Add( key );
	key->SetParent( this );
}
//--------------------------------------------------------------------------
void RegKey::RemoveChild( RegNode *key )
{
	Children.Remove( key );
}
//--------------------------------------------------------------------------
RegKey *RegKey::FindKey( const char *path )
{
	char	*next = strchr( path, '/' );
	RegKey	*key = NULL;
	int		len;

	if( next )
		len = next - path;
	else
		len = strlen( path );

	for( int i = 0; !key && ( i < Children.Count() ); i++ ) {
		RegKey	*k = (RegKey *)Children[ i ];

		if(( k->GetType() == REG_KEY ) && !strncmp( path, k->GetName(), len ))
			key = k;
	}

	if( next && key ) // we should recurse
		key = key->FindKey( next + 1 );

	return( key );
}
//--------------------------------------------------------------------------
RegKey *RegKey::GetKey( int index )
{
	RegKey	*key = NULL;
	int		keynum = 0;

	for( int i = 0; !key && ( i < Children.Count() ); i++ ) {
		RegKey	*k = (RegKey *)Children[ i ];

		if(( k->GetType() == REG_KEY ) && ( keynum++ == index ))
			key = k;
	}

	return( key );
}
//--------------------------------------------------------------------------
RegNode *RegKey::FindValue( const char *name, RegType type )
{
	RegNode	*key = NULL;

	for( int i = 0; !key && ( i < Children.Count() ); i++ ) {
		RegNode	*k = (RegNode *)Children[ i ];

		if(( k->GetType() == type ) && !strcmp( name, k->GetName() ))
			key = k;
	}

	return( key );
}
//--------------------------------------------------------------------------
RegNode *RegKey::GetValue( int index )
{
	RegNode	*key = NULL;
	int		num = 0;

	for( int i = 0; !key && ( i < Children.Count() ); i++ ) {
		RegKey	*k = (RegKey *)Children[ i ];

		if((( k->GetType() == REG_INTEGER ) || ( k->GetType() == REG_STRING )) &&
		   ( num++ == index ))
			key = k;
	}

	return( key );
}
//--------------------------------------------------------------------------
void RegKey::Save( ofstream& fh, int indent )
{
	WriteTabs( fh, indent++ );

	fh << "<Key name=\"" << Name << "\">" << endl;

	for( int i = 0; i < Children.Count(); i++ )
		((RegNode *)Children[ i ])->Save( fh, indent );

	WriteTabs( fh, indent - 1 );

	fh << "</Key>" << endl << endl;
}
//--------------------------------------------------------------------------
