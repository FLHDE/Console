/*
  DALib.h - Declarations for functions imported from DALib.dll.

  Jason Hood, 19 December, 2009.

  14 December, 2010:
  * identified TotalTime, Play and CurrentTime.
*/

#ifndef _DALIB_H
#define _DALIB_H

#pragma comment( lib, "DALib.lib" )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STDCALL __stdcall
#define EXTERN	__declspec(dllimport)


struct IAnimation2
{
  virtual void	STDCALL func_00();
  virtual void	STDCALL func_04();
  virtual void	STDCALL func_08();
  virtual void	STDCALL func_0c();
  virtual void	STDCALL func_10();

  virtual float STDCALL TotalTime( int );

  virtual void	STDCALL func_18();
  virtual void	STDCALL func_1c();
  virtual void	STDCALL func_20();

  virtual int	STDCALL Open( LPVOID, long, LPCSTR, int, int );
  virtual void	STDCALL Close( int );
  virtual void	STDCALL Play( int,	// id - Open's return value
			      int,	// 1 = backwards, 2 = repeat, 4 = cycle
			      float,	// start pos (-2 = normal, -1 = end)
			      float,	// speed
			      float,	// time to run (seconds)
			      float,	// end pos
			      float,	// unknown (apparently unused)
			      float );	// unknown (apparently unused)

  virtual void	STDCALL func_30();
  virtual void	STDCALL func_34();

  virtual void	STDCALL Stop( int );	// flush?

  virtual void	STDCALL func_3c();

  virtual float STDCALL CurrentTime( int );
};


namespace DALib
{
  EXTERN IAnimation2* Anim;
}

#endif
