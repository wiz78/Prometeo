/***************************************************************************
                                   acl.cpp
                             -------------------
    revision             : $Id: acl.cpp,v 1.3 2002-11-08 14:32:31 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : ACL manager
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

#include "registry.h"
#include "acl.h"

//---------------------------------------------------------------------------
static int pam_conv( int num_msg, const struct pam_message **msg,
					 struct pam_response **resp, void *appdata );

//---------------------------------------------------------------------------
Acl::Acl( Registry *reg, const char *basekey )
{
	Reg     = reg;
	BaseKey = basekey;

	PAM_Conv.conv        = pam_conv;
	PAM_Conv.appdata_ptr = this;
}
//---------------------------------------------------------------------------
bool Acl::UserPermission( const char *user, const char *perm )
{
	bool	ret = false;
	string	key = BaseKey + string( "/Users/" ) + string( user );

	if( Reg->OpenKey( key.c_str(), false )) {

		if( Reg->GetInteger( perm, 0 ))
			ret = true;

		Reg->CloseKey();
	}

	return( ret );
}
//---------------------------------------------------------------------------
void Acl::SetUserPermission( const char *user, const char *perm, bool granted )
{
	string	key = BaseKey + string( "/Users/" ) + string( user );

	if( Reg->OpenKey( key.c_str(), true )) {
		Reg->SetInteger( perm, granted );
		Reg->CloseKey();
	}
}
//---------------------------------------------------------------------------
void Acl::RemoveUserPermission( const char *user, const char *perm )
{
	string	key = BaseKey + string( "/Users/" ) + string( user );

	if( Reg->OpenKey( key.c_str(), true )) {
		Reg->DeleteValue( perm );
		Reg->CloseKey();
	}
}
//---------------------------------------------------------------------------
bool Acl::AuthenticateUser( const char *user, const char *pwd, const char *host )
{
	bool	ret = false;

	if( user && user[0] ) {
		int				pam_error;
		pam_handle_t	*pamh = NULL;

		Pwd       = pwd;
		pam_error = pam_start( "prometeo", user, &PAM_Conv, &pamh );

		if( pam_error == PAM_SUCCESS ) {

			if( host )
				pam_set_item( pamh, PAM_RHOST, host );

			pam_error = pam_authenticate( pamh, 0 );

			if( pam_error == PAM_SUCCESS )
				ret = true;
		}

		pam_end( pamh, pam_error );

		Pwd.erase();

		if( !ret )
			App->Log->Log( LOG_ERR, "ACL: authentication failed for user %s", user );
	}

	return( ret );
}
//---------------------------------------------------------------------------
static int pam_conv( int num_msg, const struct pam_message **msg,
					 struct pam_response **resp, void *appdata )
{
	struct pam_response *reply;
	char				buf[1024];

	/* PAM will free this later */
	reply = (struct pam_response *)malloc( num_msg * sizeof( *reply ));

	if( reply == NULL )
		return( PAM_CONV_ERR );

	for( int count = 0; count < num_msg; count++ ) {

		switch(( *msg )[ count ].msg_style ) {

			case PAM_PROMPT_ECHO_OFF:
				reply[ count ].resp         = strdup( ((Acl *)appdata )->GetPwd() );
				reply[ count ].resp_retcode = PAM_SUCCESS;
				break;

			case PAM_ERROR_MSG:
				if(( *msg )[ count ].msg )
					App->Log->Log( LOG_ERR, "ACL: PAM error: %s", ( *msg )[ count ].msg );

				reply[ count ].resp         = strdup( "" );
				reply[ count ].resp_retcode = PAM_SUCCESS;
				break;

			default:
				free( reply );
				return( PAM_CONV_ERR );
			}
	}

	*resp = reply;

	return( PAM_SUCCESS );
}
//---------------------------------------------------------------------------
