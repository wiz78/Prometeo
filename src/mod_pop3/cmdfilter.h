/***************************************************************************
                                 cmdfilter.h
                             -------------------
    revision             : $Id: cmdfilter.h,v 1.1 2004-04-24 13:51:49 tellini Exp $
    copyright            : (C) 2003-2004 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : class to filter messages through external programs
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CMDFILTER_H
#define CMDFILTER_H

using namespace std;

#include <string>

#include "filter.h"

namespace mod_pop3
{

	class CmdFilter : public Filter
	{
	public:
							CmdFilter( const string& cmd );

		virtual bool		Process( const string& msg );

	private:
		string				Cmd;

		bool				Parent( const string &msg, int wfds[2], int rfds[2] );
		void				Child( int wfds[2], int rfds[2] );
	};

}; // namespace

using namespace mod_pop3;

#endif
