/***************************************************************************
                                 registry.h
                             -------------------
	revision             : $Id: registry.h,v 1.4 2003-12-29 22:25:39 tellini Exp $
    copyright            : (C) 2002-2004 by Simone Tellini
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

#ifndef REGISTRY_H
#define REGISTRY_H

#include <string>

using namespace std;

#include "regkey.h"
#include "regstring.h"
#include "reginteger.h"
#include "list.h"

class Core;

class Registry
{
public:
					Registry( Core *app );
					~Registry();

	void			Load( void );
	void			Save( void );

	const char		*GetString( const char *name, const char *def );
	void			SetString( const char *name, const char *value );

	int				GetInteger( const char *name, int def );
	void			SetInteger( const char *name, int value );

	bool			OpenKey( const char *path, bool create );
	void			CloseKey( void );

	void			DeleteKey( const char *path );
	void			DeleteValue( const char *name );

	// enumerates the keys contained in the current context
	// returns NULL when index is out of bounds
	char			*EnumKeys( int index );
	// enumerates the values contained in the current open key
	// returns NULL when index is out of bounds
	char			*EnumValues( int index );

	// the following three functions should be considered private
	// they're the callback entry points used by expat
	void 			StartElement( char *name, char **atts );
	void 			EndElement( const char *name );
	void			CharHandler( char *str, int len );

private:
	Core			*App;
	RegKey			*Tree;
	RegKey			*CurrentKey;
	RegNode			*CurrentValue;
	string			Buffer;
	void			*Parser;
	bool			Parsing;
	List			OpenKeysStack;

	void			SetupParser( void );
	void			Clear( void );
	RegKey			*FindKey( const char *path );
	void 			CheckFilePerms( void );
};

#endif
