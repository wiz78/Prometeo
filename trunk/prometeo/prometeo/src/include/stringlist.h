/***************************************************************************
                                stringlist.h
                             -------------------
    revision             : $Id: stringlist.h,v 1.2 2002-10-29 18:01:14 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : a simple string list
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STRINGLIST_H
#define STRINGLIST_H

#include <string>

#include "list.h"

class StringList : public List
{
public:
						StringList() : List() {}
						~StringList();

	void				Add( const string& str );
	void				Add( const char *fmt, ... );

	char				*Get( int index ) const { return((char *)List::Get( index )); }

	char				*operator[]( int index ) const { return( Get( index )); }

	virtual void		Clear( void );

	// splits the string with 'separator'
	void				Explode( const string& str, const char *separator );
};

#endif
