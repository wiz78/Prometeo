/***************************************************************************
                                spamdfilter.h
                             -------------------
    revision             : $Id: spamdfilter.h,v 1.1 2004-04-24 13:51:49 tellini Exp $
    copyright            : (C) 2003-2004 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : class to filter messages through SpamAssassin
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SPAMDFILTER_H
#define SPAMDFILTER_H

using namespace std;

#include <string>

#include "filter.h"

namespace mod_pop3
{

	class SpamdFilter : public Filter
	{
	public:
							SpamdFilter( const string host, int port );

		virtual bool		Process( const string& msg );

	private:
		string				SpamdHost;
		int					SpamdPort;
	};

}; // namespace

using namespace mod_pop3;

#endif
