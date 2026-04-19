/*
  BinaryRDL.h - Definitions for the BinaryRDL format.

  Jason Hood, 2 & 3 December, 2009.
*/

#ifndef _BINARYRDL_H
#define _BINARYRDL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


enum { NoNode = 0,
       TRANode,
       TextNode,
       UnknownNode,	// causes failure if present
       PositionNode,
       ParagraphNode,
       StyleNode };

enum { TRA_Bold = 1,
       TRA_Italic = 2,
       TRA_Underline = 4,
       TRA_Color = 0xFFFFFF00 };

// The bits in brackets are the defaults, overridden by the definition in
// DATA\FONTS\rich_fonts.ini.

enum { STYLE_SENDER		 = 0xd5ec,	// white, font = 2
       STYLE_CONSOLE		 = 0x73f7,	// white (green, font = 2, iu)
       STYLE_UNIVERSE		 = 0xd681,	// white
       STYLE_SYSTEM		 = 0xd801,	// aqua
       STYLE_LOCAL		 = 0x1d9d,	// green
       STYLE_PRIVATE		 = 0x8283,	// green (i)
       STYLE_INFO		 = 0xe784,	// white, b
       STYLE_NOTICE		 = 0x3ea8,	// red, b
       STYLE_GROUP		 = 0xba28,	// #FF7EFD (magenta)
       StyleBoldYellow		 = 0x66b7,	// yellow, b (unused)
       STYLE_INVITE		 = 0x2b34,	// yellow, iu
       STYLE_PLAYER		 = 0x66ac,	// white
       STYLE_BUTTON		 = 0x828b,	// aqua, font = 6 (1)
       STYLE_TITLE		 = 0x1df5,	// aqua, font = 3, center
       STYLE_SUBTITLE		 = 0x43e0,	// aqua, font = 3
       STYLE_HEADER		 = 0xd824,	// aqua, font = 1
       STYLE_HEADER_ACTIVE	 = 0xd190,	// yellow, font = 1
       STYLE_TABLE		 = 0x69f5,	// aqua
       STYLE_TABLE_SELECT	 = 0x3343,	// yellow
       STYLE_TABLE_INACTIVE	 = 0xbb80,	// #B6B6B6 (light grey)
       STYLE_LOCTABLE		 = 0x5203,	// aqua (font = 1)
       STYLE_LOCTABLE_SELECT	 = 0x3dbc,	// yellow (font = 1)
       STYLE_LOCTABLE_INACTIVE	 = 0x867f,	// #B6B6B6 (font = 1)
       STYLE_NN 		 = 0x9ca4,	// aqua
       STYLE_DATA		 = 0x76ec,	// aqua
       StyleSmallAqua		 = 0x686c,	// aqua, font = 5 (unused)
       SYTLE_NN_BOLD		 = 0x0f64,	// aqua, b
       STYLE_NN_SELECTED	 = 0x3d0a,	// yellow
       STYLE_DIALOG		 = 0x5c12,	// aqua, font = 1
       STYLE_DIALOG_SMALL	 = 0xc9b7,	// aqua, font = 5
       STYLE_SMALL_HEADER	 = 0x3df7,	// aqua, font = 5
       STYLE_SMALL_HEADER_ACTIVE = 0xda80,	// yellow, font = 5
       STYLE_SUBSUBTITLE	 = 0xfaa9,	// aqua, font = 1
       STYLE_ERROR		 = 0x3363,	// red
       STYLE_NN_AGENCY		 = 0x5724,	// aqua, font = 5
       STYLE_CONSOLE_SENDER	 = 0x6c0e,	// #00FF00, font = 2
       STYLE_LABEL		 = 0x4433	// aqua, font = 1
     };


union VPtr
{
  PCHAR   c;
  PUSHORT s;
  PINT	  i;
  PWCHAR  w;
};


struct BinaryRDL
{
  char*  brdl;
  size_t size;
  size_t capacity;

  VPtr	 data;

  void	 add_node( int, int );

  void	 clear() { size = 0; data.c = brdl; }

  void	 TRA( UINT, UINT );
  void	 style( USHORT );
  void	 string( LPCWSTR );
  void	 string( LPCSTR );
  void	 strid( UINT );
  void	 printf( LPCWSTR, ... );
  void	 printf( UINT, ... );
  void	 para();
};

#endif
