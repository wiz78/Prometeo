/***************************************************************************
                                 loader.cpp
                             -------------------
	revision             : $Id: loader.h,v 1.1.1.1 2002-10-10 09:59:12 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : this class contains everything is needed to
	                       load/unload an external module
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LOADER_H
#define LOADER_H

#include "api.h"
#include "list.h"

#include <string>

class Loader
{
public:
				Loader();
				~Loader();

	void		LoadModules( void );

	bool		Load( const char *modname, bool forceload );
	void		Unload( const char *modname );

	void		CfgChanged( void );

	void		OnFork( void );
	void		OnTimer( time_t now );

	const char	*EnumNames( int index );
	bool		IsRunning( const char *mod );

	const char	*GetManifest( const char *mod );

private:
	List		Modules;
	string		Manifest;

	const char	*IsValid( Prom_Module *info );
};

#endif

