/***************************************************************************
                                   module.h
                             -------------------
	revision             : $Id: module.h,v 1.1.1.1 2002-10-10 09:59:13 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MODULE_H
#define MODULE_H

#include <string>

#include "api.h"
#include "bitfield.h"

// Flags
#define PROM_MODF_QUIT		(1 << 0)

class Module
{
public:
					Module( const char *filename, void *handle, Prom_Module *modinfo );

	// wrappers for API calls
	bool			Setup( const char *key );
	bool			Cleanup( void );
	void 			CfgChanged( void );
	void			OnFork( void );
	void			OnTimer( time_t now );

	string&			GetFileName( void )            { return( FileName ); }
	const char		*GetModName( void ) const      { return( ModInfo->Name ); }
	const char		*GetManifest( void )           { return( ( *ModInfo->GetManifest )( Key.c_str(), FileName.c_str() )); }
	void			*GetLoaderHandle( void ) const { return( LoaderHandle ); }

	void			SetQuitting( void )       { Flags.Set( PROM_MODF_QUIT ); }
	bool			IsQuitting( void ) const  { return( Flags.IsSet( PROM_MODF_QUIT )); }

protected:
	Prom_Module		*ModInfo;
	HANDLE			ModHandle;
	BitField		Flags;

private:
	string			FileName;
	string			Key;
	void			*LoaderHandle;
};

#endif
