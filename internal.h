/*
  internal.h - Internal functions of Freelancer.

  Jason Hood, 3 December, 2009.
*/

#ifndef _INTERNAL_H
#define _INTERNAL_H

#include "Common.h"

typedef LPVOID (*TCreateSound)( const UINT& ); // sound nickname id (ID_String)
extern TCreateSound CreateSound;

typedef bool (*TPlaySound)( LPVOID,	// the above
			    int, int ); // dunno
extern TPlaySound PlaySound;

typedef UINT (*TGetString)( LPVOID,	// resource table?
			    UINT,	// resource number
			    LPWSTR,	// buffer for string
			    UINT	// length of buffer (chars, not bytes)
			  );
extern TGetString GetString;
#define RSRC *(LPVOID*)0x67eca8 	// the resource table

typedef void (*TUpdateNavBar)();
extern TUpdateNavBar UpdateNavBar;

typedef void (*TCloseDialog)();
extern TCloseDialog CloseDialog;

typedef PUINT (*TGetRepCol)( PUINT,	// actually an RGBA structure
			     float	// the reputation
			   );
extern TGetRepCol GetRepCol;

typedef void (*TDisplayBase)( float );	// delay for script completion
extern TDisplayBase DisplayBase;

typedef void (*TDisplaySystem)();
extern TDisplaySystem DisplaySystem;

typedef void (*TFmtCredits)( LPWSTR,	// buffer for formatted string
			     int,	// number to format
			     int	// resource number or -1 for default
			   );
extern TFmtCredits FmtCredits;

enum { COMMODITY_DEALER = 1, SHIP_DEALER, EQUIPMENT_DEALER };
typedef void (*TDealerDialog)( int,	// type of dialog
			       PROC,	// called on close
			       LPVOID,	// parameter for above
			       UINT	// ship archetype (0 for current)
			     );
extern TDealerDialog DealerDialog;

typedef void (*TLoadAutosave)();
extern TLoadAutosave LoadAutosave;

typedef void (*TEnterBase)( const UINT& ); // base id
extern TEnterBase EnterBase;

/*
typedef int (*TShipValue)( UINT,	// player's ship
			   bool 	// true to include damage and resale
			 );
extern TShipValue ShipValue;
*/

typedef void (__stdcall *TSetWeaponGroup)( UINT,	// player? (ignored)
					   LPCSTR,	// all wg lines
					   int		// length of string
					 );
extern TSetWeaponGroup SetWeaponGroup;

typedef void (*TLoadGame)( const CHARACTER_ID&, 	// file name
			   bool 			// skip autosave
			 );
extern TLoadGame LoadGame;

typedef int (*Tstartup)( LPCSTR,			// file name
			 Archetype::ICliObjFactory*,
			 /*DataConfig::ProgressCB**/LPVOID );

extern PUINT const pbats_id;
extern PUINT const pbots_id;

// Make use of Freelancer's string buffer.
extern LPWSTR const wstrbuf;
extern PUINT  const wstrlen;

#endif
