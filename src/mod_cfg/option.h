/***************************************************************************
                                  option.h
                             -------------------
    revision             : $Id: option.h,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : base option class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OPTION_H
#define OPTION_H

using namespace std;

#include <string>
#include <sablot.h>
#include <sdom.h>

#include "list.h"

#include "misc.h"

class Option
{
public:
	typedef enum { T_INTEGER, T_STRING, T_TEXT, T_BOOL, T_LIST, T_SELECT } OptionType;

					Option( OptionType type, const string page, const string path = "" );

	virtual void	Render( string& result ) = 0;
	virtual void	ReadParams( SablotSituation sit, SDOM_Node ctx );

	static void		GetOptionList( SablotSituation sit, SDOM_Node ctx,
								   const string query, List& Options, const string page );

	void			SetName( const string str );
	void			SetKey( const string str )     { Key = str; }
	void			SetLabel( const string str )   { Label = str; }
	void			SetDescr( const string str )   { Descr = str; }
	void			SetDefault( const string str ) { Default = str; }
	void			SetEditable( bool edit )       { Editable = edit; }
	void			SetVisible( bool show )        { Visible = show; }

	OptionType		GetType( void ) const    { return( Type ); }
	string&			GetPage( void )          { return( Page ); }
	string&			GetName( void )          { return( Name ); }
	string&			GetPath( void )          { return( Path ); }
	string&			GetDescr( void )         { return( Descr ); }
	string&			GetLabel( void )         { return( Label ); }
	string&			GetKey( void )           { return( Key ); }
	string&			GetDefault( void )       { return( Default ); }
	bool			IsEditable( void ) const { return( Editable ); }
	bool			IsVisible( void ) const  { return( Visible ); }

protected:
	OptionType		Type;
	string			Page;
	string			Path;
	string			Name;
	string			Key;
	string			Label;
	string			Descr;
	string			Default;

private:
	bool			Editable;
	bool			Visible;
};

#endif
