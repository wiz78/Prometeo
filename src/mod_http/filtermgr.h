/***************************************************************************
                                filtermgr.h
                             -------------------
    revision             : $Id: filtermgr.h,v 1.1 2002-11-21 18:39:42 tellini Exp $
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

#ifndef FILTERMGR_H
#define FILTERMGR_H

using namespace std;

#include <string>

#include "list.h"

class Filter;

class FilterMgr
{
public:
				FilterMgr( const string key );
				~FilterMgr();

	void		Load( void );

	Filter		*Match( const char *url );

private:
	string		Key;
	List		Filters;

	void		Clear();
};

#endif
