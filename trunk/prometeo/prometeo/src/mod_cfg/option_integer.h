/***************************************************************************
                               option_integer.h
                             -------------------
    revision             : $Id: option_integer.h,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : integer option class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OPTION_INTEGER_H
#define OPTION_INTEGER_H

#include "option.h"

class IntegerOption : public Option
{
public:
					IntegerOption( const string page );

	virtual void	Render( string& result );

protected:
	int				GetValue( void );
};

#endif
