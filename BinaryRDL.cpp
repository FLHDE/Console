/*
  BinaryRDL.cpp - Implementation of BinaryRDL.

  Jason Hood, 2 & 3 December, 2009.
*/

#include "BinaryRDL.h"
#include "internal.h"
#include <cstdlib>
#include <cstdarg>


void BinaryRDL::add_node( int node, int len )
{
  int new_size = size + 8 + len;
  if (new_size > capacity)
  {
    capacity = new_size;
    char* temp = new char[capacity];
    memcpy( temp, brdl, size );
    delete[] brdl;
    brdl = temp;
    data.c = brdl + size;
  }
  size = new_size;

  *data.i++ = node;
  *data.i++ = len;
}


void BinaryRDL::TRA( UINT tra, UINT mask )
{
  add_node( TRANode, 8 );
  *data.i++ = tra;		// TextRenderAttributes
  *data.i++ = mask;
}


void BinaryRDL::style( USHORT style )
{
  add_node( StyleNode, 2 );
  *data.s++ = style;
}


void BinaryRDL::para()
{
  add_node( ParagraphNode, 2 ); // ignores it if it's zero; use 2 to keep
  *data.s++ = 0;		//  word (wchar_t) alignment
}


void BinaryRDL::string( LPCWSTR str )
{
  int len = (wcslen( str ) + 1) * sizeof(WCHAR);
  add_node( TextNode, len );
  memcpy( data.w, str, len );
  data.c += len;
}


void BinaryRDL::string( LPCSTR str )
{
  mbstowcs( wstrbuf, str, *wstrlen );
  string( wstrbuf );
}


void BinaryRDL::strid( UINT id )
{
  GetString( RSRC, id, wstrbuf, *wstrlen );
  string( wstrbuf );
}


void BinaryRDL::printf( LPCWSTR fmt, ... )
{
  va_list args;
  va_start( args, fmt );
  _vsnwprintf( wstrbuf, *wstrlen, fmt, args );
  va_end( args );

  string( wstrbuf );
}


void BinaryRDL::printf( UINT ifmt, ... )
{
  WCHAR fmt[128];
  GetString( RSRC, ifmt, fmt, 128 );

  va_list args;
  va_start( args, ifmt );
  _vsnwprintf( wstrbuf, *wstrlen, fmt, args );
  va_end( args );

  string( wstrbuf );
}
