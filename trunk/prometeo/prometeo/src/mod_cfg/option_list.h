/***************************************************************************
                                option_list.h
                             -------------------
    revision             : $Id: option_list.h,v 1.1 2002-11-20 22:53:43 tellini Exp $
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

#ifndef OPTION_LIST_H
#define OPTION_LIST_H

#include "option.h"
#include "list.h"

class ListOption : public Option
{
public:
					ListOption( const string page );

	virtual void	Render( string& result );
	virtual void	ReadParams( SablotSituation sit, SDOM_Node ctx );

	void			PrintListTable( string& result );

	string&			GetKeyName( void )    { return( KeyName ); }
	string&			GetKeyLabel( void )   { return( KeyLabel ); }
	string&			GetKeyDescr( void )   { return( KeyDescr ); }

	Option			*GetField( int i );

private:
	string			KeyName;
	string			KeyLabel;
	string			KeyDescr;
	List			Fields;

	int				AddListHeaders( string& result );
	void			AddListRow( const char *item, string& result );
};

#endif
