/***************************************************************************
                                  filter.h
                             -------------------
    revision             : $Id: filter.h,v 1.1 2004-04-24 13:51:49 tellini Exp $
    copyright            : (C) 2003-2004 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : class to filter messages
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

namespace mod_pop3
{

	class Filter
	{
	public:

		virtual bool		Process( const string& msg ) = 0;

		string				GetFilteredMsg( void )	{ return( FilteredMsg ); }
		void				Clean( void )			{ FilteredMsg = ""; }

	protected:
		string				FilteredMsg;
	};

}; // namespace

using namespace mod_pop3;

#endif
