/***************************************************************************
                                  socks5.h
                             -------------------
	revision             : $Id: socks5.h,v 1.1 2003-10-23 17:27:17 tellini Exp $
	copyright            : (C) 2003 by Simone Tellini
	email                : tellini@users.sourceforge.net

	description          : SOCKS5 details (RFC 1928)
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SOCKS5_H
#define SOCKS5_H

#define SOCKS5_VERSION						0x05

#define SOCKS5_AUTH_NO_AUTH_REQUIRED		0x00
#define SOCKS5_AUTH_GSSAPI					0x01
#define SOCKS5_AUTH_USER_PASS				0x02
#define SOCKS5_AUTH_NO_ACCEPTABLE_METHODS	0xFF

#define SOCKS5_MAX_METHODS_LEN				257

#define SOCKS5_CMD_CONNECT					0x01
#define SOCKS5_CMD_BIND						0x02
#define SOCKS5_CMD_ASSOCIATE				0x03

#define SOCKS5_ATYP_IPV4					0x01
#define SOCKS5_ATYP_DOMAINNAME				0x03
#define SOCKS5_ATYP_IPV6					0x04

#define SOCKS5_REP_SUCCEEDED				0x00
#define SOCKS5_REP_GENERAL_FAILURE			0x01
#define SOCKS5_REP_NOT_ALLOWED				0x02
#define SOCKS5_REP_NETWORK_UNREACHABLE		0x03
#define SOCKS5_REP_HOST_UNREACHABLE			0x04
#define SOCKS5_REP_CONNECTION_REFUSED		0x05
#define SOCKS5_REP_TTL_EXPIRED				0x06
#define SOCKS5_REP_CMD_NOT_SUPPORTED		0x07
#define SOCKS5_REP_ATYPE_NOT_SUPPORTED		0x08

// Field offsets

#define SOCKS5_F_VER						0
#define SOCKS5_F_NMETHODS					1
#define SOCKS5_F_METHODS					2

#define SOCKS5_F_CMD						1
#define SOCKS5_F_REP						1
#define SOCKS5_F_RSV						2
#define SOCKS5_F_ATYP						3
#define SOCKS5_F_DST_ADDR					4

#endif /* SOCKS5_H */
