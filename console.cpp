/*
  console.cpp - Use chat interface as a command console for single player.

  Jason Hood, 20 October to 22 December, 2009.

  Credits go to the FLHook team for making some of this easier.  Particular
  thanks to M0tah for help in making the ship jump around.

  Requires and assumes v1.1.

  v1.10, 23 February, 2010:
  - did not undo after "system object", causing CTD during a jump;
  - setting reputation to non-existing faction would still use friendly color;
  + added "ghost" and "godmode" commands - toggle collision and invincibility;
  + "rep = ..." will save the current reputation, "rep undo" will load it;
  * use two decimals for rotation values and remove trailing zeros;
  * allow reputation to be specified as a percentage.

  v1.20, 14 December, 2010 to 9 January, 2011:
  * handle non-bay animation separately;
  - fixed rep command when setting reputation via short name;
  - fixed crash when a target was set, but no object exists;
  * allow "base ." to revisit the current (or last) base;
  - restore original cruise constants when switching to multiplayer;
  * use Console chat channel;
  * update launch icon and ship cushion on Drive/Park;
  + enhanced line editing (for MP, too);
  + pos can set/recall multiple positions, use clipboard;
  * improved "s ." from a base (uses proper exit point).

  v1.21, 13 & 14 January, 2011:
  - fixed history search with no matching command;
  - fixed ship cushion under certain conditions.

  v1.22, 3 May, 2011:
  - preserve disabled collisions on launch/jump when ghost is off;
  - set EntryPoint_Org when needed (potential Moors conflict).

  v1.23, 6 October, 2013:
  + added "play sound" command to play a sound effect (DATA\AUDIO\sounds.ini);
  * only use IME (the flashing 'A') if using DBCS.

  v1.24, 7 May, 2016:
  - don't crash if an animation isn't run twice;
  * allow animations to run only forwards or backwards (ignored for bay doors).
*/

#include "internal.h"
#include "DALib.h"
#include "Server.h"
#include "BinaryRDL.h"
#include "resources.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>

#define PI 3.14159265f

#define NAKED	__declspec(naked)
#define STDCALL __stdcall
#define FASTCALL __fastcall


// Get chat working in single-player.
#define ADDR_ENTER	((PBYTE)  0x46a11f+1)
#define ADDR_ENTER1	((PBYTE)  0x437374+1)
#define ADDR_ENTER2	((PBYTE)  0x54ae67+1)
#define ADDR_ENTER3	((PBYTE)  0x574b91+1)
#define ADDR_ENTER4	((PBYTE)  0x574c43+1)
#define ADDR_ENTER5	((PBYTE)  0x574f74+1)
#define ADDR_MSGWIN	((PBYTE)  0x46b650)
#define ADDR_CHAT	((PBYTE)  0x5a625f+1)
#define ADDR_RTURN	((PDWORD) 0x45f828)
#define ADDR_UP 	((PDWORD) 0x45f830)
#define ADDR_DOWN	((PDWORD) 0x45f834)
#define ADDR_BKSP	((PDWORD) 0x57ce00)
#define ADDR_END	((PDWORD) 0x57ce10)
#define ADDR_HOME	((PDWORD) 0x57ce14)
#define ADDR_LEFT	((PDWORD) 0x57ce18)
#define ADDR_RIGHT	((PDWORD) 0x57ce20)
#define ADDR_DEL	((PDWORD) 0x57ce24)

// Add this DLL to [Resources].
#define ADDR_RSRC	((PDWORD)(0x5b1caa+1))

// Multiple ships.
#define ADDR_STARTUP	((PDWORD) 0x5c6cb8)
#define ADDR_LOAD	((PDWORD)(0x5ab230+1))
#define ADDR_SHIPID	((PBYTE)  0x4c3e15)
#define ADDR_EQUIPCOMM	((PBYTE)  0x437fb4+1)
#define ADDR_NAVBAR	((PBYTE)  0x43f664)
#define ADDR_CUSH0	((PDWORD)(0x44eb73+1))
#define ADDR_CUSH1	((PDWORD)(0x44f01f+2))
#define ADDR_CUSH1a	((PBYTE)  0x44f072)
#define ADDR_CUSH2	((PBYTE)  0x44f985)
#define ADDR_COMMICON	((PBYTE)  0x4781fc-1)
#define ADDR_PLAYERCASH ((PBYTE)  0x47842c+1)
#define ADDR_MAXQUANT	((PBYTE)  0x47c83f)
#define ADDR_DEALERCNT1 ((PBYTE)  0x47cb0a)
#define ADDR_DEALERCNT2 ((PBYTE)  0x47cb1d)
#define ADDR_MAXQUANT1	((PBYTE)  0x47e15c)
#define ADDR_PRICE0	((PBYTE)  0x47e4c0+1)
#define ADDR_XFERBUY	((PBYTE)  0x47eb35)
#define ADDR_FREEAMMO	((PUSHORT)0x47ed04)
#define ADDR_XFERSELL	((PBYTE)  0x47ef26)
#define ADDR_TIP	((PBYTE)  0x48028b-1)
#define ADDR_MAXQUANT2	((PBYTE)  0x480337)
#define ADDR_MAXQUANT3	((PDWORD)(0x48092b+2))
#define ADDR_DEALER	((PINT)(  0x480d40+1))
#define ADDR_LINE	((PBYTE)  0x48136b+1)
#define ADDR_TITLE	((PINT)(  0x48141f+1))
#define ADDR_HELP	((PINT)(  0x481439+1))
#define ADDR_TITLE1	((PBYTE)  0x48151c+1)
#define ADDR_BUY	((PINT)(  0x4815bf+1))
#define ADDR_SELL	((PINT)(  0x48167e+1))
#define ADDR_QUANT1	((float*)(0x481bd9+1))
#define ADDR_QUANT2	((float*)(0x481c43+1))
#define ADDR_QUANT3	((float*)(0x481c71+1))
#define ADDR_QUANT4	((float*)(0x481cb5+1))
#define ADDR_QUANT5	((float*)(0x481d8c+1))
#define ADDR_ITEMMAX	((PUSHORT)0x48381a)
#define ADDR_CARGOMSG	((PBYTE)  0x483ace+2)
#define ADDR_JOBS	((PBYTE)  0x48b8fe)

// NTB
#define ADDR_NTBLOOP	((PUSHORT)0x4ec087)
#define ADDR_NTBDIST	((float*) 0x5d954c)

// Show
#define ADDR_VISIT	((PBYTE)  0x4c4df1)
#define ADDR_HOUSES	((PUSHORT)0x49e799)
#define ADDR_PATHS	((PUSHORT)0x49cd34)
#define ADDR_PP1	((PDWORD)(0x48e4d2+1))
#define ADDR_PP2	((PDWORD)(0x48e4e2+1))
#define ADDR_PP3	((PDWORD)(0x49d98e+2))
#define ADDR_RP8_PP	(0x9288-4)

// Zoom
#define ADDR_ZOOMOUT	((float*)(0x4944b0-4))
#define ADDR_ZOOMIN	((float*)(0x49f2ab-4))

// Changing system when docked.
#define ADDR_SHIPLAUNCH ((PDWORD)(0x62b2a80+1))
#define ADDR_HARDPOINT	((PDWORD)(0x62aa9de+1))

// Animation
#define ADDR_SHIELD	((PBYTE)  0x62b2ba1)

// Ghost
#define ADDR_GHOST	((PBYTE)  0x679cc4)
#define ADDR_GHOSTJ	((PBYTE)  0x5450ca)

// Godmode
#define ADDR_GODMODE	((PBYTE)  0x67ecc0)

// Detect switch to multiplayer.
#define ADDR_NEWGAME	((PDWORD*)(0x5a8119+2))

// Intercept IME usage.
#define ADDR_IME	((PBYTE)  0x57baf3)


DWORD dummy;
#define ProtectX( addr, size ) \
  VirtualProtect( addr, size, PAGE_EXECUTE_READWRITE, &dummy );
#define ProtectW( addr, size ) \
  VirtualProtect( addr, size, PAGE_READWRITE, &dummy );

#define RELOFS( from, to ) \
  *(PDWORD)(from) = (DWORD)(to) - (DWORD)(from) - 4;

#define CALL( from, to ) \
  *(PBYTE)(from) = 0xe8; \
  RELOFS( (DWORD)from+1, to )

#define JMP( from, to ) \
  *(PBYTE)(from) = 0xe9; \
  RELOFS( (DWORD)from+1, to )

#define FUNC( func, addr ) T##func func = (T##func)addr

FUNC( CreateSound,	0x42ae40 );
FUNC( PlaySound,	0x4285f0 );
FUNC( GetString,	0x4347e0 );
FUNC( UpdateNavBar,	0x442060 );
FUNC( CloseDialog,	0x45a460 );
FUNC( GetRepCol,	0x45a650 );
FUNC( DisplayBase,	0x45b2d0 );
FUNC( DisplaySystem,	0x45b490 );
FUNC( FmtCredits,	0x4779a0 );
FUNC( DealerDialog,	0x477ab0 );
FUNC( LoadAutosave,	0x4bc830 );
FUNC( EnterBase,	0x4c4910 );
/*
FUNC( ShipValue,	0x4c6e90 );
*/
FUNC( SetWeaponGroup,	0x53c310 );
FUNC( LoadGame, 	0x5b2c70 );

Tstartup old_startup;

PUINT const pbats_id = (PUINT)0x674a88; // CreateID( "ge_s_battery_01" )
PUINT const pbots_id = (PUINT)0x674a8c; // CreateID( "ge_s_repair_01" )

// Make use of Freelancer's string buffer.
LPWSTR const wstrbuf = (LPWSTR)0x66dc60;
PUINT  const wstrlen = (PUINT)0x6119f8;

// Preserve original cruise speed & acceleration for multiplayer.
float mp_cspd, mp_cacc, sp_cspd, sp_cacc;

PUINT chat_channel = (PUINT)0x66d3fc;


class Freelancer1
{
public:
  LPVOID GetMarket( UINT base );
  void	 BaseEnter( UINT base );
  void	 BaseExit();
};

const DWORD fl1jmptable[] = { 0x43a510, 0x43b290, 0x43b3e0 };

#define FL1JMP( fn, idx ) NAKED fn { __asm jmp fl1jmptable[idx*4] }

FL1JMP( LPVOID Freelancer1::GetMarket( UINT ), 0 )
FL1JMP( void   Freelancer1::BaseEnter( UINT ), 1 )
FL1JMP( void   Freelancer1::BaseExit(),        2 )

Freelancer1* const fl1 = (Freelancer1*)0x668708;


const DWORD PlayerData_save = (DWORD)pub::Save - 0x6d5efa0 + 0x6d4c800;

NAKED
bool PlayerData::save( const CHARACTER_ID&, LPCWSTR, UINT )
{
  __asm jmp	PlayerData_save
}


// A dummy ship entry, needed after parking a ship.
UINT shipless_id;

const char shipless[] =
  "[Ship]\n"
  "nickname = shipless_ship\n"
  // Required, so make it something that doesn't have anything useful.
  "DA_archetype = ships\\utility\\csv\\csv_shield.3db\n";

bool on_launch_pad = true;	// cushion is tested before this is set
bool at_equipment_dealer;


HMODULE g_hinst;
UINT	rsrcid;

const UINT player = 1;
      UINT ship, base, sys;
      UINT docking_fixture;

BinaryRDL msg;

struct Bookmark
{
  UINT	 system;
  Vector pos;
  Matrix ornt;
};

Bookmark bookmark[10];


UINT CreateIDW( LPCWSTR wName )
{
  char aName[128];
  wcstombs( aName, wName, sizeof(aName) );
  return CreateID( aName );
}


// Retrieve a number from a string, which may be suffixed with 'k' or 'K' to
// multiply by 1000.  Returns true if a number was read, updating str to the
// next number; false otherwise.
bool get_value( float& val, LPCWSTR& str )
{
  double num;
  LPWSTR end;

  if (*str == '\0')
    return true;

  if (*str != ',')
  {
    num = wcstod( str, &end );
    if (*end == 'k' || *end == 'K')
    {
      num *= 1000;
      ++end;
    }
    if (*end != ' ' && *end != ',' && *end != '\0')
      return false;
    val = (float)num;
    str = end;
    if (*end == '\0')
      return true;
  }
  while (*++str == ' ') ;
  return true;
}


// Retrieve a vector from a string.  The vector is of the form "x [y [z]]" or
// "[x], [y], [z]".  Unused components remain unchanged in VEC.
void get_vector( Vector& vec, LPCWSTR& str )
{
  get_value( vec.x, str );
  get_value( vec.y, str );
  get_value( vec.z, str );
}


// Convert radians to degrees.
float degrees( float rad )
{
  rad *= 180 / PI;

  // Prevent displaying -0 and prefer 180 to -180.
  if (rad < 0)
  {
    if (rad > -0.005f)
      rad = 0;
    else if (rad <= -179.995f)
      rad = 180;
  }

  // Round to two decimal places here, so %g can display it without decimals.
  float frac = modff( rad * 100, &rad );
  if (frac >= 0.5f)
    ++rad;
  else if (frac <= -0.5f)
    --rad;

  return rad / 100;
}


// Convert an orientation matrix to a pitch/yaw/roll vector.  Based on what
// Freelancer does for the save game.
void Matrix_to_Vector( const Matrix& mat, Vector& vec )
{
  Vector x = { mat.m[0][0], mat.m[1][0], mat.m[2][0] };
  Vector y = { mat.m[0][1], mat.m[1][1], mat.m[2][1] };
  Vector z = { mat.m[0][2], mat.m[1][2], mat.m[2][2] };

  float h = (float)hypot( x.x, x.y );
  if (h > 1/524288.0f)
  {
    vec.x = degrees( atan2f(  y.z, z.z ) );
    vec.y = degrees( atan2f( -x.z, h   ) );
    vec.z = degrees( atan2f(  x.y, x.x ) );
  }
  else
  {
    vec.x = degrees( atan2f( -z.y, y.y ) );
    vec.y = degrees( atan2f( -x.z, h   ) );
    vec.z = 0;
  }
}


bool cmdAbout( LPCWSTR )
{
  msg.clear();
  msg.style( STYLE_INFO );
  msg.string( L"Console" );
  msg.style( STYLE_NOTICE );
  msg.string( L" by " );
  msg.style( StyleBoldYellow );
  msg.string( L"Jason Hood" );
  msg.style( STYLE_NOTICE );
  msg.string( L" (adoxa)." );
  msg.para();
  msg.strid( IDS(VERSION) );
  return true;
}


bool cmdHelp( LPCWSTR topic )
{
  int i = -1;
  if (*topic)
  {
    static const LPCWSTR topics[] =
    {
      L"game", L"base", L"move", L"ship", L"hud", L"player"
    };
    int len = wcslen( topic );
    for (i = sizeof(topics)/sizeof(*topics); --i >= 0;)
    {
      if (wcsnicmp( topics[i], topic, len ) == 0)
	break;
    }
  }
  ++i;

  FmtStr caption, message;
  caption.begin_mad_lib( rsrcid + i );
  caption.end_mad_lib();
  message.begin_mad_lib( rsrcid + i + 1000 );
  message.end_mad_lib();
  pub::Player::PopUpDialog( 0, caption, message, 8 /* OK */ );

  return false;
}


bool cmdAutoSave( LPCWSTR )
{
  pub::Save( player, 1 );	// 1 is Auto Save, anything else is strid

  msg.strid( IDS(GAME_SAVED) + true );	// assume success
  return true;
}


bool cmdAutoLoad( LPCWSTR )
{
  LoadAutosave();		// same as pressing Load Autosave after you die
  return false;
}


// Generate a file name and description for the save game.
void fl_name( char* file, LPCWSTR desc )
{
  if (*desc == '\0' || desc[1] == '\0')
  {
    WCHAR slot[3], buf[64];
    slot[0] = ' ';
    // Explicitly disallow colon to prevent an alternate data stream on NTFS.
    slot[1] = (*desc == ':') ? '?' : towupper( *desc );
    slot[2] = '\0';
    sprintf( file, "qsave%S.fl", slot+1 );
    GetString( RSRC, IDS(QUICK_SAVE), buf, 64 );
    swprintf( wstrbuf, buf, (slot[1]) ? slot : slot+1 );
  }
  else
  {
    sprintf( file, "%x.fl", CreateIDW( desc ) );
    wcscpy( wstrbuf, desc - 1 );   // use the space as an indicator
  }
}


bool cmdSave( LPCWSTR desc )
{
  CHARACTER_ID cid;
  fl_name( cid.file, desc );
  msg.strid( IDS(GAME_SAVED) + Players.playerdata->save( cid, wstrbuf, 0 ) );
  return true;
}


bool cmdLoad( LPCWSTR desc )
{
  char path[512];
  CHARACTER_ID cid;
  fl_name( cid.file, desc );
  GetUserDataPath( path );
  strcat( path, "\\Accts\\SinglePlayer\\" );
  strcat( path, cid.file );
  if (GetFileAttributes( path ) == -1)
  {
    msg.strid( IDS(GAME_NOT_FOUND) );
    return true;
  }

  LoadGame( cid, true );

  return false;
}


// Get the ship, returning true if successful.
bool InSpace()
{
  ship = 0;
  pub::Player::GetShip( player, ship );
  return (ship != 0);
}


// Retrieve the ship's ID, position and orientation.  Returns true if
// successful, false if not in space.
bool GetLocation( Vector& pos, Matrix& ornt )
{
  if (!InSpace())
    return false;

  pub::SpaceObj::GetLocation( ship, pos, ornt );
  return true;
}


// Store the current system, position and orientation in the provided bookmark.
void SetBookmark( int idx )
{
  const Universe::ISystem* pSys;
  UINT	 sys;
  Vector pos;
  Matrix ornt;

  if (!GetLocation( pos, ornt ))
    return;
  pub::Player::GetSystem( player, sys );
  pSys = Universe::get_system( sys );

  // Don't change previous position if only orientation has changed.
  if (idx != 0 ||
      bookmark[0].system != pSys->id ||
      bookmark[0].pos.x != pos.x ||
      bookmark[0].pos.y != pos.y ||
      bookmark[0].pos.z != pos.z)
  {
    bookmark[idx].system = pSys->id;
    bookmark[idx].pos	 = pos;
    bookmark[idx].ornt	 = ornt;
  }
}


// Set the ship's position and orientation (ship must already be defined) and
// update them to what was actually set.
void SetLocation( Vector& pos, Matrix& ornt )
{
  SetBookmark( 0 );	// remember previous location

  pub::Player::GetSystem( player, sys );
  pub::SpaceObj::Relocate( ship, sys, pos, ornt );
  pub::SpaceObj::GetLocation( ship, pos, ornt );
}


bool cmdSystem( LPCWSTR );

bool cmdPos( LPCWSTR opt )
{
  Vector pos, rot;
  Matrix ornt;
  bool in_space = GetLocation( pos, ornt );

  bool loc = false;
  if (*opt)
  {
    const Universe::ISystem* pSys;

    if (wcscmp( opt, L"copy" ) == 0)
    {
      if (!in_space)
	return false;
      pub::Player::GetSystem( player, sys );
      pSys = Universe::get_system( sys );
      Matrix_to_Vector( ornt, rot );
      // Don't use Unicode, just in case there's still some 9X users.
      int len = sprintf( (char*)wstrbuf, "%s %g, %g, %g, %g, %g, %g",
					 pSys->nickname,
					 pos.x, pos.y, pos.z,
					 rot.x, rot.y, rot.z ) + 1;
      if (OpenClipboard( NULL ))
      {
	EmptyClipboard();
	HANDLE clip = GlobalAlloc( GMEM_MOVEABLE, len );
	if (clip)
	{
	  LPVOID data = GlobalLock( clip );
	  if (data)
	  {
	    memcpy( data, wstrbuf, len );
	    GlobalUnlock( clip );
	    SetClipboardData( CF_TEXT, clip );
	    loc = true;
	  }
	}
	CloseClipboard();
      }
      msg.strid( (loc) ? IDS(COPIED) : IDS(NO_CLIP) );
      return true;
    }

    if ((iswdigit( *opt ) && opt[1] == '\0') ||
	wcscmp( opt, L"clip" ) == 0)
      return cmdSystem( opt );

    if (*opt == '=' && opt[1] == '\0')
    {
      bool none = true;
      msg.clear();
      msg.style( STYLE_INFO );
      msg.TRA( TRA_Underline, TRA_Underline );
      msg.strid( IDS(BOOKMARKS) );
      msg.TRA( 0, TRA_Underline );
      msg.style( STYLE_NOTICE );
      for (int i = 0; i < 10; ++i)
      {
	if (bookmark[i].system != 0)
	{
	  none = false;
	  msg.para();
	  msg.printf( L"%d = ", i );
	  pSys = Universe::get_system( bookmark[i].system );
	  msg.strid( pSys->strid_name );
	  msg.printf( L", %g, %g, %g", bookmark[i].pos.x,
				       bookmark[i].pos.y,
				       bookmark[i].pos.z );
	}
      }
      if (none)
      {
	msg.para();
	msg.strid( 3022 );	// None
      }
      return true;
    }

    if (!in_space)
      return false;

    if (*opt == '=' && iswdigit( opt[1] ) && opt[2] == '\0')
    {
      SetBookmark( opt[1] - '0' );
      msg.strid( IDS(BOOKMARK_SET) );
      return true;
    }

    get_vector( pos, opt );
    if (*opt)
    {
      Matrix_to_Vector( ornt, rot );
      get_vector( rot, opt );
      ornt = EulerMatrix( rot );
      loc = true;
    }
    SetLocation( pos, ornt );
  }
  else if (!in_space)
    return false;

  msg.printf( L"pos = %g, %g, %g", pos.x, pos.y, pos.z );
  if (loc)
  {
    Matrix_to_Vector( ornt, rot );
    msg.para();
    msg.printf( L"rotate = %g, %g, %g", rot.x, rot.y, rot.z );
  }

  return true;
}


bool cmdPosR( LPCWSTR opt )
{
  Vector pos, rot;
  Matrix ornt;
  if (!GetLocation( pos, ornt ))
    return false;

  bool loc = false;
  if (*opt)
  {
    Vector rel;
    rel.zero();
    get_vector( rel, opt );
    pos.x += rel.x;
    pos.y += rel.y;
    pos.z += rel.z;
    if (*opt)
    {
      Matrix_to_Vector( ornt, rot );
      rel.zero();
      get_vector( rel, opt );
      rot.x += rel.x;
      rot.y += rel.y;
      rot.z += rel.z;
      ornt = EulerMatrix( rot );
      loc = true;
    }
    SetLocation( pos, ornt );
  }

  msg.printf( L"pos = %g, %g, %g", pos.x, pos.y, pos.z );
  if (loc)
  {
    Matrix_to_Vector( ornt, rot );
    msg.para();
    msg.printf( L"rotate = %g, %g, %g", rot.x, rot.y, rot.z );
  }

  return true;
}


bool cmdPrint( LPCWSTR opt )
{
  msg.clear();
  msg.style( STYLE_UNIVERSE );
  msg.string( opt );
  return true;
}


bool cmdRot( LPCWSTR opt )
{
  Vector pos, rot;
  Matrix ornt;
  if (!GetLocation( pos, ornt ))
    return false;

  if (*opt)
  {
    Matrix_to_Vector( ornt, rot );
    get_vector( rot, opt );
    ornt = EulerMatrix( rot );
    SetLocation( pos, ornt );
  }

  Matrix_to_Vector( ornt, rot );
  msg.printf( L"rotate = %g, %g, %g", rot.x, rot.y, rot.z );
  return true;
}


bool cmdRotR( LPCWSTR opt )
{
  Vector pos, rot;
  Matrix ornt;
  if (!GetLocation( pos, ornt ))
    return false;

  if (*opt)
  {
    Vector rel;
    rel.zero();
    get_vector( rel, opt );
    Matrix_to_Vector( ornt, rot );
    rot.x += rel.x;
    rot.y += rel.y;
    rot.z += rel.z;
    ornt = EulerMatrix( rot );
    SetLocation( pos, ornt );
  }

  Matrix_to_Vector( ornt, rot );
  msg.printf( L"rotate = %g, %g, %g", rot.x, rot.y, rot.z );
  return true;
}


// Determine how many times the name should be found.  If OPT ends in '*', find
// the second, "**" the third and "*N" the Nth.  The '*' is replaced with NUL.
int GetCount( LPWSTR opt )
{
  LPWSTR star = wcschr( opt, '*' );
  if (!star)
    return 1;

  int count;
  if (!star[1])
    count = 2;
  else if (star[1] == '*' && !star[2])
    count = 3;
  else
  {
    LPWSTR end;
    count = wcstol( star + 1, &end, 10 );
    if (*end)
      return 1;
  }
  *star = '\0';
  return count;
}


// Find PAT within TGT, ignoring case.
LPWSTR wcsstri( LPCWSTR tgt, LPCWSTR pat )
{
  int delta = wcslen( tgt ) - wcslen( pat );
  if (delta < 0)
    return NULL;

  LPCWSTR end = tgt + delta;
  for (;;)
  {
    WCHAR ch = towlower( *pat );
    while (ch != towlower( *tgt ))
    {
      if (++tgt > end)
	return NULL;
    }
    for (int i = 1;; ++i)
    {
      if (pat[i] == '\0')
	return (LPWSTR)tgt - 1;
      if (towlower( pat[i] ) != towlower( tgt[i] ))
	break;
    }
    ++tgt;
  }
}


// Search a system for an object.
UINT find_object( UINT system, LPWSTR object )
{
  struct ObjNameEnumerator : public pub::System::SysObjEnumerator
  {
    UINT    object;
    LPCWSTR partial;
    UINT    id;
    int     count;
    bool    nick;
    bool operator()( const pub::System::SysObj& obj )
    {
      WCHAR  buf[128];
      UINT   objid = CreateID( obj.nickname );
      LPWSTR match;
      if (nick)
      {
	mbstowcs( buf, obj.nickname, 128 );
	match = wcsstri( buf, partial );
      }
      else
      {
	if (id == objid)
	{
	  object = objid;
	  return false;
	}
	GetString( RSRC, obj.ids_name, buf, 128 );
	match = wcsstr( buf, partial );
      }
      if (match)
      {
	object = objid;
	if (--count <= 0)
	  return false;
      }
      return true;
    }
  } objname;
  if (*object == '~')
  {
    objname.nick    = true;
    objname.partial = object + 1;
  }
  else
  {
    objname.nick    = false;
    objname.partial = object;
  }
  objname.id	 = CreateIDW( object );
  objname.count  = GetCount( object );
  objname.object = 0;
  pub::System::EnumerateObjects( system, objname );

  if (!objname.object)
    msg.strid( IDS(OBJECT_NOT_FOUND) );

  return objname.object;
}


NAKED
CShip* GetCShip()
{
  __asm mov	eax, 0x54baf0
  __asm call	eax
  __asm test	eax, eax
  __asm jz	noship
  __asm add	eax, 12
  __asm mov	eax, [eax+4]
  noship:
  __asm ret
}


bool cmdJump( LPCWSTR opt )
{
  Vector pos;
  Matrix ornt;
  if (!GetLocation( pos, ornt ))
    return false;

  if (iswdigit( *opt ) || *opt == '-' || *opt == ',')
  {
    Vector dist;
    dist.zero();
    get_vector( dist, opt );
    pos.add_z( dist.x, ornt );
    pos.add_x( dist.y, ornt );
    pos.add_y( dist.z, ornt );
  }
  else
  {
    UINT tgt;
    if (!*opt)
    {
      pub::SpaceObj::GetTarget( ship, tgt );
      if (!tgt)
	return false;
    }
    else
    {
      UINT objsys;
      pub::Player::GetSystem( player, sys );
      tgt = CreateIDW( opt );
      // Looks like it only works if it's in the current system, anyway.
      if (pub::SpaceObj::GetSystem( tgt, objsys ) < 0)
      {
	tgt = find_object( sys, (LPWSTR)opt );
	if (!tgt)
	  return true;
      }
    }
    UINT type;
    float  rad, brad;
    Vector vec, bvec;
    Vector tpos;
    Matrix tornt;
    if (pub::SpaceObj::GetType( tgt, type ) < 0)
    {
      // Let's assume a waypoint.
      CShip* cship = GetCShip();
      IObjRW* target = cship->get_target();
      if (target == NULL)
	return false;
      tpos = target->get_position();
      rad = 0.3121905f; // target->get_physical_radius( rad, vec );
    }
    else
    {
      pub::SpaceObj::GetLocation( tgt, tpos, tornt );
      pub::SpaceObj::GetRadius( tgt, rad, vec );
      pub::SpaceObj::GetBurnRadius( tgt, brad, bvec );
      if (rad < brad)
	rad = brad;
    }
    bool moor = false;
    if (type == 0x100)				// station
    {
      // Test for a mooring fixture.
      UINT arch;
      pub::SpaceObj::GetSolarArchetypeID( tgt, arch );
      Archetype::Solar* solar = Archetype::GetSolar( arch );
      if (solar->archid == docking_fixture)
      {
	moor = true;
	type = 0x20;
      }
    }
    if (type == 0x20)				// docking ring
    {
      pos  = tpos;
      ornt = tornt;
      pos.add_z( -rad, ornt );
      if (!moor)
	pos.add_x( 98, ornt );			// move across to the opening
    }
    else if (type == 0x40)			// jump gate
    {
      pos  = tpos;
      ornt = tornt;
      pos.add_z( 1.5f * rad, ornt );
      ornt.turn_around();
    }
    else if (type == 0x80)			// trade lane
    {
      ornt = tornt;
      Vector p = tpos;
      p.add_z( -rad, ornt );
      // Find which side we're on by simply seeing which distance is greater.
      if (pos.distsqr( p ) > pos.distsqr( tpos ))
      {
	ornt.turn_around();
	pos = tpos;
	pos.add_z( -rad, ornt );
      }
      else
      {
	pos = p;
      }
    }
    else
    {
      float dist = pos.dist( tpos );

      if (type & 7)		// moon/planet/sun
      {
	// Toggle between getting right up close and seeing the whole thing.
	if (dist < rad+2 || dist > rad*2+2)
	  rad *= 2;
      }

      pos.x = tpos.x - rad * (tpos.x - pos.x) / dist;
      pos.y = tpos.y - rad * (tpos.y - pos.y) / dist;
      pos.z = tpos.z - rad * (tpos.z - pos.z) / dist;

      Vector delta;
      delta.x = tpos.x - pos.x;
      delta.y = tpos.y - pos.y;
      delta.z = tpos.z - pos.z;
      ornt = LookMatrixYup( delta );

      if (type == 0x100)		// station
      {
	// If a station is close to a planet (such as Trenton Outpost), there's
	// a chance of ending up in the atmosphere.  If the position is not
	// empty, move across to the other side.  Our position is right on the
	// radius of the target, which counts as occupied, so test the position
	// half our ship's radius backwards, and reduce the radius by one to
	// prevent intersection with the target.
	pub::Player::GetSystem( player, sys );
	pub::SpaceObj::GetRadius( ship, brad, vec );
	brad /= 2;
	delta = pos;
	delta.add_z( -brad, ornt );
	--brad;
	bool empty;
	pub::SpaceObj::IsPosEmpty( sys, delta, brad, empty );
	if (!empty)
	{
	  pos.add_z( 2 * rad, ornt );
	  ornt.turn_around();
	}
      }
    }
  }

  SetLocation( pos, ornt );
  return false;
}


bool cmdKillSelf( LPCWSTR )
{
  CShip* cship = GetCShip();
  if (cship)
    pub::SpaceObj::Destroy( cship->id, FuseDestroy );

  return false;
}

NAKED
UINT FASTCALL get_id(IObjRW* obj)
{
  __asm {
    mov eax, [ecx+0x10]
    test eax, eax
    je noid
    mov ecx, [eax+0x4C]
    and ecx, 3
    cmp cl, 3
    jne noid
    mov eax, [eax+0xB0]
    ret
  noid:
    xor eax, eax
    ret
  }
}

bool cmdKillTarget( LPCWSTR )
{
  CShip* cship = GetCShip();
  if (cship)
  {
    IObjRW* target = cship->get_target();
    if (target)
    {
      UINT tgt_id = get_id( target );
      if (tgt_id)
        pub::SpaceObj::Destroy( tgt_id, FuseDestroy );
    }
  }

  return false;
}


bool cmdCSpd( LPCWSTR opt )
{
  if (*opt)
  {
    sp_cspd = EngineEquipConsts::CRUISING_SPEED = (float)wcstod( opt, 0 );
    // Set the maximum speed a little higher than cruise speed, since ships
    // that have fallen behind go faster than cruise to catch up.
    float max = EngineEquipConsts::CRUISING_SPEED*1.5f;
    if (max > PhySys::ANOM_LIMITS_MAX_VELOCITY)
      PhySys::ANOM_LIMITS_MAX_VELOCITY = max;
  }

  msg.printf( IDS(CRUISESPD), EngineEquipConsts::CRUISING_SPEED );
  return true;
}


bool cmdCAcc( LPCWSTR opt )
{
  if (*opt)
    sp_cacc = EngineEquipConsts::CRUISE_ACCEL_TIME = (float)wcstod( opt, 0 );

  msg.printf( IDS(CRUISEACC), EngineEquipConsts::CRUISE_ACCEL_TIME );
  return true;
}


enum /* BayState */ { CLOSED, CLOSING, OPENED, OPENING };

static int anim = -1, anim_state, anim_dir;
static CShip* anim_ship;

// Wait for the animation to finish and then remove it.
DWORD WINAPI AnimWait( LPVOID )
{
  for (;;)
  {
    if (anim_state == OPENING)
    {
      if (DALib::Anim->CurrentTime( anim ) >= DALib::Anim->TotalTime( anim ))
      {
	if (anim_dir == 1)
	  break;
	anim_state = OPENED;
      }
    }
    else if (anim_state == CLOSING)
    {
      if (DALib::Anim->CurrentTime( anim ) == 0)
	break;
    }

    Sleep( 100 );
    if (GetCShip() != anim_ship)
      break;
  }

  AnimDB::Rem( anim );
  DALib::Anim->Stop( anim );
  DALib::Anim->Close( anim );
  anim = -1;

  return 0;
}


bool cmdAnim( LPCWSTR opt )
{
  CShip* cship = GetCShip();
  if (!cship)
    return false;

  if (*opt && ((*opt != '<' && *opt != '>') || opt[1]))
  {
    if (anim == -1)
    {
      anim_dir = 0;
      if (*opt == '>')
      {
	anim_dir = 1;
	++opt;
      }
      else if (*opt == '<')
      {
	anim_dir = -1;
	++opt;
      }
      char script[128];
      if (wcsnicmp( L"wings", opt, wcslen( opt ) ) == 0)
	strcpy( script, "Sc_extend wing" );
      else
	wcstombs( script, opt, sizeof(script) );
      anim = DALib::Anim->Open( cship->aship->animation_list,
				cship->engine_instance,
				script,
				0, 0 );
      if (anim == -1)
      {
	msg.strid( IDS(ANIMATION_NOT_FOUND) );
	return true;
      }
      DALib::Anim->Play( anim, (anim_dir < 0), (anim_dir < 0) ? -1 : -2,
			 1, 0, 1, 1, 0 );
      AnimDB::Add( anim );
      anim_state = (anim_dir < 0) ? CLOSING : OPENING;
      anim_ship = cship;
      CloseHandle( CreateThread( NULL, 4096, AnimWait, NULL, 0, NULL ) );
    }
    else
    {
      DALib::Anim->Play( anim, (anim_state != CLOSING), -2, 1, 0, 1, 1, 0 );
      AnimDB::Add( anim );
      anim_state = (anim_state == CLOSING) ? OPENING : CLOSING;
    }
  }
  else
  {
    if (cship->bay_state >= OPENED)
    {
      cship->close_bay();
    }
    else
    {
      *ADDR_SHIELD = 0xEB;	// prevent the shield being deactivated
      cship->open_bay();
      *ADDR_SHIELD = 0x74;
    }
  }

  return false;
}


void ChangeBase()
{
  on_launch_pad = true;

  fl1->BaseExit();
  fl1->BaseEnter( base );
  EnterBase( base );
  DisplayBase( 2 );
}


bool cmdBase( LPCWSTR wcsBase )
{
  Universe::IBase* pBase;
  bool launch = false;

  UINT cur_base = 0;
  pub::Player::GetBase( player, cur_base );
  if (cur_base)
  {
    if (*wcsBase)
    {
      launch = true;
    }
    else
    {
      pBase = Universe::get_base( cur_base );
      msg.string( pBase->nickname );
      return true;
    }
  }
  if (!*wcsBase)
  {
    if (!cur_base && base)
    {
      pBase = Universe::get_base( base );
      msg.strid( pBase->strid_name );
      return true;
    }
    return false;
  }

  if (*wcsBase == '.' && wcsBase[1] == '\0')
  {
    if (cur_base)
      base = cur_base;
    else if (!base)
      return false;
  }
  else
  {
    base = 0;
    UINT iBase = CreateIDW( wcsBase );
    int count = GetCount( (LPWSTR)wcsBase );
    pBase = Universe::GetFirstBase();
    while (pBase)
    {
      if (pBase->id == iBase)
      {
	base = iBase;
	break;
      }
      GetString( RSRC, pBase->strid_name, wstrbuf, *wstrlen );
      if (wcsstr( wstrbuf, wcsBase ))
      {
	// Ignore the intro bases.
	if (strnicmp( "intro", pBase->nickname, 5 ) != 0)
	{
	  base = pBase->id;
	  if (--count <= 0)
	    break;
	}
      }
      pBase = Universe::GetNextBase();
    }
    if (!base)
    {
      msg.strid( IDS(BASE_NOT_FOUND) );
      return true;
    }
  }

  if (launch)
  {
    ChangeBase();
  }
  else
  {
    SetBookmark( 0 );
    pub::Player::ForceLand( player, base );
  }

  return false;
}


PBYTE SwitchOut;
PUINT const pSysSwitch = (PUINT)0x67977c;


void SwitchOutDone()
{
  PBYTE const patch = SwitchOut;

  patch[0x0d7] = 0x0f;
  patch[0x0d8] = 0x84;
  patch[0x119] = 0x87;
  *(PDWORD)(patch+0x11a) = 0x1b8;

  *(PDWORD)(patch+0x25d) = 0x1cf7f;

  patch[0x266] = 0x1a;
  *(float*)(patch+0x2b0) =
  *(float*)(patch+0x2b8) =
  *(float*)(patch+0x2c0) = 0;
  *(float*)(patch+0x2c8) =
  *(float*)(patch+0x2d0) =
  *(float*)(patch+0x2d8) = 1;
  *(float*)(patch+0x2e0) =
  *(float*)(patch+0x2e8) =
  *(float*)(patch+0x2f0) =
  *(float*)(patch+0x2f8) =
  *(float*)(patch+0x300) =
  *(float*)(patch+0x308) = 0;

  *(PDWORD)(patch+0x388) = 0xcf8b178b;

  *(PDWORD)(patch+0x3c8) = 0x5ca64;

  DisplaySystem();
}


bool cmdLaunch( LPCWSTR );

bool  ship_entry;
DWORD EntryPoint_Org, ShipLaunch_Org;


void enter_at_ship()
{
  Vector pos;
  Matrix ornt;
  GetLocation( pos, ornt );

  float* const patch = (float*)(SwitchOut+0x2b0);
  patch[ 0] = pos.z;
  patch[ 2] = pos.y;
  patch[ 4] = pos.x;
  patch[ 6] = ornt.m[2][2];
  patch[ 8] = ornt.m[1][1];
  patch[10] = ornt.m[0][0];
  patch[12] = ornt.m[2][1];
  patch[14] = ornt.m[2][0];
  patch[16] = ornt.m[1][2];
  patch[18] = ornt.m[1][0];
  patch[20] = ornt.m[0][2];
  patch[22] = ornt.m[0][1];
}


NAKED
void ChangeSystem()
{
  __asm push	0x62aff80
  __asm push	ecx

  *pSysSwitch = sys;
  if (ship_entry)
  {
    enter_at_ship();
    RELOFS( ADDR_HARDPOINT, EntryPoint_Org );
  }

  *ADDR_SHIPLAUNCH = ShipLaunch_Org;

  __asm pop	ecx
  // Force the station type, since docking rings cause additional problems.
  __asm mov	dword ptr [ecx+0x28], 0x100
  __asm ret
}


// Replace the dock mount hardpoint with the second dock point, so we enter
// where the launch sequence usually finishes.
LPCSTR STDCALL EntryPoint( LPCSTR hp )
{
  static char hpdockpoint[64] = "hpdockpoint?02";

  char* mnt = strstr( hp, "hpdockmount" );
  if (mnt == NULL)
    return hp;

  hpdockpoint[11] = mnt[11];
  return hpdockpoint;
}


NAKED
void EntryPoint_Hook()
{
  __asm {
	mov	eax, SwitchOut
	add	eax, -0xf600 + 0x20b14
	cmp	[esp+0x90], eax 	// make sure it's for our launch
	jne	done
	push	edx
	call	EntryPoint
	mov	[esp+8], eax
  done:
	jmp	EntryPoint_Org
  }
}


struct entry_stack
{
  UINT	 object;		// presumably const UINT&
  Vector pos;
  BYTE	 stuff[0x28];
  Matrix ornt;
};

UINT entry_object;


LPVOID STDCALL enter_system( entry_stack* stack )
{
  UINT	 type;
  float  rad, brad;
  Vector vec;

  pub::SpaceObj::GetType( entry_object, type );
  pub::SpaceObj::GetLocation( entry_object, stack->pos, stack->ornt );
  pub::SpaceObj::GetRadius( entry_object, rad, vec );
  pub::SpaceObj::GetBurnRadius( entry_object, brad, vec );
  if (rad < brad)
    rad = brad;

  if (type & 7) 			// moon/planet/sun
  {
    rad *= 2;				// view the whole thing
  }
  else if (type == 0x20)		// docking ring
  {
    stack->pos.add_x( 98, stack->ornt ); // move across to the opening
  }
  else if (type == 0x40)		// jump gate
  {
    rad *= 1.5f;
    stack->ornt.turn_around();
  }

  stack->pos.add_z( -rad, stack->ornt );

  return 0;
}


bool cmdSystem( LPCWSTR opt )
{
  const Universe::ISystem* pSys;

  if (!*opt)
  {
    pub::Player::GetSystem( player, sys );
    pSys = Universe::get_system( sys );
    msg.string( pSys->nickname );
    return true;
  }

  Vector pos, rot;
  Matrix ornt;
  bool launch = !GetLocation( pos, ornt );
  bool bm = false;
  bool cb = false;

  ship_entry = false;
  entry_object = 0;

  if (iswdigit( *opt ) && opt[1] == '\0')
  {
    int idx = *opt - '0';
    if (bookmark[idx].system == 0)
    {
      msg.strid( IDS(BOOKMARK_NOT_SET) );
      return true;
    }
    sys  = bookmark[idx].system;
    pos  = bookmark[idx].pos;
    ornt = bookmark[idx].ornt;
    bm = true;
  }
  else if (wcscmp( opt, L"clip" ) == 0)
  {
    if (IsClipboardFormatAvailable( CF_TEXT ) && OpenClipboard( NULL ))
    {
      HANDLE clip = GetClipboardData( CF_TEXT );
      if (clip)
      {
	char* data = (char*)GlobalLock( clip );
	if (data)
	{
	  cb = true;
	  opt = wstrbuf + *wstrlen;	// there are two buffers
	  mbstowcs( (LPWSTR)opt, data, *wstrlen );
	  GlobalUnlock( clip );
	}
      }
      CloseClipboard();
    }
    if (!cb)
    {
      msg.strid( IDS(NO_CLIP) );
      return true;
    }
  }
  if (!bm)
  {
    int i = 0;
    WCHAR nick[64];
    while (*opt != ' ' && *opt != '\0' && i < 63)
      nick[i++] = *opt++;
    nick[i] = '\0';
    if (*nick == '.' && i == 1)
      pub::Player::GetSystem( player, sys );
    else
    {
      sys = 0;
      UINT iSys = CreateIDW( nick );
      int count = GetCount( nick );
      pSys = Universe::GetFirstSystem();
      while (pSys)
      {
	if (pSys->id == iSys)
	{
	  sys = iSys;
	  break;
	}
	GetString( RSRC, pSys->strid_name, wstrbuf, *wstrlen );
	if (wcsstr( wstrbuf, nick ))
	{
	  sys = pSys->id;
	  if (--count <= 0)
	    break;
	}
	pSys = Universe::GetNextSystem();
      }
      if (!sys)
      {
	msg.strid( IDS(SYSTEM_NOT_FOUND) );
	return true;
      }
    }

    while (*opt == ' ')
      ++opt;
    if (iswdigit( *opt ) || *opt == '-' || *opt == ',')
    {
      get_vector( pos, opt );
      if (*opt)
      {
	Matrix_to_Vector( ornt, rot );
	get_vector( rot, opt );
	ornt = EulerMatrix( rot );
      }
      else if (launch)
      {
	rot.zero();
	ornt = EulerMatrix( rot );
      }
    }
    else if (*opt)
    {
      entry_object = find_object( sys, (LPWSTR)opt );
      if (!entry_object)
	return true;
    }
    else
      ship_entry = true;
  }

  UINT oldsys;
  pub::Player::GetSystem( player, oldsys );
  if (sys == oldsys && !launch && (bm || cb))
  {
    SetLocation( pos, ornt );
    return false;
  }
  SetBookmark( 0 );

  PBYTE const patch = SwitchOut;
  patch[0x0d7] = 0xeb;				// ignore exit object
  patch[0x0d8] = 0x40;
  patch[0x119] = 0xbb;				// set the destination system
  *(PDWORD)(patch+0x11a) = sys;

  if (entry_object)
  {
    RELOFS( patch+0x25d, enter_system );	// locate our entry object
    patch[0x266] = 0x15;			// don't generate warning
  }
  else
  {
    patch[0x266] = 0x45;			// don't generate warning
    *(float*)(patch+0x2b0) = pos.z;		// set entry location
    *(float*)(patch+0x2b8) = pos.y;
    *(float*)(patch+0x2c0) = pos.x;
    *(float*)(patch+0x2c8) = ornt.m[2][2];
    *(float*)(patch+0x2d0) = ornt.m[1][1];
    *(float*)(patch+0x2d8) = ornt.m[0][0];
    *(float*)(patch+0x2e0) = ornt.m[2][1];
    *(float*)(patch+0x2e8) = ornt.m[2][0];
    *(float*)(patch+0x2f0) = ornt.m[1][2];
    *(float*)(patch+0x2f8) = ornt.m[1][0];
    *(float*)(patch+0x300) = ornt.m[0][2];
    *(float*)(patch+0x308) = ornt.m[0][1];
  }

  *(PDWORD)(patch+0x388) = 0x03ebc031;		// ignore entry object

  RELOFS( patch+0x3c8, SwitchOutDone ); 	// replace autosave with undo

  if (launch)
  {
    Players.playerdata->skip_autosave = true;
    ShipLaunch_Org = *ADDR_SHIPLAUNCH;
    RELOFS( ADDR_SHIPLAUNCH, ChangeSystem );
    if (ship_entry)
    {
      EntryPoint_Org = (DWORD)ADDR_HARDPOINT + *ADDR_HARDPOINT + 4;
      RELOFS( ADDR_HARDPOINT, EntryPoint_Hook );
    }
    cmdLaunch( 0 );
  }
  else
    *pSysSwitch = sys;

  return false;
}


// Toggle collision detection.
bool cmdGhost( LPCWSTR )
{
  *ADDR_GHOST ^= 1;

  msg.strid( (*ADDR_GHOST) ? IDS(GHOST_ON) : IDS(GHOST_OFF) );

  return true;
}


// Toggle god mode.
bool cmdGodmode( LPCWSTR )
{
  *ADDR_GODMODE ^= 1;

  msg.strid( (*ADDR_GODMODE) ? IDS(GODMODE_ON) : IDS(GODMODE_OFF) );

  return true;
}


// Emulate clicking the launch icon.
bool cmdLaunch( LPCWSTR )
{
  UINT base = 0;
  pub::Player::GetBase( player, base );
  if (base)
  {
    on_launch_pad = true;

  __asm push	eax
  __asm push	offset done
  __asm sub	esp, 0x38
  __asm push	ebx
  __asm push	ebp
  __asm push	esi
  __asm push	edi
  __asm mov	eax, 0x440f94
  __asm jmp	eax
  }
  done:

  return false;
}


EquipDescVector empty_loadout;

struct ShipStatus
{
  UINT			 base;
  UINT			 ship;
  EquipDescVector	 loadout;
  /*
  float 		 hull;
  CollisionGroupDescList cg;
  */
  string		 wg;
};

typedef std::list<ShipStatus> ShipStorage;
typedef ShipStorage::iterator ShipStorageIter;
ShipStorage ship_storage;


// List all the stored ships, or just those on the current base.
void ListShips( bool all = false )
{
  bool any = false;

  UINT base;
  pub::Player::GetBase( player, base );

  msg.clear();
  msg.style( STYLE_INFO );
  msg.TRA( TRA_Underline, TRA_Underline );
  msg.strid( IDS(SHIPS) );
  msg.TRA( 0, TRA_Underline );
  msg.style( STYLE_NOTICE );
  ShipStorageIter iter = ship_storage.begin(), end = ship_storage.end();
  for (;;)
  {
    if (all || base == iter->base)
    {
      any = true;
      msg.para();
      Archetype::Ship* aship = Archetype::GetShip( iter->ship );
      msg.strid( aship->strid );
      if (all)
      {
	msg.string( L" (" );
	Universe::IBase* pBase = Universe::get_base( iter->base );
	msg.strid( pBase->strid_name );
	msg.string( L")" );
      }
    }
    if (++iter == end)
      break;
  }
  if (!any)
  {
    msg.para();
    msg.strid( 3022 );	// None
  }
}


bool cmdShips( LPCWSTR )
{
  if (ship_storage.empty())
    return false;

  ListShips( true );
  return true;
}


ShipStorageIter FindShip( LPWSTR name )
{
  pub::Player::GetBase( player, base );

  int cnt = GetCount( name );
  int len = wcslen( name );
  ShipStorageIter iter = ship_storage.begin(), end = ship_storage.end();
  ShipStorageIter rc = end;
  for (; iter != end; ++iter)
  {
    if (base != iter->base)
      continue;
    if (!*name)
    {
      if (rc != end)
      {
	ListShips();
	return end;
      }
      rc = iter;
    }
    else
    {
      Archetype::Ship* aship = Archetype::GetShip( iter->ship );
      GetString( RSRC, aship->strid, wstrbuf, *wstrlen );
      if (wcsnicmp( wstrbuf, name, len ) == 0)
      {
	rc = iter;
	if (--cnt <= 0)
	  break;
      }
    }
  }
  if (rc == end)
    msg.strid( (*name) ? IDS(SHIP_NOT_FOUND) : IDS(NO_SHIPS_HERE) );

  return rc;
}


// Determine if the current ship is in full health.  This is used because
// restoring the status requires a bit more work (changing hull status at
// 0x673380 and collision group list at 0x67296c) which doesn't seem worth it.
// Since I use the Equipment Dealer to handle transfer, there's no means to
// store an item's health, so test those, too.
bool IsShipHealthy()
{
  CollisionGroupDescListIter iter, end;
  std::list<EquipDesc>::iterator e, ee;

  if (Players.playerdata->hull_status != 1)
    goto repair;

  iter = Players.playerdata->collisiongroupdesc.begin();
  end  = Players.playerdata->collisiongroupdesc.end();
  for (; iter != end; ++iter)
  {
    if (iter->health != 1)
      goto repair;
  }

  e  = Players.playerdata->equipdesclist.equip.begin();
  ee = Players.playerdata->equipdesclist.equip.end();
  for (; e != ee; ++e)
  {
    if (e->fHealth != 1)
      goto repair;
  }

  return true;

repair:
  msg.strid( IDS(REPAIR_DAMAGE) );
  return false;
}


bool GetShipStatus( ShipStatus& status )
{
  if (InSpace())
    return false;

  pub::Player::GetBase( player, status.base );
  pub::Player::GetShipID( player, status.ship );
  if (status.ship == shipless_id)
    return false;

  status.loadout = Players.playerdata->equipdesclist;
  /*
  status.hull	 = Players.playerdata->hull_status;

  CollisionGroupDescListIter iter = Players.playerdata->collisiongroupdesc.begin();
  CollisionGroupDescListIter end  = Players.playerdata->collisiongroupdesc.end();
  for (; iter != end; ++iter)
  {
    if (iter->health != 1)
      status.cg.push_back( *iter );
  }
  */

  status.wg = Players.playerdata->wg;

  return true;
}


bool SetShipStatus( ShipStatus& status )
{
  if (InSpace())
    return false;

  pub::Player::SetShipAndLoadout( player, status.ship, status.loadout );

  /*
  Players.playerdata->hull_status = status.hull;

  CollisionGroupDescListIter iter = status.cg.begin(), end = status.cg.end();
  for (; iter != end; ++iter)
  {
    CollisionGroupDescListIter pcg  = Players.playerdata->collisiongroupdesc.begin();
    CollisionGroupDescListIter pcge = Players.playerdata->collisiongroupdesc.end();
    for (; pcg != pcge; ++pcg)
    {
      if (pcg->id == iter->id)
      {
	pcg->health = iter->health;
	break;
      }
    }
  }
  */

  Players.playerdata->wg = status.wg;
  SetWeaponGroup( player, status.wg.c_str(), status.wg.length() );

  return true;
}


// Remove or show the ship cushion.  If I knew removing the cushion was going
// to be this complicated, I wouldn't have bothered.

DWORD ResetCushion_Org, SetCushion_Org, CushionLoop_Org;
DWORD cushion_base, cushion_base_temp;
DWORD cushion_ret;
int   cushion, cushion_idx, cushion_idx_temp, cushion_cnt;
const char gf_rtc_shipcushion[] = "gf_rtc_shipcushion";


NAKED
void ResetCushion_Hook()
{
  __asm {
	cmp	ecx, cushion_base
	jne	done
	mov	cushion_base, ebp
  done:
	jmp	ResetCushion_Org
  }
}


// Manhattan uses PlayerShipHover, so when you start the game without a ship,
// the ship cushion can be removed.  However, every other base uses
// gf_rtc_shipcushion, so there is no difference between the player's ship and
// ships for sale.  Test if the entity_name indicates it is for player.
bool STDCALL IsPlayerCushion( LPCSTR entity, DWORD base, int idx )
{
  char lc_entity[256];

  strcpy( lc_entity, entity );
  _strlwr( lc_entity );
  if (strstr( lc_entity, "player" ) != NULL)
    return true;

  if (++cushion_cnt == 1)
  {
    cushion_base_temp = base;
    cushion_idx_temp  = idx;
  }
  return false;
}


NAKED
void SetCushion_Hook()
{
  __asm {
	pop	cushion_ret

	cmp	cushion_base, 0 	// already found it
	jne	done			// (eax contains pointer)

	call	SetCushion_Org		// DACOM.stricmp
	test	eax, eax		// PlayerShipHover is definitely
	jz	okay			//  the player's cushion
	push	offset gf_rtc_shipcushion
	call	SetCushion_Org		// DACOM.stricmp
	pop	ecx
	test	eax, eax
	jnz	done
	push	edi
	push	[esi+0x48]
	push	[ebp+0x38]		// entity_name
	call	IsPlayerCushion
	test	al, al
	jz	fin
  okay:
	mov	eax, 0x4c3e10
	mov	edx, [esi+0x48]
	mov	cushion_idx, edi
	mov	cushion_base, edx
	call	eax
	test	eax, eax
	jnz	done
	mov	cushion, edi
  fin:
	inc	eax
  done:
	jmp	cushion_ret
  }
}

DWORD SetCushion = (DWORD)SetCushion_Hook;


// We didn't explicitly find a cushion for the player, so if there was only one,
// that must be it (since we don't do this in the ship dealer).  Due to a bit
// of luck, if there are two, the first is always the player.
NAKED
void CushionLoop_Hook()
{
  __asm {
	xor	ebp, ebp
	xchg	cushion_cnt, ebp
	cmp	cushion_base, 0
	jne	done
	cmp	ebp, 2
	je	okay
	cmp	ebp, 1
	jne	done
	cmp	on_launch_pad, 1
	je	okay
	cmp	at_equipment_dealer, 1
	jne	done
  okay:
	mov	ebp, cushion_base_temp
	mov	ebx, cushion_idx_temp
	mov	cushion_base, ebp
	mov	cushion_idx,  ebx
	mov	ebp, 0x4c3e10
	push	eax
	call	ebp
	test	eax, eax
	pop	eax
	jnz	done
	mov	cushion, ebx
  done:
	mov	ebp, [esp+0x24+4]
	mov	ebx, eax
	ret
  }
}


NAKED
void UpdateCushion_Hook()
{
  __asm {
	cmp	eax, cushion_base
	jne	okay
	xor	ecx, ecx
	cmp	esi, cushion
	je	done
  okay:
	mov	ecx, [eax+esi*4]
	test	ecx, ecx
  done:
	ret
  }
}


int  drive_park;
const char equipment[] = "Equipment";


// Getting the current room turned out to be quite impractical (virtual rooms
// store the name of the room as part of the NavBar structure, but normal
// rooms use the location id).	This hooks the creation of the bar to turn off
// the flag, turning it on when ShipID_Hook is called to create the launch icon.
NAKED
void NavBar_Hook()
{
  __asm {
	mov	on_launch_pad, 0

	// This only works for a real room, not a virtual room.  For current
	// purposes, this is sufficient.  To test a virtual room, use
	// [ebx+0x374], if it is not zero.
	mov	edi, offset equipment
	mov	esi, eax
	mov	ecx, 10 //sizeof(equipment)
	repe	cmpsb
	sete	at_equipment_dealer

	mov	edi, 0x5cb7ec
	ret
  }
}


// Intercept the function that retrieves the current ship id.  If it's
// shipless_id, return 0 to indicate we don't actually have a ship, except for
// those calls where it matters.
NAKED
void ShipID_Hook()
{
  __asm {
	mov	ecx, [esp]
	cmp	ecx, 0x43f6eb		// creating launch icon
	jne	not_launch
	mov	on_launch_pad, 1
  not_launch:
	cmp	drive_park, 0		// handle launch icon (this occurs
	jne	launch			//  before the update)
	cmp	eax, shipless_id
	jne	done
	cmp	ecx, 0x448866		// needed to make the ship (dis)appear
	je	done
	cmp	ecx, 0x448b33
	je	done
	cmp	ecx, 0x478e45		// buy ship cargo bar
	je	buyship
	cmp	ecx, 0x479145		// buy ship info button
	je	buyship
	cmp	ecx, 0x47b36f		// buy ship inventory scroll bar
	je	done
	cmp	ecx, 0x48146f		// buy ship message
	je	done
	cmp	ecx, 0x481f00		// back to ship selection screen button
	je	done
	cmp	ecx, 0x4826f5
	je	done
  zero:
	xor	eax, eax
  done:
	ret
  buyship:
	mov	ecx, [esi+0x360]
	test	ecx, ecx
	jz	zero
	cmp	[ecx+0x978], SHIP_DEALER
	jne	zero
	ret
  launch:
	mov	eax, drive_park
	dec	eax
	ret
  }
}


// Update the launch icon and ship cushion after Drive/Park.
void UpdateShip( bool park )
{
  if (on_launch_pad)
  {
    drive_park = (park) ? 1 : 2;
    UpdateNavBar();
    drive_park = 0;
  }
  cushion = (park) ? cushion_idx : -1;
}


// Put the ship into storage.
bool cmdPark( LPCWSTR )
{
  ShipStatus status;
  if (!GetShipStatus( status ))
    return false;

  if (!IsShipHealthy())
    return true;

  // Disable the job board.
  *ADDR_JOBS = 0xeb;
  CloseDialog();

  ship_storage.push_back( status );

  pub::Player::SetShipAndLoadout( player, shipless_id, empty_loadout );
  /*
  Players.playerdata->hull_status = 1;
  */
  UpdateShip( true );

  WCHAR name[128];
  Archetype::Ship* aship = Archetype::GetShip( status.ship );
  GetString( RSRC, aship->strid, name, 128 );
  msg.printf( IDS(PARKED), name );
  return true;
}


bool cmdPing( LPCWSTR )
{
  msg.string( L"Pong!" );
  return true;
}


// Retrieve a ship from storage.
bool cmdDrive( LPCWSTR opt )
{
  if (InSpace() || ship_storage.empty())
    return false;

  ShipStorageIter iter = FindShip( (LPWSTR)opt );
  if (iter == ship_storage.end())
    return true;

  // Store the current ship.
  ShipStatus status;
  if (GetShipStatus( status ))
  {
    if (!IsShipHealthy())
      return true;
    ship_storage.push_back( status );
    WCHAR name[128];
    Archetype::Ship* aship = Archetype::GetShip( status.ship );
    GetString( RSRC, aship->strid, name, 128 );
    msg.printf( IDS(PARKED), name );
    msg.para();
  }
  else
  {
    UpdateShip( false );
  }

  SetShipStatus( *iter );

  Archetype::Ship* aship = Archetype::GetShip( iter->ship );
  msg.strid( aship->strid );

  // Take it out of storage.
  ship_storage.erase( iter );

  // Enable the job board.
  *ADDR_JOBS = 0x74;
  CloseDialog();

  return true;
}


struct MarketGoodInfo
{
  UINT	arch;
  float price;
  int	unused[3];		// min, stock, dunno (0)
  float rank;
  float rep;
  int	quantity;
  float scale;
  bool	something;
  BYTE	mounts; 		// make use of padding
};

float  remaining_cargo;
int    max_bots,    max_bats;
PINT   market_bots, market_bats;

// Looks like the market people used a newer library.
//typedef std::map<UINT, MarketGoodInfo> MarketMap;
//MarketMap market;
LPVOID market;


NAKED
MarketGoodInfo* FindMarketGood( LPVOID market, UINT good )
{
  __asm mov	ecx, [esp+4]		// market
  __asm push	dword ptr [esp+8]	// good
  __asm mov	eax, 0x437d90
  __asm call	eax
  __asm ret
}


int STDCALL CheckCargoSpace( int quantity, UINT arch )
{
  Archetype::Equipment* equip = Archetype::GetEquipment( arch );
  if (equip && equip->volume)
  {
    // Mounted items don't occupy cargo space, so check if it's being
    // transferred back.
    MarketGoodInfo* good = FindMarketGood( market, arch );
    if (good && good->quantity >= good->mounts)
    {
      float volume = quantity * equip->volume;
      if (volume > remaining_cargo)
	quantity = remaining_cargo / equip->volume;
    }
  }
  return quantity;
}


void STDCALL XferBuy( MarketGoodInfo* good, int quantity )
{
  if (good->quantity >= good->mounts)
  {
    Archetype::Equipment* equip = Archetype::GetEquipment( good->arch );
    if (equip && equip->volume)
      remaining_cargo += quantity * equip->volume;
  }
}


NAKED
void XferBuy_Hook()
{
  __asm sub	eax, edi
  __asm mov	[ebx+0x1c], eax
  __asm push	edi
  __asm push	ebx
  __asm call	XferBuy
  __asm ret
}


void STDCALL XferSell( MarketGoodInfo* good, int quantity )
{
  if (good->quantity > good->mounts)
  {
    Archetype::Equipment* equip = Archetype::GetEquipment( good->arch );
    if (equip && equip->volume)
      remaining_cargo -= quantity * equip->volume;
  }
}


NAKED
void XferSell_Hook()
{
  __asm add	[ebp+0x1c], esi
  __asm push	esi
  __asm push	ebp
  __asm call	XferSell
  __asm mov	eax, [edi+4]
  __asm ret
}


// Set the quantity slider maximum to the appropriate value for the other ship.
NAKED
void SetMaxQuantity()
{
  __asm push	eax
  __asm push	edx
  __asm cmp	edx, ds:[0x674a8c]
  __asm jne	bat
  __asm mov	ecx, max_bots
  __asm mov	edx, market_bots
  __asm jmp	tst
  bat:
  __asm cmp	edx, ds:[0x674a88]
  __asm jne	cargo
  __asm mov	ecx, max_bats
  __asm mov	edx, market_bats
  tst:
  __asm sub	ecx, [edx]
  __asm cmp	ebx, ecx
  __asm jle	ok
  __asm mov	ebx, ecx
  ok:
  __asm pop	edx
  __asm pop	eax
  __asm mov	[eax+0x38c], ebx
  __asm ret
  cargo:
  __asm push	edx
  __asm push	ebx
  __asm call	CheckCargoSpace
  __asm mov	ebx, eax
  __asm jmp	ok
}


// Set the quantity on selection.
NAKED
void SetMaxQuantity1()
{
  __asm je	ok
  __asm mov	ebp, [eax+0x14] 	// EquipDesc.iCount
  __asm mov	edx, [eax+4]		// EquipDesc.iArchID
  __asm cmp	edx, ds:[0x674a8c]
  __asm jne	bat
  __asm mov	ecx, max_bots
  __asm mov	edx, market_bots
  __asm jmp	tst
  bat:
  __asm cmp	edx, ds:[0x674a88]
  __asm jne	cargo
  __asm mov	ecx, max_bats
  __asm mov	edx, market_bats
  tst:
  __asm sub	ecx, [edx]
  __asm cmp	ebp, ecx
  __asm jle	ok
  __asm mov	ebp, ecx
  ok:
  __asm ret
  cargo:
  __asm push	edx
  __asm push	ebp
  __asm call	CheckCargoSpace
  __asm mov	ebp, eax
  __asm ret
}


// Set the quantity on initialisation.
NAKED
void SetMaxQuantity2()
{
  __asm mov	ecx, eax
  __asm mov	eax, 1
  __asm jz	ok
  __asm mov	eax, [ecx+0x14] 	// EquipDesc.iCount
  __asm mov	edx, [ecx+4]		// EquipDesc.iArchID
  __asm cmp	edx, ds:[0x674a8c]
  __asm jne	bat
  __asm mov	ecx, max_bots
  __asm mov	edx, market_bots
  __asm jmp	tst
  bat:
  __asm cmp	edx, ds:[0x674a88]
  __asm jne	cargo
  __asm mov	ecx, max_bats
  __asm mov	edx, market_bats
  tst:
  __asm sub	ecx, [edx]
  __asm cmp	eax, ecx
  __asm jle	ok
  __asm mov	eax, ecx
  ok:
  __asm ret
  cargo:
  __asm push	edx
  __asm push	eax
  __asm call	CheckCargoSpace
  __asm ret
}


// Disable transferring if capacity is full.
NAKED
void SetMaxQuantity3()
{
  __asm cmp	eax, ds:[0x674a8c]
  __asm jne	bat
  __asm mov	eax, 932
  __asm mov	ecx, max_bots
  __asm mov	edx, market_bots
  __asm jmp	tst
  bat:
  __asm cmp	eax, ds:[0x674a88]
  __asm jne	cargo
  __asm mov	eax, 933
  __asm mov	ecx, max_bats
  __asm mov	edx, market_bats
  tst:
  __asm cmp	ecx, [edx]
  __asm jg	ok
  full:
  __asm push	0
  __asm push	eax
  __asm mov	edx, 0x47fd50
  __asm mov	ecx, esi
  __asm call	edx
  __asm xor	eax, eax
  __asm ret
  ok:
  __asm jmp	dword ptr ds:[0x5c603c]
  cargo:
  __asm push	eax
  __asm push	1
  __asm call	CheckCargoSpace
  __asm test	eax, eax
  __asm jnz	ok
  __asm mov	eax, 1168
  __asm jmp	full
}

DWORD test_quantity = (DWORD)SetMaxQuantity3;


MarketGoodInfo* old_market;
LPVOID market_iter, market_end;

NAKED
MarketGoodInfo* FirstMarketGood( LPVOID market )
{
  __asm mov	eax, [esp+4] // market
  __asm mov	edx, [eax+4]
  __asm mov	ecx, [edx]
  __asm mov	market_iter, ecx
  __asm mov	market_end, edx
  __asm xor	eax, eax
  __asm cmp	ecx, edx
  __asm je	done
  __asm lea	eax, [ecx+0x10]
  done:
  __asm ret
}

NAKED
MarketGoodInfo* NextMarketGood()
{
  __asm mov	eax, 0x438930
  __asm lea	ecx, market_iter
  __asm call	eax
  __asm xor	eax, eax
  __asm mov	ecx, market_iter
  __asm cmp	ecx, market_end
  __asm je	done
  __asm lea	eax, [ecx+0x10]
  done:
  __asm ret
}


bool equip_first( const EquipDesc& a, const EquipDesc& b )
{
  return (!a.is_internal() && b.is_internal());
}


// Translate the market back to loadout and restore the original market.
void Xfer_close( bool idle, std::vector<EquipDesc>* loadout )
{
  MarketGoodInfo* good;

  if (!idle)
  {
    // Sort the equipment list, placing mounted items before unmounted, so we
    // transfer cargo before equipment.
    std::stable_sort( loadout->begin(), loadout->end(), equip_first );

    for (int i = 0; i < loadout->size(); ++i)
    {
      good = FindMarketGood( market, (*loadout)[i].iArchID );
      if (good)
      {
	if (good->quantity == 0)
	{
	  loadout->erase( loadout->begin() + i );
	  --i;
	}
	else
	{
	  if (arch_is_combinable( good->arch ))
	  {
	    (*loadout)[i].iCount = good->quantity;
	    good->quantity = 0;
	  }
	  else
	    --good->quantity;
	}
      }
    }
  }

  EquipDesc equip;
  MarketGoodInfo* item = old_market;
  for (good = FirstMarketGood( market ); good; good = NextMarketGood())
  {
    if (!idle && good->quantity)
    {
      equip.iArchID = good->arch;
      if (arch_is_combinable( equip.iArchID ))
      {
	equip.iCount = good->quantity;
	loadout->push_back( equip );
      }
      else
      {
	equip.iCount = 1;
	do
	  loadout->push_back( equip );
	while (--good->quantity);
      }
    }
    *good = *item++;
  }

  delete[] old_market;

  *ADDR_EQUIPCOMM  = 0x94;
  *ADDR_COMMICON   = 0x03;
  *ADDR_PLAYERCASH = 0x58;
  *ADDR_DEALERCNT1 = 0x74;
  *ADDR_DEALERCNT2 = 0x75;
  *ADDR_PRICE0	   = 0x5E;
  *ADDR_FREEAMMO   = 0x8E0F;
  *ADDR_TIP	   = 2;
  *ADDR_DEALER	   = 0x2143;
  *ADDR_LINE	   = 0x10;
  *ADDR_TITLE	   = 1164;
  *ADDR_HELP	   = 1149;
  *ADDR_TITLE1	   = 0x84;
  *ADDR_BUY	   = 3016;
  *ADDR_SELL	   = 3017;
  *ADDR_QUANT1	   =
  *ADDR_QUANT2	   =
  *ADDR_QUANT3	   =
  *ADDR_QUANT4	   = 0.384f;
  *ADDR_QUANT5	   = 0.05f;
  *ADDR_ITEMMAX    = 0xF7F7;
  *ADDR_CARGOMSG   = 1;

  *ADDR_MAXQUANT = 0x89;
  *(PDWORD)(ADDR_MAXQUANT+1) = 0x00038c98;
  ADDR_MAXQUANT[5] = 0x00;

  *(PDWORD)ADDR_MAXQUANT1 = 0x688b0374;
  ADDR_MAXQUANT1[4] = 0x14;

  *ADDR_MAXQUANT2 = 0x74;
  *(PDWORD)(ADDR_MAXQUANT2+1) = 0x14408b09;

  *ADDR_MAXQUANT3 = 0x5c603c;

  *ADDR_XFERBUY = 0x2b;
  *(PDWORD)(ADDR_XFERBUY+1) = 0x1c4389c7;

  *(PUSHORT)ADDR_XFERSELL    = 0x7501;
  *(PDWORD)(ADDR_XFERSELL+2) = 0x04478b1c;
}


// Transfer equipment between the current and selected ship by replacing the
// market with the loadout.
bool cmdXfer( LPCWSTR opt )
{
  if (InSpace() || ship_storage.empty())
    return false;

  pub::Player::GetShipID( player, ship );
  if (ship == shipless_id)
  {
    if (*opt)
      msg.strid( IDS(DRIVE_A_SHIP) );
    else
      ListShips();
    return true;
  }

  if (!IsShipHealthy())
    return true;

  ShipStorageIter iter = FindShip( (LPWSTR)opt );
  if (iter == ship_storage.end())
    return true;

  market = fl1->GetMarket( base );	// base set by FindShip
  old_market = new MarketGoodInfo[*((int*)market+4)]; // market->size()

  // Backup and reset the market data.
  MarketGoodInfo* item = old_market;
  MarketGoodInfo* good;
  for (good = FirstMarketGood( market ); good; good = NextMarketGood())
  {
    *item++ = *good;
    good->price = good->rank = 0;
    good->rep = -1;
    good->quantity = 0;
    good->mounts = 0;
  }

  Archetype::Ship* aship = Archetype::GetShip( iter->ship );
  remaining_cargo = aship->hold_size;
  max_bots = aship->nanobots;
  max_bats = aship->batteries;

  // Turn the loadout into market and find cargo size.
  for (int i = iter->loadout.equip.size(); --i >= 0;)
  {
    good = FindMarketGood( market, iter->loadout.equip[i].iArchID );
    if (good)
    {
      good->quantity += iter->loadout.equip[i].iCount;
      if (iter->loadout.equip[i].is_internal())
	remaining_cargo -= iter->loadout.equip[i].get_cargo_space_occupied();
      else
	++good->mounts;
    }
  }

  market_bats = &FindMarketGood( market, *pbats_id )->quantity;
  market_bots = &FindMarketGood( market, *pbots_id )->quantity;

  *ADDR_EQUIPCOMM  = 0x96;	// search for equipment & commodity
  *ADDR_COMMICON   = 0x02;	// show the commodity icon
  *ADDR_DEALERCNT1 =		// display item count for dealer
  *ADDR_DEALERCNT2 = 0xEB;	//  even for equipment
  *ADDR_PRICE0	   = 0x00;	// display item even if price is zero
  *ADDR_FREEAMMO   = 0xE990;	// don't include free ammo
  *ADDR_TIP	   = 3; 	// Transfer, not Buy/Sell
  *ADDR_DEALER	   = aship->strid; // use ship name instead of "DEALER"
  *ADDR_LINE	   = 0x00;	// position the dividing line
  *ADDR_TITLE	   = IDS(TRANSFER);
  *ADDR_HELP	   = IDS(TRANSFER)+1;
  *ADDR_TITLE1	   = 0x85;	// title is only a single line
  *ADDR_BUY	   =		// "TRANSFER"
  *ADDR_SELL	   = 1162;
  *ADDR_PLAYERCASH = 0x62;	// don't display credits
  *ADDR_QUANT1	   =		// Price:
  *ADDR_QUANT2	   =		// Total:
  *ADDR_QUANT3	   =		// $
  *ADDR_QUANT4	   =		// $
  *ADDR_QUANT5	   = 0; 	// plus/minus
  *ADDR_ITEMMAX    = 0xC089;	// maximum number of an item you can afford
  *ADDR_CARGOMSG   = 4; 	// "move" instead of "buy"

  CALL( ADDR_MAXQUANT,	SetMaxQuantity	);
  CALL( ADDR_MAXQUANT1, SetMaxQuantity1 );
  CALL( ADDR_MAXQUANT2, SetMaxQuantity2 );

  *ADDR_MAXQUANT3 = (DWORD)&test_quantity;

  // Adjust remaining cargo space after a transfer.
  CALL( ADDR_XFERBUY,  XferBuy_Hook  );
  CALL( ADDR_XFERSELL, XferSell_Hook );
  ADDR_MAXQUANT[5] = ADDR_XFERSELL[5] = 0x90;

  CloseDialog();
  DealerDialog( EQUIPMENT_DEALER, (PROC)Xfer_close,
		(LPVOID)&iter->loadout.equip, 0 );

  return false;
}


// Sell a ship in storage.
bool cmdSell( LPCWSTR opt )
{
  if (InSpace() || ship_storage.empty())
    return false;

  if (!*opt)
  {
    ListShips();
    return true;
  }

  ShipStorageIter iter = FindShip( (LPWSTR)opt );
  if (iter == ship_storage.end())
    return true;

  if (!IsShipHealthy())
    return true;

  // Remember the current ship.
  ShipStatus status;
  /*
  status.hull = 1;		// in case it's the dummy ship
  */
  GetShipStatus( status );

  // Get the value of the stored ship.
  SetShipStatus( *iter );
  float sale;
  pub::Player::GetAssetValue( player, sale );
  /*
  // The above doesn't take ship damage into account, so use the function in
  // Freelancer.exe that does.	With false, it gets the value of the ship, but
  // without depreciation, so adjust for that, too.
  sale -= ShipValue( ship, false ) * *(float*)0x5d6d38;
  sale += ShipValue( ship, true );
  */
  pub::Player::AdjustCash( player, sale );

  // Restore the current ship.
  if (status.ship == shipless_id)
    pub::Player::SetShipAndLoadout( player, shipless_id, empty_loadout );
  else
    SetShipStatus( status );

  WCHAR name[64], price[64];
  Archetype::Ship* aship = Archetype::GetShip( iter->ship );
  GetString( RSRC, aship->strid, name, 64 );
  FmtCredits( price, sale, -1 );
  msg.printf( IDS(SOLD_FOR), name, price );

  // Take it out of storage.
  ship_storage.erase( iter );

  return true;
}


// Hitch a ride to another base.
bool cmdHitch( LPCWSTR wBase )
{
  if (InSpace())
    return false;

  pub::Player::GetShipID( player, ship );
  if (ship != shipless_id)
    cmdPark( 0 );

  if (*wBase)
    return cmdBase( wBase );

  UINT cur_base;
  pub::Player::GetBase( player, cur_base );
  ShipStorageIter iter = ship_storage.begin(), end = ship_storage.end();
  bool okay = true;
  for (base = 0; iter != end; ++iter)
  {
    if (iter->base == cur_base)
      continue;
    if (base)
    {
      if (iter->base != base)
      {
	okay = false;
	break;
      }
    }
    else
      base = iter->base;
  }
  if (okay && base)
  {
    ChangeBase();
    return false;
  }

  msg.strid( IDS(SPECIFY_BASE) );
  return true;
}


// Save all the ships in storage.
void STDCALL save_ships( LPCSTR name )
{
  char names[MAX_PATH];
  sprintf( names, "%ss", name );
  if (ship_storage.empty())
  {
    DeleteFile( names );
    return;
  }
  FILE* file = fopen( names, "w" );
  if (!file)
    return;

  ShipStorageIter iter = ship_storage.begin(), end = ship_storage.end();
  for (; iter != end; ++iter)
  {
    // Use [Player] simply so FlCodec can decode it.
    fprintf( file, "[Player]\n"
		   "base = %u\n"
		   "ship_archetype = %u\n",
		   iter->base, iter->ship );

    /*
    if (iter->hull != 1)
      fprintf( file, "hull_status = %g\n", iter->hull );

    Archetype::Ship* aship = Archetype::GetShip( iter->ship );
    Archetype::EqObj* eqobj = (Archetype::EqObj*)aship;
    CollisionGroupDescListIter cg = iter->cg.begin(), cge = iter->cg.end();
    for (; cg != cge; ++cg)
    {
      const Archetype::CollisionGroup* acg = eqobj->get_group_by_id( cg->id );
      fprintf( file, "collision_group = %s, %g\n",
	       acg->name.str, cg->health );
    }
    */

    int size = iter->loadout.equip.size();
    for (int i = 0; i < size; ++i)
    {
      if (iter->loadout.equip[i].bMounted)
      {
	fprintf( file, "equip = %u, %s, %g\n",
		 iter->loadout.equip[i].iArchID,
		 (iter->loadout.equip[i].is_internal())
		 ? "" : iter->loadout.equip[i].szHardPoint.str,
		 iter->loadout.equip[i].fHealth );
      }
      else
      {
	fprintf( file, "cargo = %u, %d, , ",
		 iter->loadout.equip[i].iArchID,
		 iter->loadout.equip[i].iCount );
	if (iter->loadout.equip[i].fHealth != 1)
	  fprintf( file, "%g", iter->loadout.equip[i].fHealth );
	fprintf( file, ", %d\n", iter->loadout.equip[i].bMission );
      }
    }

    fputc( '\n', file );

    if (iter->wg[0] != '\n' && iter->wg[0] != '\0')
      fprintf( file, "%s", iter->wg.c_str() );
  }

  fclose( file );
}


// This intercepts the save game routine when it gets the rank for SP.
NAKED
void save_ships_hook()
{
  __asm push	dword ptr [esp+0x1c]
  __asm call	save_ships
  __asm mov	edx, [ebx+0x284]
  __asm ret
}


// Load the ships into storage.
void STDCALL load_ships( LPCSTR name )
{
  ship_storage.clear();

  char names[512];
  char path[MAX_PATH];
  GetUserDataPath( path );
  sprintf( names, "%s\\Accts\\SinglePlayer\\%ss", path, name );
  INI_Reader ini;
  if (!ini.open( names ))
    return;

  while (ini.read_header())
  {
    ShipStatus status;
    EquipDesc  equip;
    while (ini.read_value())
    {
      if (ini.is_value( "base" ))
      {
	// May as well allow the nickname, too.
	status.base = ini.get_value_int( 0 );
	if (status.base == 0)
	  status.base = CreateID( ini.get_value_string( 0 ) );
      }
      else if (ini.is_value( "ship_archetype" ))
      {
	// May as well allow the nickname, too.
	status.ship = ini.get_value_int( 0 );
	if (status.ship == 0)
	  status.ship = CreateID( ini.get_value_string( 0 ) );
      }
      else if (ini.is_value( "equip" ))
      {
	if (Loadout::ReadEquipLine( ini, equip ))
	  status.loadout.equip.push_back( equip );
      }
      else if (ini.is_value( "cargo" ))
      {
	if (Loadout::ReadCargoLine( ini, equip ))
	  status.loadout.equip.push_back( equip );
      }
      else if (ini.is_value( "wg" ))
      {
	status.wg += ini.get_line_ptr();
	status.wg += '\n';
      }
    }
    ship_storage.push_back( status );
  }

  ini.close();
}


// Intercepts the call used to load a game.
NAKED
void load_ships_hook()
{
  __asm push	ecx
  __asm push	esi
  __asm call	load_ships
  __asm pop	ecx
  __asm push	0x5ab250
  __asm ret
}


// This replaces the _unlink function used to delete a game.
void del_ships_hook( LPCSTR name )
{
  char names[MAX_PATH];
  sprintf( names, "%ss", name );
  DeleteFile( name );
  DeleteFile( names );
}


float ntb_dist;

bool cmdNTB( LPCWSTR opt )
{
  if (*opt)
  {
    int len = wcslen( opt );
    if (wcsnicmp( L"off", opt, len ) == 0 && len >= 2)
    {
      *ADDR_NTBLOOP = 0xe990;
    }
    else if (wcsicmp( L"on", opt ) == 0)
    {
      *ADDR_NTBLOOP = 0x8e0f;
      *ADDR_NTBDIST = ntb_dist;
    }
    else if (wcsnicmp( L"sight", opt, len ) == 0)
    {
      *ADDR_NTBLOOP = 0x8e0f;
      *ADDR_NTBDIST = 1;
    }
    else
      return false;
  }

  msg.strid( (*ADDR_NTBLOOP == 0xe990) ? IDS(NTB_OFF) :
	     IDS(NTB_ON) + (*ADDR_NTBDIST == 1) );

  return true;
}


bool cmdShow( LPCWSTR opt )
{
  static const BYTE code[] =
  {
    0x8b,0x4C,0x24,0x08,	// mov ecx, [esp+8]
    0x80,0x09,0x01,		// or  [ecx], 1 	; set visited flag
    0xc3			// ret
  };

  UINT show;
  int len = wcslen( opt );
  if (!len)
  {
    if (*ADDR_VISIT == 0xc3)
    {
      memcpy( ADDR_VISIT, code, sizeof(code) );
      show = IDS(SHOW_ON);
    }
    else
    {
      *ADDR_VISIT = 0xc3;
      show = IDS(SHOW_OFF);
    }
  }
  else if (wcsnicmp( L"houses", opt, len ) == 0)
  {
    if (*ADDR_HOUSES == 0x8e8b)
    {
      *ADDR_HOUSES = 0x4eeb;
      show = IDS(HOUSES_OFF);
    }
    else
    {
      *ADDR_HOUSES = 0x8e8b;
      show = IDS(HOUSES_ON);
    }
  }
  else if (wcsnicmp( L"paths", opt, len ) == 0)
  {
    if (*ADDR_PATHS == 0x850f)
    {
      *ADDR_PATHS = 0xe990;
      show = IDS(PPATHS_UNFILTERED);
    }
    else
    {
      *ADDR_PATHS = 0x850f;
      show = IDS(PPATHS_FILTERED);
    }
  }
  else
    return false;

  msg.strid( show );
  return true;
}


bool cmdZoom( LPCWSTR val )
{
  float speed;

  if (*val)
  {
    speed = (float)wcstod( val, 0 );
    if (speed == 0)
      speed = 0.001f;
    *ADDR_ZOOMOUT = *ADDR_ZOOMIN = speed;
  }
  else
    speed = *ADDR_ZOOMOUT;

  msg.printf( IDS(ZOOM_TIME), speed );
  return true;
}


bool cmdCash( LPCWSTR val )
{
  int cash;

  if (*val)
  {
    bool rel = (*val == '+' || *val == '-');
    LPWSTR end;
    int amount = wcstol( val, &end, 10 );
    if (*end == 'k' || *end == 'K')
      amount *= 1000;
    if (!rel)
    {
      pub::Player::InspectCash( player, cash );
      amount -= cash;
    }
    pub::Player::AdjustCash( player, amount );
  }

  pub::Player::InspectCash( player, cash );
  FmtCredits( wstrbuf, cash, -1 );
  msg.string( wstrbuf );
  return true;
}


// Find the faction from the word starting at OPT, setting wstrbuf to its
// short name; -1 if none, with msg containing a suitable error message.
// OPT is updated to the next word.
int FindFaction( LPCWSTR& opt )
{
  LPCWSTR wRep;
  char	  aRep[128];
  int	  faclen;
  UINT	  fac;

  wRep = opt;
  faclen = 0;
  while (*opt && *opt != ' ')
    aRep[faclen++] = (char)*opt++;	// nicknames are straight ASCII
  aRep[faclen] = '\0';

  while (*opt == ' ')
    ++opt;

  if (!aRep[1])
  {
    switch (*aRep | 0x20)
    {
      case 'l': strcpy( aRep, "li_p_grp" ); break;
      case 'b': strcpy( aRep, "br_p_grp" ); break;
      case 'k': strcpy( aRep, "ku_p_grp" ); break;
      case 'r': strcpy( aRep, "rh_p_grp" ); break;
    }
  }

  pub::Reputation::GetReputationGroup( fac, aRep );
  if (fac == -1)
  {
    struct RepNameEnumerator : public pub::Reputation::Enumerator
    {
      UINT    faction;
      LPCWSTR partial;
      int     partlen;
      int     count;

      bool operator()( UINT fac )
      {
	WCHAR facstr[64];
	UINT  strid;
	pub::Reputation::GetShortGroupName( fac, strid );
	GetString( RSRC, strid, facstr, 64 );
	if (wcsnicmp( facstr, partial, partlen ) == 0 &&
	    (Reputation::get_info_card( fac ) != 66205 ||   // ignore fc_?n_grp
	     fac == 25320))				    // but not fc_n_grp
	{
	  faction = fac;
	  if (--count <= 0)
	    return false;
	}
	return true;
      }
    } factions;
    factions.faction = 0;
    factions.partial = wRep;
    factions.count   = GetCount( (LPWSTR)wRep );
    factions.partlen = faclen;
    pub::Reputation::EnumerateGroups( factions );
    if (!factions.faction)
    {
      msg.strid( IDS(FACTION_NOT_FOUND) );
      return -1;
    }
    fac = factions.faction;
  }

  UINT strid;
  pub::Reputation::GetShortGroupName( fac, strid );
  GetString( RSRC, strid, wstrbuf, *wstrlen );

  return fac;
}


bool GetRep( LPCWSTR opt, float& val )
{
  bool rc = true;

  switch (*opt)
  {
    case 'H': val = -0.9f; break;
    case 'h': val = -0.6f; break;
    case 'n':
      switch (*++opt)
      {
	case 'h':  val = -0.2f; break;
	case '\0': val =  0.0f; return true;
	case 'f':  val =  0.2f; break;
	default:   rc = false;	break;
      }
      break;
    case 'f': val = 0.6f; break;
    case 'F': val = 0.9f; break;

    default:  rc = false;
  }
  if (rc)
    return (opt[1] == '\0');

  LPWSTR end;
  val = (float)wcstod( opt, &end );
  if (val < -1)
  {
    val = (val - 0.000004f) / 100;
    if (val < -1)
      val = -1;
  }
  else if (val > 1)
  {
    // Damn rounding errors (0.6f == 0.60000002, but 60f/100 == 0.59999996).
    val = (val + 0.000004f) / 100;
    if (val > 1)
      val = 1;
  }
  return (*end == '\0');
}


// Let's assume the enumerator uses the same order, so it's only necessary to
// preserve the value.
std::vector<float> cur_rep;

bool cmdRep( LPCWSTR opt )
{
  float val;
  int	rep;
  UINT	fac;
  UINT	col;

  if (!*opt)
    return false;

  pub::Player::GetRep( player, rep );

  if (wcsicmp( opt, L"undo" ) == 0)
  {
    if (cur_rep.empty())
      return false;
    struct AllRep : public pub::Reputation::Enumerator
    {
      int rep;
      int idx;
      bool operator()( UINT fac )
      {
	pub::Reputation::SetReputation( rep, fac, cur_rep[idx++] );
	return true;
      }
    } factions;
    factions.rep = rep;
    factions.idx = 0;
    pub::Reputation::EnumerateGroups( factions );
    cur_rep.clear();
    msg.strid( IDS(REP_LOADED) );
    return true;
  }

  if (*opt == '=' && (opt[1] == ' ' || opt[1] == '\0'))
  {
    msg.clear();
    bool rc = false;
    if (cur_rep.empty())
    {
      struct AllRep : public pub::Reputation::Enumerator
      {
	int rep;
	bool operator()( UINT fac )
	{
	  float val;
	  pub::Reputation::GetReputation( rep, fac, val );
	  cur_rep.push_back( val );
	  return true;
	}
      } factions;
      factions.rep = rep;
      pub::Reputation::EnumerateGroups( factions );
      msg.style( STYLE_NOTICE );
      msg.strid( IDS(REP_SAVED) );
      rc = true;
    }
    while (*++opt == ' ') ;
    if (!*opt)
      return rc;
    if (GetRep( opt, val ))
    {
      struct AllRep : public pub::Reputation::Enumerator
      {
	int   rep;
	float val;
	bool operator()( UINT fac )
	{
	  pub::Reputation::SetReputation( rep, fac, val );
	  return true;
	}
      } factions;
      factions.rep = rep;
      factions.val = val;
      pub::Reputation::EnumerateGroups( factions );
      if (rc)
	msg.para();
      GetRepCol( &col, val );
      msg.TRA( (col << 8) | TRA_Bold, TRA_Color | TRA_Bold );
      msg.printf( IDS(REP), val );
      return true;
    }

    if (rc)
      msg.para();

    fac = FindFaction( opt );
    if (fac != -1)
    {
      struct GroupRep : public pub::Reputation::Enumerator
      {
	int  rep;
	UINT grp;
	bool operator()( UINT fac )
	{
	  float val;
	  pub::Reputation::GetReputation( fac, grp, val );
	  pub::Reputation::SetReputation( rep, fac, val );
	  return true;
	}
      } factions;
      factions.rep = rep;
      factions.grp = fac;
      pub::Reputation::EnumerateGroups( factions );
      // Get the friendly color directly.
      col = *(PUINT)0x679bac;
      msg.TRA( (col << 8) | TRA_Bold, TRA_Color | TRA_Bold );
      msg.string( wstrbuf );
    }
    return true;
  }

  fac = FindFaction( opt );
  if (fac == -1)
    return true;

  if (*opt && GetRep( opt, val ))
    pub::Reputation::SetReputation( rep, fac, val );

  pub::Reputation::GetReputation( rep, fac, val );
  swprintf( wstrbuf + wcslen( wstrbuf ), L": %g", val );
  msg.clear();
  GetRepCol( &col, val );
  msg.TRA( (col << 8) | TRA_Bold, TRA_Color | TRA_Bold );
  msg.string( wstrbuf );
  return true;
}


bool cmdMonkey( LPCWSTR )
{
  pub::Player::SetMonkey( player );

  msg.strid( IDS(MONKEY) );
  return true;
}


bool cmdRobot( LPCWSTR )
{
  pub::Player::SetRobot( player );

  msg.strid( IDS(ROBOT) );
  return true;
}


bool cmdTrent( LPCWSTR )
{
  pub::Player::SetTrent( player );

  msg.strid( IDS(TRENT) );
  return true;
}


bool cmdCostume( LPCWSTR wcostume )
{
  UINT id = CreateIDW( wcostume );
  const CostumeDescriptions* cd = GetCostumeDescriptions();
  if (!cd->find_costume( id ))
  {
    msg.strid( IDS(COSTUME_NOT_FOUND) );
    return true;
  }
  cd->get_costume( id, Players.playerdata->costume );

  msg.printf( L"%c%s.", towupper( *wcostume ), wcostume+1 );
  return true;
}


bool cmdPlay( LPCWSTR nickname )
{
  LPVOID snd = CreateSound( CreateIDW( nickname ) );
  if (snd == NULL)
  {
    msg.strid( IDS(SOUND_NOT_FOUND) );
    return true;
  }
  PlaySound( snd, 0, 0 );
  return false;
}


typedef bool (*TCmd)( LPCWSTR );

struct
{
  LPCWSTR cmd;
  TCmd	  func;
} cmdlist[] = {
  { L"about",    cmdAbout    },
  { L"anim",     cmdAnim     },
  { L"al",       cmdAutoLoad },
  { L"as",       cmdAutoSave },
  { L"autoload", cmdAutoLoad },
  { L"autosave", cmdAutoSave },
  { L"base",     cmdBase     },
  { L"cacc",     cmdCAcc     },
  { L"cash",     cmdCash     },
  { L"costume",  cmdCostume  },
  { L"cspd",     cmdCSpd     },
  { L"drive",    cmdDrive    },
  { L"ghost",    cmdGhost    },
  { L"godmode",  cmdGodmode  },
  { L"help",     cmdHelp     },
  { L"hitch",    cmdHitch    },
  { L"jump",     cmdJump     },
  { L"killself", cmdKillSelf },
  { L"killtarget", cmdKillTarget },
  { L"l",        cmdLaunch   },
  { L"launch",   cmdLaunch   },
  { L"load",     cmdLoad     },
  { L"monkey",   cmdMonkey   },
  { L"ntb",      cmdNTB      },
  { L"park",     cmdPark     },
  { L"ping",     cmdPing     },
  { L"play",     cmdPlay     },
  { L"pos",      cmdPos      },
  { L"posr",     cmdPosR     },
  { L"print",    cmdPrint    },
  { L"rep",      cmdRep      },
  { L"robot",    cmdRobot    },
  { L"rot",      cmdRot      },
  { L"rotr",     cmdRotR     },
  { L"s",        cmdSystem   },
  { L"save",     cmdSave     },
  { L"say",      cmdPrint    },
  { L"sell",     cmdSell     },
  { L"ships",    cmdShips    },
  { L"show",     cmdShow     },
  { L"system",   cmdSystem   },
  { L"trent",    cmdTrent    },
  { L"xfer",     cmdXfer     },
  { L"zoom",     cmdZoom     },
};

#define CMDS (sizeof(cmdlist)/sizeof(*cmdlist))


bool STDCALL Console( PCHAR& rdl, int& rlen )
{
  if (!SinglePlayer())
    return true;

  LPCWSTR cmd = (LPCWSTR)0x66d4f2;
  //int len = *(int*)0x66d4ee / sizeof(WCHAR) - 1;
  int cmdlen, idx;
  int i;

  LPCWSTR opt;
  cmdlen = 0;
  for (opt = cmd; *opt != ' ' && *opt != '\0'; ++opt)
    ++cmdlen;

  idx = -1;
  i = 0;
  WCHAR ch = towlower( *cmd );
  while (*cmdlist[i].cmd < ch)
  {
    if (++i == CMDS)
      return false;
  }
  while (*cmdlist[i].cmd == ch)
  {
    int clen = wcslen( cmdlist[i].cmd );
    if (clen >= cmdlen && wcsnicmp( cmdlist[i].cmd, cmd, cmdlen ) == 0)
    {
      if (cmdlen == clen)
      {
	idx = i;
	break;
      }
      if (idx != -1)
	return false;
      idx = i;
    }
    if (++i == CMDS)
      break;
  }
  if (idx == -1)
    return false;

  while (*opt == ' ')
    ++opt;

  msg.clear();
  msg.style( STYLE_NOTICE );

  if (!cmdlist[idx].func( opt ))
    return false;

  rdl  = msg.brdl;
  rlen = msg.size;
  return true;
}


NAKED
void Chat_Hook()
{
  __asm lea	ecx, [esp+8]
  __asm lea	edx, [esp+12]
  __asm push	edx
  __asm push	ecx
  __asm call	Console
  __asm test	al, al
  __asm jz	done
  __asm push	0x46a150
  done:
  __asm ret
}


// Improve editing of the chat line (actually input in general).

struct ChatChar
{
  wchar_t ch;
  USHORT  u1;
  UINT	  u2;
  UINT	  u3;
};

bool operator==( const ChatChar& lhs, const ChatChar& rhs )
{
  return (lhs.ch == rhs.ch);
}


typedef std::vector<ChatChar> VChatChar;
typedef std::list<VChatChar> LVChatChar;
typedef LVChatChar::iterator LVChatCharI;

DWORD Bksp_Org, Left_Org, Right_Org, Del_Org, Home_Org, End_Org;
DWORD Up_Org, Down_Org, Rturn_Org;

LVChatChar  chat_history;
LVChatCharI history_pos;
bool	    history_begin;


inline bool iswword( wchar_t ch )
{
  return (ch == '_' || iswalnum( ch ));
}


void STDCALL WordLeft( const VChatChar& text, int& pos )
{
  int p = pos;

  if (p > text.size())
    p = text.size();
  if (--p <= 0)
    return;

  // Skip all spaces to find the end of the previous word.
  while (p >= 0 && !iswword( text[p].ch ))
    --p;

  // Find the start of the word.
  while (p >= 0 && iswword( text[p].ch ))
    --p;

  // Point to the second character, the normal left will point to the first.
  pos = p + 2;
}


void STDCALL WordRight( const VChatChar& text, int& pos )
{
  int p = pos;

  if (p < 0)
    p = 0;
  if (++p >= text.size())
    return;

  // Skip the current word.
  while (p < text.size() && iswword( text[p].ch ))
    ++p;

  // Skip the spaces to find the next word.
  while (p < text.size() && !iswword( text[p].ch ))
    ++p;

  // Point to the space, the normal right will then point to the word.
  pos = p - 1;
}


void STDCALL DelWordLeft( VChatChar& text, int& pos )
{
  if (pos <= 0)
    return;

  int p = pos;
  WordLeft( text, pos );
  text.erase( text.begin() + --pos, text.begin() + p );
}


void STDCALL DelWordRight( VChatChar& text, int& pos )
{
  if (pos >= text.size())
    return;

  int p = pos;
  WordRight( text, p );
  text.erase( text.begin() + pos, text.begin() + p+1 );
}


void STDCALL DelHome( VChatChar& text, int& pos )
{
  text.erase( text.begin(), text.begin() + pos );
  pos = 0;
}


void STDCALL DelEnd( VChatChar& text, int& pos )
{
  text.erase( text.begin() + pos, text.end() );
}


inline bool make_history( const VChatChar& text )
{
  return (SinglePlayer() ||
	  (!text.empty() &&
	   (text.front().ch == '/' || text.front().ch == '.')));
}


bool match_prefix( int len, const VChatChar& prefix, const VChatChar& text )
{
  if (text.size() < len)
    return false;
  while (--len >= 0)
    if (prefix[len].ch != text[len].ch)
      return false;
  return true;
}


void STDCALL Store_History( const VChatChar& text )
{
  if (!make_history( text ) || text.size() < 2)
    return;

  // If it already exists, remove it.
  LVChatCharI i = std::find( chat_history.begin(), chat_history.end(), text );
  if (i != chat_history.end())
    chat_history.erase( i );

  chat_history.push_back( text );
  history_pos = chat_history.begin();
}


DWORD STDCALL Prev_History( VChatChar& text, int& pos, int flag )
{
  if (!make_history( text ) && !(flag & 4))
    return Up_Org;

  // Only the blank line is in the history, do nothing.
  if (chat_history.size() == 1)
    return 0x45f811;

  LVChatCharI history_start = history_pos;
  while (true)
  {
    if (history_pos == chat_history.begin())
      history_pos = chat_history.end();
    --history_pos;
    if (flag & 4)
    {
      if (match_prefix( pos, text, *history_pos ))
	break;
      if (history_pos == history_start)
	return 0x45f811;
      continue;
    }
    // Don't go back to the blank line in MP.
    if (SinglePlayer() || history_pos != chat_history.begin())
      break;
  }
  text = *history_pos;
  if (!(flag & 4))
    pos = text.size();

  return 0x45f80a;
}


DWORD STDCALL Next_History( VChatChar& text, int& pos, int flag )
{
  if (!make_history( text ) && !(flag & 4))
    return Down_Org;

  // Only the blank line is in the history, do nothing.
  if (chat_history.size() == 1)
    return 0x45f811;

  LVChatCharI history_start = history_pos;
  while (true)
  {
    if (++history_pos == chat_history.end())
      history_pos = chat_history.begin();
    if (flag & 4)
    {
      if (match_prefix( pos, text, *history_pos ))
	break;
      if (history_pos == history_start)
	return 0x45f811;
      continue;
    }
    // Don't go back to the blank line in MP.
    if (SinglePlayer() || history_pos != chat_history.begin())
      break;
  }
  text = *history_pos;
  if (!(flag & 4))
    pos = text.size();

  return 0x45f80a;
}


NAKED
void Left_Hook()
{
  __asm {
	test	byte ptr [edi+8], 4	// Control
	jz	done
	mov	eax, [esi+0x52c]
	test	eax, eax
	jnz	done
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	WordLeft
  done:
	jmp	Left_Org
  }
}


NAKED
void Right_Hook()
{
  __asm {
	test	byte ptr [edi+8], 4	// Control
	jz	done
	mov	eax, [esi+0x52c]
	test	eax, eax
	jnz	done
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	WordRight
  done:
	jmp	Right_Org
  }
}


NAKED
void Bksp_Hook()
{
  __asm {
	test	byte ptr [edi+8], 4	// Control
	jz	done
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	DelWordLeft
	push	0x57cd68
	ret
  done:
	jmp	Bksp_Org
  }
}


NAKED
void Del_Hook()
{
  __asm {
	test	byte ptr [edi+8], 4	// Control
	jz	done
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	DelWordRight
	push	0x57cd68
	ret
  done:
	jmp	Del_Org
  }
}


NAKED
void Home_Hook()
{
  __asm {
	test	byte ptr [edi+8], 4	// Control
	jz	done
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	DelHome
	push	0x57cd68
	ret
  done:
	jmp	Home_Org
  }
}


NAKED
void End_Hook()
{
  __asm {
	test	byte ptr [edi+8], 4	// Control
	jz	done
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	DelEnd
	push	0x57cd68
	ret
  done:
	jmp	End_Org
  }
}


NAKED
void Rturn_Hook()
{
  __asm {
	lea	ecx, [esi+0x4c4]
	push	ecx
	call	Store_History
	jmp	Rturn_Org
  }
}


NAKED
void Up_Hook()
{
  __asm {
	push	[ecx+8]
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	Prev_History
	jmp	eax
  }
}


NAKED
void Down_Hook()
{
  __asm {
	push	[ecx+8]
	lea	edx, [esi+0x49c]
	lea	ecx, [esi+0x4c4]
	push	edx
	push	ecx
	call	Next_History
	jmp	eax
  }
}


// Since using Windows 7, chat (and input in general) now has a flashing 'A'.
// I put this down to using CP936 (for testing other software), but it remained
// when I switched back to CP437.  Since it disables left and right, test for
// a DBCS code page, and disable it if not (wasn't able to find out why it
// didn't occur when I was using XP).
DWORD STDCALL Ime_Test( DWORD whatever )
{
  CPINFO cpi;

  if (strstr( GetCommandLine(), "NoIme" ) != NULL)
    return 0;

  if (GetCPInfo( CP_ACP, &cpi ) && cpi.MaxCharSize == 1)
    return 0;

  return whatever;
}


NAKED
void Ime_Hook()
{
  __asm {
	test	eax, eax
	jz	done
	push	eax
	call	Ime_Test
  done:
	mov	[esi+0x52c], eax
	ret
  }
}


void STDCALL New_Game( bool MP )
{
  if (MP)
  {
    EngineEquipConsts::CRUISING_SPEED	 = mp_cspd;
    EngineEquipConsts::CRUISE_ACCEL_TIME = mp_cacc;
  }
  else
  {
    EngineEquipConsts::CRUISING_SPEED	 = sp_cspd;
    EngineEquipConsts::CRUISE_ACCEL_TIME = sp_cacc;
    *chat_channel = 0;
  }
}


DWORD old_new_game;

NAKED
void New_Game_Hook()
{
  __asm push	[esp+4]
  __asm call	New_Game
  __asm jmp	old_new_game
}

DWORD new_new_game = (DWORD)New_Game_Hook;


int startup( LPCSTR file, Archetype::ICliObjFactory* obj, LPVOID cb )
{
  int rc = old_startup( file, obj, cb );

  // Create a dummy ship.  This method provides protection from mods.
  // Originally done as part of Patch, but that would crash multiplayer.
  shipless_id = CreateID( "shipless_ship" );
  char buf[MAX_PATH];
  GetTempPath( MAX_PATH-14, buf );
  strcat( buf, "shipless.ini" );
  HANDLE hFile = CreateFile( buf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			     0, NULL );
  WriteFile( hFile, shipless, sizeof(shipless)-1, &dummy, NULL );
  CloseHandle( hFile );
  Archetype::LoadShips( buf, false, obj );
  DeleteFile( buf );

  mp_cspd = sp_cspd = EngineEquipConsts::CRUISING_SPEED;
  mp_cacc = sp_cacc = EngineEquipConsts::CRUISE_ACCEL_TIME;

  return rc;
}


DWORD old_resource;

NAKED
void Resource_Hook()
{
  __asm push	[esp+4]
  __asm call	old_resource
  __asm add	esp, 4

  __asm mov	eax, ds:[0x67c404]	// get the number of our resource
  __asm sub	eax, ds:[0x67c400]
  __asm shl	eax, 14
  __asm mov	rsrcid, eax
  __asm mov	eax, 0x57d800
  __asm push	g_hinst
  __asm call	eax
  __asm add	esp, 4
  __asm ret
}


void Patch()
{
  ProtectX( ADDR_ENTER1,     1 );
  //ProtectX( ADDR_EQUIPCOMM,  1 );
  ProtectX( ADDR_NAVBAR,     5 );
  ProtectX( ADDR_CUSH0,      4 );
  ProtectX( ADDR_CUSH1,      4 );
  //ProtectX( ADDR_CUSH1a,     6 );
  //ProtectX( ADDR_CUSH2,      5 );
  ProtectX( ADDR_RTURN,      4 );
  //ProtectX( ADDR_UP,	       4 );
  //ProtectX( ADDR_DOWN,       4 );
  ProtectX( ADDR_ENTER,      1 );
  ProtectX( ADDR_MSGWIN,     5 );
  ProtectX( ADDR_COMMICON,   1 );
  //ProtectX( ADDR_PLAYERCASH, 1 );
  ProtectX( ADDR_MAXQUANT,   6 );
  //ProtectX( ADDR_DEALERCNT1, 1 );
  //ProtectX( ADDR_DEALERCNT2, 1 );
  ProtectX( ADDR_MAXQUANT1,  5 );
  //ProtectX( ADDR_PRICE0,     1 );
  //ProtectX( ADDR_XFERBUY,    5 );
  //ProtectX( ADDR_FREEAMMO,   2 );
  //ProtectX( ADDR_XFERSELL,   6 );
  ProtectX( ADDR_TIP,	     1 );
  //ProtectX( ADDR_MAXQUANT2,  5 );
  //ProtectX( ADDR_MAXQUANT3,  4 );
  //ProtectX( ADDR_DEALER,     4 );
  ProtectX( ADDR_LINE,	     1 );
  //ProtectX( ADDR_TITLE,      4 );
  //ProtectX( ADDR_HELP,       4 );
  //ProtectX( ADDR_TITLE1,     1 );
  //ProtectX( ADDR_BUY,        4 );
  //ProtectX( ADDR_SELL,       4 );
  //ProtectX( ADDR_QUANT1,     4 );
  //ProtectX( ADDR_QUANT2,     4 );
  //ProtectX( ADDR_QUANT3,     4 );
  //ProtectX( ADDR_QUANT4,     4 );
  //ProtectX( ADDR_QUANT5,     4 );
  ProtectX( ADDR_ITEMMAX,    2 );
  //ProtectX( ADDR_CARGOMSG,   1 );
  ProtectX( ADDR_JOBS,	     1 );
  ProtectX( ADDR_PP1,	     4 );
  //ProtectX( ADDR_PP2,        4 );
  ProtectX( ADDR_ZOOMOUT,    4 );
  ProtectX( ADDR_PATHS,      2 );
  ProtectX( ADDR_PP3,	     4 );
  ProtectX( ADDR_HOUSES,     2 );
  ProtectX( ADDR_ZOOMIN,     4 );
  ProtectX( ADDR_SHIPID,     5 );
  ProtectX( ADDR_VISIT,      8 );
  ProtectX( ADDR_NTBLOOP,    2 );
  ProtectX( ADDR_GHOSTJ,     6 );
  ProtectX( ADDR_ENTER2,     1 );
  ProtectX( ADDR_ENTER3,     1 );
  //ProtectX( ADDR_ENTER4,     1 );
  //ProtectX( ADDR_ENTER5,     1 );
  ProtectX( ADDR_BKSP,	     4 );
  //ProtectX( ADDR_END,        4 );
  //ProtectX( ADDR_HOME,       4 );
  //ProtectX( ADDR_LEFT,       4 );
  //ProtectX( ADDR_RIGHT,      4 );
  //ProtectX( ADDR_DEL,        4 );
  ProtectX( ADDR_CHAT,	     4 );
  ProtectX( ADDR_NEWGAME,    4 );
  ProtectX( ADDR_LOAD,	     4 );
  ProtectX( ADDR_RSRC,	     4 );
  ProtectX( ADDR_STARTUP,    4 );

  ProtectW( ADDR_NTBDIST,    4 );

  ProtectX( ADDR_HARDPOINT,  4 );
  ProtectX( ADDR_SHIPLAUNCH, 4 );
  //ProtectX( ADDR_SHIELD,     1 );

  ProtectX( ADDR_IME,	     6 );

  // Bypass the single-player tests preventing chat.
  *ADDR_ENTER  =
  *ADDR_ENTER1 =
  *ADDR_ENTER2 =
  *ADDR_ENTER3 =
  *ADDR_ENTER4 =
  *ADDR_ENTER5 = 0;

  // Enable the chat output window.
  JMP( ADDR_MSGWIN, 0x46b580 );

  // Hook prior to display.
  RELOFS( ADDR_CHAT, Chat_Hook );

  // Add moving by word.
  Left_Org  = *ADDR_LEFT;
  Right_Org = *ADDR_RIGHT;
  *ADDR_LEFT  = (DWORD)Left_Hook;
  *ADDR_RIGHT = (DWORD)Right_Hook;

  // Add deleting by word.
  Bksp_Org = *ADDR_BKSP;
  Del_Org  = *ADDR_DEL;
  *ADDR_BKSP = (DWORD)Bksp_Hook;
  *ADDR_DEL  = (DWORD)Del_Hook;

  // Delete to begin/end.
  Home_Org = *ADDR_HOME;
  End_Org  = *ADDR_END;
  *ADDR_HOME = (DWORD)Home_Hook;
  *ADDR_END  = (DWORD)End_Hook;

  // Keep a history.
  Rturn_Org = *ADDR_RTURN;
  Up_Org    = *ADDR_UP;
  Down_Org  = *ADDR_DOWN;
  *ADDR_RTURN = (DWORD)Rturn_Hook;
  *ADDR_UP    = (DWORD)Up_Hook;
  *ADDR_DOWN  = (DWORD)Down_Hook;
  chat_history.push_back( VChatChar() );	// always keep an empty line
  history_pos = chat_history.begin();

  // Hook in code to load our dummy ship definition.
  old_startup = (Tstartup)*ADDR_STARTUP;
  *ADDR_STARTUP = (DWORD)startup;

  // Replace the dummy ship id with 0.
  JMP( ADDR_SHIPID, ShipID_Hook );

  // Hook in code to load ourself as a resource DLL.
  old_resource = *ADDR_RSRC + (DWORD)ADDR_RSRC + 4;
  RELOFS( ADDR_RSRC, Resource_Hook );

  // Intercept the save/load/delete game functions to do same for our ships.
  RELOFS( ADDR_LOAD, load_ships_hook );
  PBYTE server	  = (PBYTE)GetModuleHandle( "server.dll" );
  PBYTE save_addr = server + 0x6d275;
  PBYTE del_addr  = server + 0x6a177;
  ProtectX( save_addr, 5 );
  ProtectX( del_addr,  5 );
  CALL( save_addr, save_ships_hook );
  CALL( del_addr,  del_ships_hook  );
  save_addr[5] = del_addr[5] = 0x90;

  // Find out when we're on the launch pad.
  CALL( ADDR_NAVBAR, NavBar_Hook );

  // Handle the ship cushion.
  ResetCushion_Org = *ADDR_CUSH0 + (DWORD)ADDR_CUSH0 + 4;
  RELOFS( ADDR_CUSH0, ResetCushion_Hook );
  SetCushion_Org = *(PDWORD)(*ADDR_CUSH1);
  *ADDR_CUSH1 = (DWORD)&SetCushion;
  CALL( ADDR_CUSH1a, CushionLoop_Hook );
  ADDR_CUSH1a[5] = 0x90;
  CALL( ADDR_CUSH2, UpdateCushion_Hook );

  // Set the protection for the system jump.
  SwitchOut = server + 0xf600;
  ProtectX( SwitchOut+0xd7, 2 );	// covers the lot

  // Double the number of indices available for the patrol paths, provided
  // they haven't already been edited.
  PBYTE rp8 = (PBYTE)GetModuleHandle( "rp8.dll" );
  if (*(PDWORD)(rp8+ADDR_RP8_PP) == 0x4000 && *ADDR_PP1 == 0x17700)
  {
    ProtectX( rp8+ADDR_RP8_PP, 4 );
    *(PDWORD)(rp8+ADDR_RP8_PP) = 0x8000;
    *ADDR_PP1 = 0x2ee00;
    *ADDR_PP2 = 0x11940;
    *ADDR_PP3 = 0x2ee0;
  }

  docking_fixture = CreateID( "docking_fixture" );

  ntb_dist = *ADDR_NTBDIST;
  if (ntb_dist == 1)		// if "sight" has been modified directly
    ntb_dist = 1/2500.0f;	//  use original value

  // Prevent collision detection turning itself off.
  memcpy( ADDR_GHOSTJ, "\x09\xC1\x74\x21\xEB\x08", 6 );

  old_new_game = **ADDR_NEWGAME;
  *ADDR_NEWGAME = &new_new_game;

  CALL( ADDR_IME, Ime_Hook );
  ADDR_IME[5] = 0x90;
}


BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
  if (fdwReason == DLL_PROCESS_ATTACH)
  {
    g_hinst = hinstDLL;
    Patch();
  }

  return TRUE;
}
