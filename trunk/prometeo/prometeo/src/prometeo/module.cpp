/***************************************************************************
                                 module.cpp
                             -------------------
    revision             : $Id: module.cpp,v 1.2 2002-11-13 16:45:59 tellini Exp $
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

#include "main.h"

#include "module.h"

//---------------------------------------------------------------------------
Module::Module( const char *filename, void *handle, Prom_Module *modinfo )
{
	FileName     = filename;
	Key          = "root/Modules/" + FileName;
	LoaderHandle = handle;
	ModInfo      = modinfo;
	ModHandle    = NULL;
}
//---------------------------------------------------------------------------
bool Module::Setup( const char *key )
{
	bool	ret = true;

	if( ModInfo->Setup ) {
		ModHandle = ( *ModInfo->Setup )( key );
		ret       = ModHandle != NULL;
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool Module::Cleanup( void )
{
	bool	ret;

	ret = ( *ModInfo->Cleanup )( ModHandle );

	return( ret );
}
//---------------------------------------------------------------------------
void Module::CfgChanged( void )
{
	if( ModInfo->CfgChanged )
		( *ModInfo->CfgChanged )( ModHandle );
}
//---------------------------------------------------------------------------
void Module::OnFork( void )
{
	if( ModInfo->OnFork )
		( *ModInfo->OnFork )( ModHandle );
}
//---------------------------------------------------------------------------
void Module::OnTimer( time_t now )
{
	if( ModInfo->OnTimer )
		( *ModInfo->OnTimer )( ModHandle, now );
}
//---------------------------------------------------------------------------
void Module::ParseCmd( const char *cmd, char *result, int reslen )
{
	if( ModInfo->ParseCmd )
		( *ModInfo->ParseCmd )( ModHandle, cmd, result, reslen );
}
//---------------------------------------------------------------------------

