/***************************************************************************
                                 Registry.cpp
                             -------------------
	revision             : $Id: registry.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include <expat.h>
#include <sys/stat.h>
#include <fstream>

#include "registry.h"
#include "regnote.h"

#define CFG_FILE	CFG_DIR"/prometeo.xml"

static void startElement( void *userData, const char *name, const char **atts );
static void endElement( void *userData, const char *name );
static void charHandler( void *userData, const XML_Char *s, int len );

//--------------------------------------------------------------------------
Registry::Registry( Core *app )
{
	App          = app;
	Tree         = new RegKey( "root" );
	CurrentKey   = NULL;
	CurrentValue = NULL;
}
//--------------------------------------------------------------------------
Registry::~Registry()
{
	Clear();
}
//--------------------------------------------------------------------------
void Registry::Clear( void )
{
	delete Tree;

	Tree         = NULL;
	CurrentKey   = NULL;
	CurrentValue = NULL;
}
//--------------------------------------------------------------------------
void Registry::SetupParser( void )
{
	Parser = XML_ParserCreate( NULL );

	if( Parser ) {

		XML_SetUserData(( XML_Parser )Parser, this );
		XML_SetElementHandler(( XML_Parser )Parser, startElement, endElement );
		XML_SetCharacterDataHandler(( XML_Parser )Parser, charHandler );
	}
}
//--------------------------------------------------------------------------
void Registry::Load( void )
{
	CheckFilePerms();
	SetupParser();

	if( Parser ) {
		XML_Parser 	parser = (XML_Parser)Parser;
		ifstream	fp( CFG_FILE );

		if( fp ) {
			int		done;
			RegKey	*oldtree = Tree;

			// try creating a new one
			Tree = new RegKey( "root" );

			Parsing      = true;
			CurrentKey   = NULL;
			CurrentValue = NULL;

			do {
				char	buf[ 1024 ];
				int		len;

				fp.read( buf, sizeof( buf ));

				len  = fp.gcount();
				done = len < sizeof( buf );

				if( !XML_Parse( parser, buf, len, done )) {
					App->Log->Log( LOG_ERR,
								   "config: %s at line %d",
								   XML_ErrorString( XML_GetErrorCode( parser )),
								   XML_GetCurrentLineNumber( parser ));
					Parsing = false;
					Clear();
				}

			} while( !done && Parsing );

			if( Tree )
				delete oldtree;
			else {

				Tree = oldtree;

				App->Log->Log( LOG_WARNING,
							   "config: configuration file ignored due to errors, reverting to previous settings" );
			}

			fp.close();
		}

		XML_ParserFree( parser );
    }

	CurrentKey   = NULL;
	CurrentValue = NULL;
}
//--------------------------------------------------------------------------
void Registry::Save( void )
{
	ofstream fh( CFG_FILE, ios::trunc );

	if( fh ) {

		fh << "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>" << endl;

		if( Tree )
			Tree->Save( fh, 0 );

		fh.close();

	} else
		App->Log->Log( LOG_ERR,
					   "config: cannot open %s for writing",
					   CFG_FILE );
}
//--------------------------------------------------------------------------
void Registry::StartElement( char *name, char **atts )
{
	if( !strcmp( name, "Key" )) {
		char	*keyname = NULL;

		while( *atts ) {
			char *attrname = *atts++;
			char *attrval  = *atts++;

			if( !strcmp( attrname, "name" ))
				keyname = attrval;
		}

		if( keyname ) {
			RegKey	*key = new RegKey( keyname );

			if( CurrentKey )
				CurrentKey->AddChild( key );
			else {
				delete Tree;
				Tree = key;
			}

			CurrentKey = key;

		} else
			App->Log->Log( LOG_WARNING,
						   "config: skipping unnamed key at line %d",
						   XML_GetCurrentLineNumber((XML_Parser)Parser ));

	} else if( !strcmp( name, "Value" )) {
		char	*valname = NULL;
		char	*valtype = NULL;

		while( *atts ) {
			char *attrname = *atts++;
			char *attrval  = *atts++;

			if( !strcmp( attrname, "name" ))
				valname = attrval;
			else if( !strcmp( attrname, "type" ))
				valtype = attrval;
		}

		if( valname && valtype ) {
			RegNode *val;

			if( !strcmp( valtype, "integer" ))
				val = new RegInteger( valname );
			else if( !strcmp( valtype, "string" ))
				val = new RegString( valname );

			if( CurrentKey ) {
				CurrentKey->AddChild( val );
				CurrentValue = val;
			} else {
				App->Log->Log( LOG_WARNING,
							   "config: skipping value outside key at line %d",
							   XML_GetCurrentLineNumber((XML_Parser)Parser ));
				delete val;
			}

		} else
			App->Log->Log( LOG_WARNING,
						   "config: skipping incomplete value at line %d",
						   XML_GetCurrentLineNumber((XML_Parser)Parser ));

	} else if( !strcmp( name, "Note" )) {

		if( CurrentKey ) {
			RegNode *val = new RegNote();

			CurrentKey->AddChild( val );
			CurrentValue = val;
		}

	} else {

		App->Log->Log( LOG_ERR,
					   "config: unknown keyword (%s) at line %d!",
					   name,
					   XML_GetCurrentLineNumber((XML_Parser)Parser ));

		Parsing = false;
	}
}
//--------------------------------------------------------------------------
void Registry::EndElement( const char *name )
{
	if( CurrentValue && !strcmp( CurrentValue->GetTag(), name )) {

		switch( CurrentValue->GetType() ) {

			case REG_INTEGER:
				((RegInteger *)CurrentValue )->SetValue( Buffer.ToInt() );
				break;

			case REG_STRING:
				((RegString *)CurrentValue )->SetValue( Buffer.c_str() );
				break;

			case REG_NOTE:
				((RegNote *)CurrentValue )->SetValue( Buffer.c_str() );
				break;
		}

		CurrentValue = NULL;

	} else if( CurrentKey  && !strcmp( CurrentKey->GetTag(), name )) {

		CurrentKey = CurrentKey->GetParent();

	} else {

		App->Log->Log( LOG_ERR,
					   "config: ending tag (%s) with no matching start tag at line %d",
					   name,
					   XML_GetCurrentLineNumber((XML_Parser)Parser ));

		Parsing = false;

		Clear(); // reject everything in case of malformed config
	}

	Buffer = "";
}
//--------------------------------------------------------------------------
void Registry::CharHandler( char *str, int len )
{
	// skip whitespace between <Value>'s
	if( CurrentValue )
		Buffer.append( str, len );
}
//--------------------------------------------------------------------------
static void startElement( void *userData, const char *name, const char **atts )
{
	((Registry *)userData )->StartElement((char *)name, (char **)atts );
}
//--------------------------------------------------------------------------
static void endElement( void *userData, const char *name )
{
	((Registry *)userData )->EndElement( name );
}
//--------------------------------------------------------------------------
static void charHandler( void *userData, const XML_Char *s, int len )
{
	((Registry *)userData )->CharHandler((char *)s, len );
}
//--------------------------------------------------------------------------
bool Registry::OpenKey( const char *path, bool create )
{
	bool 	ret = true;
	RegKey	*key;

	key = FindKey( path );

	if( !key ) {

		ret = false;

		if( create ) {
			char	*ptr = strrchr( path, '/' );

			if( ptr && *++ptr ) {
				int		len = ptr - path;
				char 	*tmp = new char[ len ];

				strncpy( tmp, path, len );

				tmp[ len - 1 ] = '\0';

				if( OpenKey( tmp, true )) {

					key = new RegKey( ptr );

					CurrentKey->AddChild( key );

					CurrentKey = key;
					ret        = true;
				}

				delete[] tmp;
			}
		}

	} else
		CurrentKey = key;

	return( ret );
}
//--------------------------------------------------------------------------
void Registry::CloseKey( void )
{
	CurrentKey = CurrentKey->GetParent();
}
//--------------------------------------------------------------------------
void Registry::DeleteKey( const char *path )
{
	delete FindKey( path );
}
//--------------------------------------------------------------------------
RegKey *Registry::FindKey( const char *path )
{
	RegKey	*key = NULL;

	// try looking inside the current key if path
	// doesn't start with root/
	if( CurrentKey && strncmp( path, "root/", 5 ))
		key = CurrentKey->FindKey( path );

	else {
		char 	*next = strchr( path, '/' );

		if( Tree && next ) // there should be another part, the first one must be 'root' at the moment
			key = Tree->FindKey( next + 1 );
	}

	return( key );
}
//--------------------------------------------------------------------------
const char *Registry::GetString( const char *name, const char *def )
{
	if( CurrentKey ) {
		RegString	*val = (RegString *)CurrentKey->FindValue( name, REG_STRING );

		if( val )
			def = val->GetValue();
	}

	return( def );
}
//--------------------------------------------------------------------------
void Registry::SetString( const char *name, const char *value )
{
	if( CurrentKey ) {
		RegString	*val = (RegString *)CurrentKey->FindValue( name, REG_STRING );

		if( !val ) {

			val = new RegString( name );

			CurrentKey->AddChild( val );
		}

		val->SetValue( value );
	}
}
//--------------------------------------------------------------------------
int Registry::GetInteger( const char *name, int def )
{
	if( CurrentKey ) {
		RegInteger	*val = (RegInteger *)CurrentKey->FindValue( name, REG_INTEGER );

		if( val )
			def = val->GetValue();
	}

	return( def );
}
//--------------------------------------------------------------------------
void Registry::SetInteger( const char *name, int value )
{
	if( CurrentKey ) {
		RegInteger	*val = (RegInteger *)CurrentKey->FindValue( name, REG_INTEGER );

		if( !val ) {

			val = new RegInteger( name );

			CurrentKey->AddChild( val );
		}

		val->SetValue( value );
	}
}
//--------------------------------------------------------------------------
char *Registry::EnumKeys( int index )
{
	char *name = NULL;

	if( CurrentKey ) {
		RegKey *key = CurrentKey->GetKey( index );

		if( key )
			name = key->GetName();
	}

	return( name );
}
//--------------------------------------------------------------------------
char *Registry::EnumValues( int index )
{
	char *name = NULL;

	if( CurrentKey ) {
		RegNode *key = CurrentKey->GetValue( index );

		if( key )
			name = key->GetName();
	}

	return( name );
}
//--------------------------------------------------------------------------
void Registry::CheckFilePerms( void )
{
	struct stat	statbuf;

	// The config file perms should be rwx------

	if( stat( CFG_FILE, &statbuf ) >= 0 ) {

		if( statbuf.st_mode & ( S_IRWXG | S_IRWXO ))
		   App->Log->Log( LOG_NOTICE,
						  "config: %s has insecure permissions",
						  CFG_FILE );
	}
}
//--------------------------------------------------------------------------
