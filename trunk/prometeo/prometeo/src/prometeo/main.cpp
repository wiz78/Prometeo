/***************************************************************************
                                   main.cpp
                             -------------------
	revision             : $Id: main.cpp,v 1.2 2003-04-11 12:23:10 tellini Exp $
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

//---------------------------------------------------------------------------
// hopefully, this should be the only global variable we need
Core *App;

//---------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	save_ps_display_args( argc, argv );

	App = new Core();

	try {

		App->Run();
	}
	catch( const char *p ) {

		App->Log->Log( LOG_CRIT, "Raised exception: %s", p );
	}
	catch(...) {

		App->Log->Log( LOG_CRIT, "Unhandled exception!" );
	}

	delete App;

	return( EXIT_SUCCESS );
}
//---------------------------------------------------------------------------
