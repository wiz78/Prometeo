/***************************************************************************
                                  mystring.h
                             -------------------
	revision             : $Id: mystring.h,v 1.1.1.1 2002-10-10 09:59:14 tellini Exp $
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

#ifndef MYSTRING_H
#define MYSTRING_H

#include <string>

#include "stringlist.h"

class MyString : public string
{
public:
					MyString() : string() {}
					MyString( const char *str ) : string( str ) {}

	int				ToInt( void ) const;

	// splits the string with 'separator', putting the resulting
	// pieces into 'list'
	void			Explode( const char *separator, StringList& list );

private:
};

#endif
