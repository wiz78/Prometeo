/***************************************************************************
                                 loader.cpp
                             -------------------
	revision             : $Id: loader.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include "main.h"

#include <ltdl.h>
#include <string>

#include "loader.h"
#include "module.h"
#include "registry.h"

//---------------------------------------------------------------------------
Loader::Loader()
{
	if( lt_dlinit())
		App->Log->Log( LOG_ERR, "loader: init error: %s", lt_dlerror() );
}
//---------------------------------------------------------------------------
Loader::~Loader()
{
	for( int i = Modules.Count() - 1; i >= 0; i-- ) {
		Module	*mod = (Module *)Modules[ i ];

		Unload( mod->GetFileName().c_str() );
	}

	lt_dlexit();
}
//---------------------------------------------------------------------------
bool Loader::Load( const char *modname, bool forceload )
{
	bool			ret = false;
	lt_dlhandle		module;
	const char		*name = NULL;
	bool			load = true;
	string			key = "root/Modules/";

	App->Log->Log( LOG_INFO,
				   "loader: trying to load %s... ",
				   modname );

	key += modname;

	// find the file name from the config
	if( App->Cfg->OpenKey( key.c_str(), false )) {

		name = App->Cfg->GetString( "module", NULL );
		load = App->Cfg->GetInteger( "enabled", true );

		App->Cfg->CloseKey();
	}

	if( !load && !forceload )
		App->Log->Log( LOG_INFO,
					   "loader: %s is not enabled, skipping it", 
					   modname );

	else if( name && ( module = lt_dlopenext( name ))) {
		Prom_Module	*modinfo;
		const char	*error;

		modinfo = (Prom_Module *)lt_dlsym( module, PROM_MODINFO_NAME );
		error   = IsValid( modinfo );

		if( error ) {

			App->Log->Log( LOG_ERR,
						   "loader: error loading %s (%s) - %s ",
						   modname, name, error );

			lt_dlclose( module );

		} else {
			Module	*mod = new Module( modname, module, modinfo );

			Modules.Add( mod );

			try {
				ret = mod->Setup( key.c_str() );
			}
			catch( const char *p ) {
				ret = false;
				App->Log->Log( LOG_ERR,
							   "loader: setup raised an exception in module %s - %s",
							   modname, p );
			}
			catch(...) {
				ret = false;
				App->Log->Log( LOG_ERR,
							   "loader: setup raised an exception in module %s",
							   modname );
			}

			if( !ret ) {

				Unload( modname );

				App->Log->Log( LOG_ERR,
							   "loader: setup failed for module %s",
							   modname );

			} else
				App->Log->Log( LOG_INFO,
							   "loader: module %s is up & running",
							   modname );
		}

	} else if( name )
		App->Log->Log( LOG_ERR,
					   "loader: coulnd't load the module %s - %s ",
					   name, lt_dlerror() );
	else
		App->Log->Log( LOG_ERR,
					   "loader: module %s is not described in the configuration file (%s)",
					   modname, key.c_str() );

	return( ret );
}
//---------------------------------------------------------------------------
void Loader::Unload( const char *modname )
{
	App->Log->Log( LOG_INFO, "loader: unloading %s", modname );

	for( int i = 0; i < Modules.Count(); i++ ) {
		Module	*mod = (Module *)Modules[ i ];

		if( mod->GetFileName() == modname ) {

			if( mod->Cleanup() ) {

				Modules.Remove( i );

				lt_dlclose(( lt_dlhandle )mod->GetLoaderHandle() );

				delete mod;

			} else
				mod->SetQuitting();

			break;
		}
	}
}
//---------------------------------------------------------------------------
const char *Loader::IsValid( Prom_Module *info )
{
	const char *err = NULL;

	if( info ) {

		if( info->Version > PROM_API_VERSION )
			err = "unsupported API version";

		else if(!( info->GetManifest && info->Cleanup ))
			err = "required function(s) missing";

	} else
		err = lt_dlerror();

	return( err );
}
//---------------------------------------------------------------------------
void Loader::CfgChanged( void )
{
	for( int i = 0; i < Modules.Count(); i++ ) {
		Module	*mod = (Module *)Modules[ i ];

		mod->CfgChanged();
	}
}
//---------------------------------------------------------------------------
void Loader::OnFork( void )
{
	for( int i = 0; i < Modules.Count(); i++ ) {
		Module	*mod = (Module *)Modules[ i ];

		mod->OnFork();
	}
}
//---------------------------------------------------------------------------
void Loader::LoadModules( void )
{
	if( App->Cfg->OpenKey( "root/Modules", false )) {
		const char	*name;
		int			num = 0;

		name = App->Cfg->GetString( "path", MOD_DIR );

		App->Log->Log( LOG_INFO, "loader: loading modules from %s", name );

		if( lt_dlsetsearchpath( name ))
			App->Log->Log( LOG_ERR, "loader: can't set search path: %s", lt_dlerror() );

		while( name = App->Cfg->EnumKeys( num++ ))
			Load( name, false );

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
void Loader::OnTimer( time_t now )
{
	for( int i = Modules.Count() - 1; i >= 0; i-- ) {
		Module	*mod = (Module *)Modules[ i ];

		if( mod->IsQuitting() ) {

			if( mod->Cleanup() ) {

				Modules.Remove( i );
				lt_dlclose(( lt_dlhandle )mod->GetLoaderHandle() );

				delete mod;
			}

		} else
			mod->OnTimer( now );
	}
}
//---------------------------------------------------------------------------
const char *Loader::EnumNames( int index )
{
	const char *ret = NULL;
		
	if( index < Modules.Count() )
		ret = ((Module *)Modules[ index ])->GetFileName().c_str();

	return( ret );
}
//---------------------------------------------------------------------------
bool Loader::IsRunning( const char *mod )
{
	bool	ret = false;
	int		i = 0;

	while( !ret && ( i < Modules.Count() ))
		if( ((Module *)Modules[ i++ ])->GetFileName() == mod )
			ret = true;

	return( ret );
}
//---------------------------------------------------------------------------
const char *Loader::GetManifest( const char *mod )
{
	const char	*ret = NULL;
	int			i = 0;

	// first search it among the loaded modules
	while( !ret && ( i < Modules.Count() )) {
		Module	*m = (Module *)Modules[ i++ ];

		if( m->GetFileName() == mod )
			ret = m->GetManifest();
	}

	// if it's not there, try to load it to get the manifest
	if( !ret ) {
		lt_dlhandle	module;
		string		key = "root/Modules/";
		const char	*name;

		key += mod;

		// find the file name from the config
		if( App->Cfg->OpenKey( key.c_str(), false )) {

			name = App->Cfg->GetString( "module", NULL );

			App->Cfg->CloseKey();
		}

		module = lt_dlopenext( name );

		if( module ) {
			Prom_Module *modinfo;

			modinfo = (Prom_Module *)lt_dlsym( module, PROM_MODINFO_NAME );

			if( !IsValid( modinfo ))
				Manifest = ( *modinfo->GetManifest )( key.c_str(), mod );

			// we need to store it in this buffer because now we're going
			// to dispose the module
			ret = Manifest.c_str();

			lt_dlclose( module );
		}
	}

	return( ret ? ret : "" );
}
//---------------------------------------------------------------------------

