/***************************************************************************
                               option_select.h
                             -------------------
    revision             : $Id: option_select.h,v 1.2 2002-11-21 18:36:55 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : select option class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OPTION_SELECT_H
#define OPTION_SELECT_H

#include "option_string.h"

class SelectItem
{
public:
	string	Value;
	string	Label;
};

class SelectOption : public StringOption
{
public:
					SelectOption( const string page );
					~SelectOption();

	virtual void	Render( string& result );
	virtual void	ReadParams( SablotSituation sit, SDOM_Node ctx );

	string			GetValueLabel( string value );

private:
	SelectItem		*Items;
	int				NumItems;
};

#endif
