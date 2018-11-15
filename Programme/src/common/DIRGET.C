/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .08.1993
**
**  Funktionen zum Durchsuchen eines Directories
**
**  18.08.1993  Beginn
*/

#include <ctype.h>
#include <dos.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"  /* nur für Prototyp */

#ifndef __ZTC__
#  include <fcntl.h>
#endif

#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#define LOCAL _pascal _near

/*
**  Buffer um die Pfadangabe zwischenzuspeichern
*/
static char buffer [ 200 ];

/*
**  dort beginnt der Filename im 'buffer'
*/
static char* namestart;

/*
**  Trennt Pfad von Filenamen
**  name   : Pfad mit Filenamen
**  all    : dort wird Zeiger auf Buffer mit Pfad+Filename zurückgeben
**  return : Zeiger auf Beginn des Filenames im Buffer
*/
static char* LOCAL loesepfad ( const char* name, char* buffer )
  {
  int filename = 0;
  int i;
  for( i = 0; name[i] != '\0'; ++i )
    {
    const char akku = buffer[i] = toupper( name[i] );
    if( akku == ':' || akku == '/' || akku == '\\' ) filename = i+1;
    }
  return buffer + filename;
  }

#if defined __ZTC__

  #define FINDFIRST findfirst
  #define FINDNEXT  findnext

#elif defined __TURBOC__

  #include <dir.h>
  #define FIND ffblk
  #define name ff_name

  static struct FIND find_buffer;

  static struct FIND* LOCAL FINDFIRST ( const char* name, unsigned attr )
    {
    int result = findfirst( name, &find_buffer, attr );
    return result ? NULL : &find_buffer;
    }

  static struct FIND* LOCAL FINDNEXT ( void )
    {
    int result = findnext( &find_buffer );
    return result ? NULL : &find_buffer;
    }

#else

  #define FIND     find_t

  static struct FIND find_buffer;

  static struct FIND* LOCAL FINDFIRST ( const char* name, unsigned attr )
    {
    int result = _dos_findfirst( (char*) name, attr, &find_buffer );
    return result ? NULL : &find_buffer;
    }

  static struct FIND* LOCAL FINDNEXT ( void )
    {
    int result = _dos_findnext( &find_buffer );
    return result ? NULL : &find_buffer;
    }

#endif

/*
**  name   : Filename (mit Pfad), darf Joker *,? enthalten
**  return : Filename (mit Pfad) von erster gefunden Datei oder
**           NULL wenn keine Datei gefunden
*/
const char* dirget_first ( const char* name )
  {
  struct FIND* find;
  int ok;
  namestart = loesepfad( name, buffer );
  find = FINDFIRST( name, 0xff );
  ok = find != NULL;
  if( ok ) strcpy( namestart, find->name );
  return ok ? buffer : NULL;
  }

/*
**  Stellt die durch 'dirget_first' begonne Suche fort.
**  return : Filename (mit Pfad) von nächster gefundenr Datei oder
**           NULL wenn keine Datei gefunden
*/
const char* dirget_next ( void )
  {
  const struct FIND* find = FINDNEXT();
  const int ok = find != NULL;
  if( ok ) strcpy( namestart, find->name );
  return ok ? buffer : NULL;
  }

/*
**  Name des gerade gefundenen Files ohne Pfad erfragen
**  return : der Name
*/
const char* dirget_name ( void )
  {
  return namestart;
  }
