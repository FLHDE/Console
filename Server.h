/*
  Server.h - Declarations for functions imported from Server.dll.

  Jason Hood, 20 October, 2009.
*/

#ifndef _SERVER_H
#define _SERVER_H

#pragma comment( lib, "Server.lib" )

#include "Common.h"


enum DestroyType {  SilentDestroy = 0,
                    FuseDestroy = 1 };


namespace pub
{
  EXTERN int Save( UINT, UINT );

  namespace Player
  {
    EXTERN int AdjustCash( const UINT&, int );
    EXTERN int ForceLand( UINT, UINT );
    EXTERN int GetAssetValue( const UINT&, float& );
    EXTERN int GetBase( const UINT&, UINT& );
    EXTERN int GetRep( const UINT&, int& );
    EXTERN int GetShip( const UINT&, UINT& );
    EXTERN int GetShipID( const UINT&, UINT& );
    EXTERN int GetSystem( const UINT&, UINT& );
    EXTERN int InspectCash( const UINT&, int& );
    EXTERN int PopUpDialog( UINT, const FmtStr&, const FmtStr&, UINT );
    EXTERN int RevertCamera( UINT );
    EXTERN int SetMonkey( UINT );
    EXTERN int SetRobot( UINT );
    EXTERN int SetShipAndLoadout( const UINT&, UINT, const EquipDescVector& );
    EXTERN int SetTrent( UINT );
  }

  namespace Reputation
  {
    struct Enumerator
    {
      virtual bool operator()( UINT ) = 0;
    };

    EXTERN int EnumerateGroups( Enumerator& );
    EXTERN int GetReputation( const int&, const UINT&, float& );
    EXTERN int GetReputation( const UINT&, const UINT&, float& );
    EXTERN int GetReputationGroup( UINT&, LPCSTR );
    EXTERN int GetShortGroupName( const UINT&, UINT& );
    EXTERN int SetReputation( const int&, const UINT&, float );
  }

  namespace SpaceObj
  {
    EXTERN int Destroy( UINT, DestroyType );
    EXTERN int GetBurnRadius( const UINT&, float&, Vector& );
    EXTERN int GetLocation( UINT, Vector&, Matrix& );
    EXTERN int GetRadius( const UINT&, float&, Vector& );
    EXTERN int GetSolarArchetypeID( UINT, UINT& );
    EXTERN int GetSystem( UINT, UINT& );
    EXTERN int GetTarget( const UINT&, UINT& );
    EXTERN int GetType( UINT, UINT& );
    EXTERN int IsPosEmpty( const UINT&, const Vector&, const float&, bool& );
    EXTERN int Relocate( const UINT&, const UINT&, const Vector&, const Matrix& );
  }

  namespace System
  {
    struct SysObj
    {
      char   nick[32];		// NOT NUL-terminated if longer
      Vector pos;
      UINT   archid;
      UINT   ids_name;
      UINT   ids_info;
      char   reputation[32];
      UINT   dock_with;
      UINT   goto_system;
      UINT   system;
      // -------------------
      // Some nicknames are longer than 32 characters, so take advantage of the
      // fact that where it gets the nickname from immediately follows.
      size_t len;		// TString<64>
      char   nickname[64];
    };

    struct SysObjEnumerator
    {
      virtual bool operator()( const SysObj& ) = 0;
    };

    EXTERN int EnumerateObjects( const UINT&, SysObjEnumerator& );
  }
}


typedef std::list<CollisionGroupDesc> CollisionGroupDescList;
typedef CollisionGroupDescList::iterator CollisionGroupDescListIter;

struct PlayerData
{
  /* 0x000 */ BYTE		     dontcare1[0x268];
  /* 0x268 */ float		     hull_status;
  /* 0x26C */ CollisionGroupDescList collisiongroupdesc;
  /* 0x278 */ EquipDescList	     equipdesclist;
  /* 0x284 */ BYTE		     dontcare2[0x60];
  /* 0x2E4 */ Costume		     costume;
  /* 0x318 */ BYTE		     dontcare3[0x28];
  /* 0x340 */ bool		     skip_autosave;
  /* 0x341 */ BYTE		     dontcare4[0x3f];
  /* 0x380 */ string		     wg;
  /* 0x390 */ BYTE		     dontcare5[0x50];
  /* 0x3E0 */ UINT		     system;
  /* 0x3E4 */ BYTE		     dontcare6[0x34];
  /* 0x418 */

  bool save( const CHARACTER_ID& filename, LPCWSTR desc, UINT desc_id );
};

class PlayerDB
{
public:
  PlayerData* playerdata;
};


EXTERN PlayerDB Players;


#endif
