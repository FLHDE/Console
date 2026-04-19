/*
  Common.h - Declarations for functions imported from Common.dll.

  Jason Hood, 20 October, 2009.
*/

#ifndef _COMMON_H
#define _COMMON_H

#pragma comment( lib, "Common.lib" )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cmath>
#include <list>
#include <vector>
#include <string>

using std::string;

#define EXTERN __declspec(dllimport)

// Remove the warning caused by the vector inside the struct.
#pragma warning( disable : 4251 )


struct EXTERN CacheString
{
  void clear();

  LPSTR str;
};


struct CHARACTER_ID
{
  char file[512];
};


struct EXTERN EquipDesc
{
  EquipDesc();
  EquipDesc( const EquipDesc& );
  EquipDesc& operator=( const EquipDesc& );
  bool operator==( const EquipDesc& ) const;
  bool operator!=( const EquipDesc& ) const;
  bool operator<( const EquipDesc& ) const;
  bool operator>( const EquipDesc& ) const;

  UINT	 get_arch_id() const;
  float  get_cargo_space_occupied() const;
  int	 get_count() const;
  const CacheString& get_hardpoint() const;
  USHORT get_id() const;
  int	 get_owner() const;
  float  get_status() const;
  bool	 get_temporary() const;
  bool	 is_equipped() const;
  bool	 is_internal() const;
  void	 make_internal();
  void	 set_arch_id( UINT );
  void	 set_count( int );
  void	 set_equipped( bool );
  void	 set_hardpoint( const CacheString& );
  void	 set_id( USHORT );
  void	 set_owner( int );
  void	 set_status( float );
  void	 set_temporary( bool );

  BYTE	      b1;		// ?
  USHORT      sID;
  UINT	      iArchID;
  CacheString szHardPoint;
  bool	      bMounted;
  float       fHealth;
  UINT	      iCount;
  bool	      bMission; 	// temporary
  int	      iOwner;

  static const CacheString CARGO_BAY_HP_NAME;
};


class EquipDescList
{
public:
  std::list<EquipDesc> equip;
};


struct EXTERN EquipDescVector
{
  EquipDescVector();
  EquipDescVector( const EquipDescVector& );
  EquipDescVector( const EquipDescList& );
  ~EquipDescVector();
  EquipDescVector& operator=( const EquipDescVector& );

  std::vector<EquipDesc> equip;
};


class Matrix
{
public:
  float m[3][3];

  // Perform a 180-degree turn.
  void turn_around()
  {
    m[0][0] *= -1;
    m[0][1] *= -1;
    m[0][2] *= -1;
    m[2][0] *= -1;
    m[2][1] *= -1;
    m[2][2] *= -1;
  }
};


class Vector
{
public:
  float x, y, z;

  void zero()
  {
    x = y = z = 0;
  }

  // Right/left.
  void add_x( float dist, const Matrix& ornt )
  {
    x += dist * ornt.m[0][0];
    y += dist * ornt.m[1][0];
    z += dist * ornt.m[2][0];
  }
  // Up/down.
  void add_y( float dist, const Matrix& ornt )
  {
    x += dist * ornt.m[0][1];
    y += dist * ornt.m[1][1];
    z += dist * ornt.m[2][1];
  }
  // Forwards/backwards.
  void add_z( float dist, const Matrix& ornt )
  {
    x -= dist * ornt.m[0][2];
    y -= dist * ornt.m[1][2];
    z -= dist * ornt.m[2][2];
  }

  // Determine the square of the distance to VEC.
  float distsqr( const Vector& vec )
  {
    return ((x - vec.x) * (x - vec.x) +
	    (y - vec.y) * (y - vec.y) +
	    (z - vec.z) * (z - vec.z));
  }

  // Determine the distance to VEC.
  float dist( const Vector& vec)
  {
    return sqrtf( distsqr( vec ) );
  }
};


EXTERN UINT   CreateID( LPCSTR );
EXTERN bool   GetUserDataPath( LPSTR const );
EXTERN Matrix EulerMatrix( const Vector& );
EXTERN Matrix LookMatrixYup( const Vector& );
EXTERN bool   SinglePlayer();
EXTERN bool   arch_is_combinable( UINT );


struct CollisionGroupDesc
{
  USHORT id;
  float  health;
};


struct Costume
{
  UINT head;
  UINT body;
  UINT lefthand;
  UINT righthand;
  UINT accessory[8];
  int  accessories;
};

struct costume;

class CostumeDescriptions
{
public:
  const costume* find_costume( ULONG ) const;
  void get_costume( int, Costume& ) const;
};

const CostumeDescriptions* GetCostumeDescriptions();


struct EXTERN FmtStr
{
  UINT something;
  UINT strid;		// resource containing text
  BYTE tnav_marker;	// counters for each type
  BYTE tsystem;
  BYTE tbase;
  BYTE tstring;
  BYTE tgood;
  BYTE tunused;
  BYTE tint;
  BYTE trep_instance;
  BYTE trep_group;
  BYTE tzone_id;
  BYTE tspace_obj_id;
  BYTE tfmt_str;
  BYTE tinstallation;
  BYTE tloot;

  FmtStr() : something( 0 ),
	     strid( 0 ),
	     tnav_marker( 0 ),
	     tsystem( 0 ),
	     tbase( 0 ),
	     tstring( 0 ),
	     tgood( 0 ),
	     tunused( 0 ),
	     tint( 0 ),
	     trep_instance( 0 ),
	     trep_group( 0 ),
	     tzone_id( 0 ),
	     tspace_obj_id( 0 ),
	     tfmt_str( 0 ),
	     tinstallation( 0 ),
	     tloot( 0 )
	     { }
  ~FmtStr();
  void append_string( UINT );
  void begin_mad_lib( UINT );
  void end_mad_lib();
};


class EXTERN INI_Reader
{
public:
  INI_Reader();
  ~INI_Reader();
  void	 close();
  LPCSTR get_line_ptr();
  UINT	 get_num_parameters() const;
  bool	 get_value_bool( UINT );
  float  get_value_float( UINT );
  int	 get_value_int( UINT );
  LPCSTR get_value_string( UINT );
  LPCSTR get_value_string();
  bool	 is_header( LPCSTR );
  bool	 is_value( LPCSTR );
  bool	 is_value_empty( UINT );
  bool	 open( LPCSTR, bool = false );
  bool	 read_header();
  bool	 read_value();

private:
  BYTE	 data[0x1568];
};


struct ISpatialPartition;


namespace AnimDB
{
  EXTERN void Add( int );
  EXTERN void Rem( int );
}


namespace Archetype
{
  struct ICliObjFactory;

  struct Equipment
  {
    BYTE  dontcare[0x5c];
    float volume;
  };

  struct CollisionGroup
  {
    CollisionGroup* next;
    USHORT	id;
    CacheString name;
  };

  struct EqObj
  {
    const CollisionGroup* get_group_by_id( USHORT ) const;
    const CollisionGroup* get_group_by_name( const CacheString& ) const;
  };

  struct Ship
  {
    /* 0x000 */ BYTE   dontcare1[0x14];
    /* 0x014 */ UINT   strid;	// actually part of the root EqObj, I think
    /* 0x018 */ BYTE   dontcare2[0x2c];
    /* 0x044 */ LPVOID animation_list;
    /* 0x048 */ BYTE   dontcare3[0xb8];
    /* 0x100 */ float  hold_size;
    /* 0x104 */ BYTE   dontcare4[0x3c];
    /* 0x140 */ int    nanobots;
    /* 0x144 */ int    batteries;
  };

  struct Solar
  {
    BYTE dontcare[0x08];
    UINT archid;
  };

  Equipment* GetEquipment( UINT );
  Ship*      GetShip( UINT );
  Solar*     GetSolar( UINT );
  int	     LoadShips( LPCSTR, bool, ICliObjFactory* );
}


namespace EngineEquipConsts
{
  EXTERN float CRUISE_ACCEL_TIME;
  EXTERN float CRUISING_SPEED;
}


namespace Loadout
{
  EXTERN bool ReadCargoLine( INI_Reader&, EquipDesc& );
  EXTERN bool ReadEquipLine( INI_Reader&, EquipDesc& );
}


namespace PhySys
{
  EXTERN float ANOM_LIMITS_MAX_VELOCITY;
}


namespace Reputation
{
  EXTERN UINT get_info_card( UINT );
}


namespace Universe
{
  struct IBase
  {
    UINT   id;
    LPCSTR nickname;
    UINT   strid_name;
    LPCSTR file;
    UINT   system;
    string run_by;
    UINT   object;
    BYTE   visit;
    UINT   ship_sml_01;
    UINT   ship_sml_02;
    UINT   ship_sml_03;
    UINT   ship_mdm_01;
    UINT   ship_mdm_02;
    UINT   ship_mdm_03;
    UINT   ship_lrg_01;
    UINT   ship_lrg_02;
    UINT   ship_lrg_03;
    UINT   terrain_tiny;
    UINT   terrain_sml;
    UINT   terrain_mdm;
    UINT   terrain_lrg;
    UINT   terrain_dyna_01;
    UINT   terrain_dyna_02;
    bool   autosave_forbidden;

    virtual UINT   get_id() const;
    virtual LPCSTR get_nickname() const;
    virtual UINT   get_strid_name() const;
    virtual LPCSTR get_file() const;
    virtual UINT   get_system() const;
    virtual LPCSTR get_run_by() const;
    virtual UINT   get_ship_sml_01() const;
    virtual UINT   get_ship_sml_02() const;
    virtual UINT   get_ship_sml_03() const;
    virtual UINT   get_ship_mdm_01() const;
    virtual UINT   get_ship_mdm_02() const;
    virtual UINT   get_ship_mdm_03() const;
    virtual UINT   get_ship_lrg_01() const;
    virtual UINT   get_ship_lrg_02() const;
    virtual UINT   get_ship_lrg_03() const;
    virtual UINT   get_terrain_tiny() const;
    virtual UINT   get_terrain_sml() const;
    virtual UINT   get_terrain_mdm() const;
    virtual UINT   get_terrain_lrg() const;
    virtual UINT   get_terrain_dyna_01() const;
    virtual UINT   get_terrain_dyna_02() const;
    virtual bool   get_autosave_forbidden() const;
    virtual UINT   set_system( UINT );
    virtual UINT   get_object() const;
    virtual BYTE   get_visit() const;
    virtual UINT   set_object( UINT );
    virtual BYTE   set_visit( BYTE );
  };

  struct ISystem
  {
    LPVOID pvtable;

    LPVOID pvftable;		// CommReferrable
    size_t msgidprefix_len;	// TString<64>
    char   msgidprefix_str[64];

    UINT   id;			// ID_String
    LPCSTR nickname;		// CacheString
    LPVOID connections[4];	// std::vector
    BYTE   visit;
    UINT   strid_name;
    UINT   ids_info;
    LPCSTR file;		// CacheString
    Vector NavMapPos;
    LPVOID zones[3];		// std::list
    ISpatialPartition* spatial;
    float  NavMapScale;
    UINT   spacemusic;
  };

  EXTERN       IBase*	GetFirstBase();
  EXTERN       ISystem* GetFirstSystem();
  EXTERN       IBase*	GetNextBase();
  EXTERN       ISystem* GetNextSystem();
  EXTERN       IBase*	get_base( UINT );
  EXTERN const ISystem* get_system( UINT );
}


struct EXTERN IObjRW // : public IObjInspectImpl
{
  virtual const Vector& get_position() const;
};


struct EXTERN CShip
{
  IObjRW* get_target() const;
  float   open_bay();
  float   close_bay();

  /* 0x000 */ LPVOID vftable;
  /* 0x004 */ long   engine_instance;
  /* 0x008 */ BYTE   dontcare2[0x80];
  /* 0x088 */ Archetype::Ship* aship;
  /* 0x08c */ BYTE   dontcare3[0x24];
  /* 0x0b0 */ UINT   id;
  /* 0x0b4 */ BYTE   dontcare4[0x178];
  /* 0x22c */ int    bay_anim;
  /* 0x230 */ BYTE   dontcare5[0x7c];
  /* 0x2ac */ int    bay_state; 	// enum BayState
};

#endif
