/***************************************************************************
                                    acl.h
                             -------------------
    revision             : $Id: acl.h,v 1.1 2002-11-03 17:28:46 tellini Exp $
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

#ifndef PROM_ACL_H
#define PROM_ACL_H

using namespace std;

#include <string>
#include <security/pam_appl.h>

class Registry;

class Acl
{
public:
				Acl( Registry *reg, const char *basekey );

	bool		UserPermission( const char *user, const char *perm );
	void		SetUserPermission( const char *user, const char *perm, bool granted );
	void		RemoveUserPermission( const char *user, const char *perm );

	bool		AuthenticateUser( const char *user, const char *pwd, const char *host = NULL );

	const char	*GetPwd( void ) const { return( Pwd.c_str() ); }

private:
	Registry	*Reg;
	string		BaseKey;
	string		Pwd;
	pam_conv	PAM_Conv;
};

#endif
