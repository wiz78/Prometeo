/***************************************************************************
                                  core.cpp
                             -------------------
	revision             : $Id: core.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>

#include "registry.h"
#include "ctrlipc.h"
#include "loader.h"
#include "dnscache.h"
#include "iodispatcher.h"

#define PROM_PID_FILE	"/tmp/prometeo.pid"
#define SPOOL_DIR		"/var/spool/prometeo"

//--------------------------------------------------------------------------
static void SigChildExit( int signum );
static void SigReloadCfg( int signum );
static void SigQuit( int signum );
static void SigDie( int signum );
static void SigNoAction( int signum );

static volatile int LastSig = -1;

//--------------------------------------------------------------------------
Core::Core()
{
	Cfg   = new Registry( this );
	Log   = new Logger();
	IO    = new IODispatcher();
	IPC   = NULL;
	Mods  = NULL;
	DNS   = NULL;
}
//--------------------------------------------------------------------------
Core::~Core()
{
	// be careful with the order of the deletions
	delete IPC;
	delete Mods;
	delete DNS;
	delete Cfg;
	delete Log;
	delete IO;
}
//--------------------------------------------------------------------------
void Core::Run( void )
{
	if( Daemonize() ) {

		CfgReload();

		Log->Log( LOG_INFO, PACKAGE" "VERSION" starting" );

//		IPC  = new CtrlIPC();
		DNS  = new DNSCache( IO );
		Mods = new Loader();
		IPC  = new CtrlIPC();

		Mods->LoadModules();

		Prom_set_ps_display( "ready" );

		MainLoop();

		Log->Log( LOG_INFO, PACKAGE" "VERSION" quitting" );

		IPC->Cleanup();
		
		unlink( PROM_PID_FILE );
	}
}
//--------------------------------------------------------------------------
void Core::MainLoop( void )
{
	time_t	CheckTime;

	Running = true;

	time( &CheckTime );

	while( Running ) {
		time_t	now;

		IO->WaitEvents( 1 );

		// stuff to do at most once per second
		if( time( &now ) > CheckTime ) {

			DNS->Flush( now );
			IO->CheckTimeouts( now );
			Mods->OnTimer( now );

			CheckTime = now;
		}
	}
}
//--------------------------------------------------------------------------
bool Core::Daemonize( void )
{
	bool	child = false;
#ifndef DEBUG
	pid_t	pid = fork();

	if( pid == -1 )
		Log->Log( LOG_CRIT, "fork() failed - %s", strerror( errno ));

	else if( pid == 0 ) { // child

		if( CheckPid() ) {
#endif
			child = true;

			Prom_init_ps_display( "master" );

#ifndef DEBUG
			// we don't need these
			close( STDIN_FILENO );
			close( STDOUT_FILENO );
			close( STDERR_FILENO );

			setsid();
#endif
			umask( 022 );

			mkdir( SPOOL_DIR, 0700 );
			chdir( SPOOL_DIR );

			signal( SIGCHLD, SigChildExit );
			signal( SIGHUP,  SigReloadCfg );
			signal( SIGQUIT, SigQuit      );
			signal( SIGINT,  SigQuit      );
			signal( SIGTERM, SigQuit      );
			signal( SIGSEGV, SigDie       );
			signal( SIGABRT, SigDie       );
			signal( SIGFPE,  SigDie       );
			signal( SIGUSR1, SigNoAction  );
			signal( SIGPIPE, SigNoAction  );
#ifndef DEBUG
		}
	}
#endif

	return( child );
}
//--------------------------------------------------------------------------
pid_t Core::Fork( char *ident )
{
	pid_t	pid;

	// ignore SIGHUP because:
	//   a. the child should ignore it
	//   b. the parent should stay blocked until the child is ready
	//
	// all other signals won't be ignored, since they cause the application
	// to quit.
	signal( SIGHUP, SIG_IGN );

	InitWait();

	pid = fork();

	if( pid == 0 ) {			// child

		// avoid that the child inherits some listening
		// sockets and stuff

		if( Mods )
			Mods->OnFork();

		if( DNS )
			DNS->OnFork();

		delete IPC;

		// at this point, we can unblock the parent
		kill( getppid(), SIGUSR1 );

		IPC  = NULL;

		Prom_init_ps_display( ident );

	} else if( pid > 0 ) {		// parent

		// let's wait until the child is done with the initial setup
		Wait();
	}

	if( pid != 0 )
		signal( SIGHUP, SigReloadCfg );

	return( pid );
}
//--------------------------------------------------------------------------
bool Core::InitWait( void )
{
	LastSig = -1;

	sigemptyset( &ZeroMask );
	sigemptyset( &NewMask );

	sigaddset( &NewMask, SIGUSR1 );

	return( sigprocmask( SIG_BLOCK, &NewMask, &OldMask ) >= 0 );
}
//--------------------------------------------------------------------------
void Core::Wait( void )
{
	while( LastSig == -1 )
		sigsuspend( &ZeroMask );

	sigprocmask( SIG_SETMASK, &OldMask, NULL );
}
//--------------------------------------------------------------------------
bool Core::CheckPid( void )
{
	bool		ret = true;
	ifstream	ifh( PROM_PID_FILE );

	if( ifh ) {

		ifh.close();

		ret = false;

		printf( "prometeo is already running\n" );

	} else {
		ofstream	ofh( PROM_PID_FILE );

		if( ofh ) {

			ofh << getpid();

			ofh.close();
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
void Core::CfgReload( void )
{
	Cfg->Load();
	Log->Setup( Cfg );

	if( DNS )
		DNS->ReloadCfg();

	if( Mods )
		Mods->CfgChanged();

	Log->Log( LOG_INFO, "configuration loaded" );
}
//--------------------------------------------------------------------------
static void SigChildExit( int signum )
{
    int status;

	// XXX check pid and re-spawn subprocesses if necessary
    while( waitpid( -1, &status, WNOHANG ) > 0 );

    signal( SIGCHLD, SigChildExit );
}
//--------------------------------------------------------------------------
static void SigReloadCfg( int signum )
{
	App->CfgReload();
	signal( SIGHUP, SigReloadCfg );
}
//--------------------------------------------------------------------------
static void SigQuit( int signum )
{
	App->Quit();
}
//--------------------------------------------------------------------------
static void SigDie( int signum )
{
	switch( signum ) {
			
		case SIGSEGV:
			throw "segmentation violation";

		case SIGABRT:
			throw "abnormal termination";

		case SIGFPE:
			throw "FPU exception";
	}
}
//--------------------------------------------------------------------------
static void SigNoAction( int signum )
{
	LastSig = signum;
	
	signal( signum, SigNoAction );
}
//--------------------------------------------------------------------------

