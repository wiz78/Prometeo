/***************************************************************************
                                  filter.h
                             -------------------
    revision             : $Id: filter.h,v 1.1 2002-11-21 18:39:42 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : filter URL's
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILTER_H
#define FILTER_H

using namespace std;

#include <string>

#include "list.h"

class Filter
{
public:
	typedef enum { F_NOOP, F_DONT_CACHE, F_FORBID, F_REDIRECT } ActionType;

					Filter( const char *regkey );
					~Filter();

	bool			Match( const char *url );

	ActionType		GetAction( void ) const { return( Action ); }
	string&			GetTarget( void )       { return( Target ); }

private:
	ActionType		Action;
	List			URLs;
	string			Target;

	void			PrepareURLs( string urls );
};

#endif
