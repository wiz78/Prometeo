/***************************************************************************
                                option_bool.h
                             -------------------
    revision             : $Id: option_bool.h,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : boolean option class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OPTION_BOOL_H
#define OPTION_BOOL_H

#include "option_integer.h"

class BoolOption : public IntegerOption
{
public:
					BoolOption( const string page );

	virtual void	Render( string& result );
};

#endif
