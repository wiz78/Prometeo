/***************************************************************************
                                 cfgdata.h
                             -------------------
	revision             : $Id: cfgdata.h,v 1.1.1.1 2002-10-10 09:59:26 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : configuration editor via web
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CFGDATA_H
#define CFGDATA_H

#define MOD_VERSION	"1.0"

#include <sys/types.h>
#include <string>

#include "api.h"
#include "linkedlist.h"
#include "buffer.h"

class CfgEditor;
class TcpSocket;
class PageMaker;

class CfgData : public LinkedListNode
{
public:
					CfgData( CfgEditor *cfg, TcpSocket *sock );
					~CfgData();

	void			SocketEvent( Prom_SC_Reason reason, int data );
			
private:
	CfgEditor		*Editor;
	TcpSocket		*Peer;
	enum {
		S_REQUEST_HEADER,
		S_REQUEST_BODY,
		S_CLOSING
	}				State;
	char			ReqBuf[ 1024 ];
	unsigned int	ReqLen;
	Buffer			Body;
	unsigned int	BodyLen;
	enum {
		M_NONE,
		M_GET,
		M_POST,
		M_UNKNOWN
	}				Method;
	string			RequestedPage;
	string			QueryString;
	string			Username;
	string			Password;

	void			Error( int err );
	void			GotData( int len );

	void			AddHeaderData( int len );
	void			AddBodyData( int len );

	void			ParseMethod( void );
	void			ParseHeader( char *hdr );
	void			ParseRequest( void );
	void			ParseAuth( char *str );

	void			ProcessRequest( PageMaker& pg );

	const char		*GetCodeMsg( int code ) const;
	void			SendError( int code, const char *body );

	bool			IsAuthorized( void );

	void			SendFile( string name );

	string			DecodeArg( const char *arg );
	string			UrlDecode( const string val );

	void			UpdateSettings( PageMaker& pg );
	void			UpdateMods( PageMaker& pg );

	void			DeleteMod( void );
	void			AddMod( void );
	void			EnableMod( bool enable );
	void			LoadMod( bool load );
};

#endif
