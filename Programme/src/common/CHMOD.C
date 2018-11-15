/*
**  C  ( Zortech V2.1 )
**
**  .03.1993 Ulrich Berntien
**
**  Anzeigen und Ändern von Fileattributen
**
**  09.03.1993  Beginn
**  22.05.1993  Joker in Filenamen und Schalter s,r ergänzt
**  21.09.1993  chmod arbeitet weiter, auch wenn ein Lesen/Schreiben der
**              Fileattribute nicht gelungen ist.
**  06.10.1993  immer nach Files suchen, wenn Schalter /s gesetzt
**  09.10.1993  Schalter q ergänzt
*/

#include <ctype.h>
#include <dos.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
**  Programm - Kurzanleitung
*/
static const char manual [] =
  "CHMOD   .03.1993   Ulrich Berntien\n"
  "Anzeigen und ändern von Fileattributen.\n\n"
  "Aufrufformat:\n"
  "     chmod [(+|-)flag] [switch] file [ ... ]\n\n"
  "   +     setzt das Fileattribut\n"
  "   -     löscht das Fileattribut\n"
  "flag R   Read-only         H  Hidden\n"
  "     S   System            V  Volume label\n"
  "     D   Sub-Directory     A  Archive\n"
  " switch  /s rekursiv Unterverzeichnisse durchsuchen\n"
  "         /r nicht mehr durchsuchen\n"
  "         /q Fileattribute nicht anzeigen\n"
  "         /p wieder anzeigen\n"
  " file    Dateiname, darf Joker *,? enthalten\n";

/*
**  Schalterstellungen
*/
static int subdirs = 0;
static int quit = 0;

/*
**  Bit - Buchstabezuordnung der Fileattribute
*/
static const char attribute [] = "RHSVDA";

/*
**  Struktur zur Aufnahme von Pfadangabe und Filename
*/
typedef struct filenameTAG
  {
  char pfad [200];
  char name [50];
  char* pfadende;
  }
  filename;

/*
**  Struktur zur Aufname der zu setzenden/löschenden Bits
*/
typedef struct taskTAG
  {
  unsigned plus;         /* setzende  Bits */
  unsigned minus;        /* löschende Bits */
  }
  task;

/*
**  Fehlermeldung ausgeben und Programm beenden
**  form : das Format für die Fehlermeldung für printf
*/
static void error ( const char* form, ... )
  {
  va_list arg_ptr;
  va_start( arg_ptr, form );
  fputs( "chmod error : ", stderr );
  vfprintf( stderr, form, arg_ptr );
  va_end( arg_ptr );
  exit( EXIT_FAILURE );
  }

/*
**  Identifiziert das Flag
**  flag   : Buchstabe, der das Fileattribut identifiziert
**  return : zugehöriges Bit
*/
static unsigned bitflag ( char flag )
  {
  const char* ptr = attribute;
  unsigned i = 1;
  flag = toupper( flag );
  while( *ptr != flag && *ptr != '\0' )
    {
    ++ptr;
    i <<= 1;
    }
  if( *ptr == '\0' )
    error( "unknown flag '%c'\n", flag );
  return i;
  }

/*
**  Auswerten eines String aus Flags
**  str    : String mit den Fileattributen
**  pflags : hier wird das Bit gesetzt, wenn es in nflags nicht gelöscht wurde
**  nflags : hier wird das Bit gelöscht, wenn es gesetzt war
*/
static void addflags ( const char* str, unsigned* pflags, unsigned* nflags )
  {
  while( *str != '\0' )
    {
    const unsigned akku = bitflag( *str++ );
    if( akku & *nflags )
      *nflags &= ~akku;
    else
      *pflags |= akku;
    }
  }

/*
**  Ausgabe Filename und File-Attribute
**  fname : Filename
**  attr  : Fileattribute
*/
static void print ( const char* fname, unsigned attr )
  {
  unsigned i = 1;
  char buffer[9] = "________";
  char* dest = buffer + 7;
  const char* ptr = attribute;
  while( *ptr != 0 )
    {
    if( attr & i ) *dest = *ptr;
    i <<= 1;
    ++ptr;
    --dest;
    }
  printf( "%s   %s\n", buffer, fname );
  }

/*
**  Ändert Fileattribute gemäß plus, minus
**  zeigt die geänderten Attribute an
**  fname : Filename
**  flags : zu machende Änderungen
*/
static void set ( const char* fname, task flags )
  {
  unsigned alt;
  if( dos_getfileattr( fname, &alt ) != 0 )
    printf( "can't get file attributes of '%s'\n", fname );
  else
    {
    const unsigned neu = ( alt | flags.plus ) & ~flags.minus;
    if( neu != alt && dos_setfileattr( fname, neu ) != 0 )
      printf( "can't set file attibutes to '%s'\n", fname );
    else
      if( ! quit ) print( fname, neu );
    }
  }

/*
**  Trennt Name vom Pfad und untersucht auf Joker
**  name   : zusammengesetzter Filename
**  fname  : die Teile getrennt
**  return : != 0, wenn Joker im Filenamen
*/
static int separate ( const char* name, filename* fname )
  {
  int joker = 0;
  char akku;
  char* pfad = fname->pfad;
  char* namestart = fname->pfad;
  while( ( akku = *name++ ) != '\0' )
    {
    switch( akku )
      {
      case '/'  : akku = '\\';
      case '\\' :
      case ':'  : namestart = pfad + 1;
                  break;
      case '*'  :
      case '?'  : joker = 1;
                  break;
      }
    *pfad++ = akku;
    }
  *pfad = akku;
  strcpy( fname->name, namestart );
  fname->pfadende = namestart;
  return joker;
  }

/*
**  Durchsucht Directory und leitet die gefunden Files an set weiter
**  fname   : zerlegter Filename
**  flags   : zu machende Änderungen
*/
static void setdir ( filename* fname, task flags )
  {
  char* last = fname->pfadende;
  struct FIND find;
  int nofile = _dos_findfirst( fname->pfad, ~FA_DIREC, &find );
  while( ! nofile )
    {
    strcpy( last, find.name );
    set( fname->pfad, flags );
    nofile = _dos_findnext( &find );
    }
  if( subdirs )
    {
    strcpy( last, "*.*" );
    nofile = _dos_findfirst( fname->pfad, FA_DIREC, &find );
    while( ! nofile )
      {
      if( *find.name != '.' && ( find.attribute & ( FA_DIREC | FA_LABEL ) ) != 0 )
        {
        char* ptr = stpcpy( last, find.name );
        *ptr++ = '\\';
        fname->pfadende = ptr;
        ptr = stpcpy( ptr, fname->name );
        setdir( fname, flags );
        }
      nofile = _dos_findnext( &find );
      }
    fname->pfadende = last;
    }
  }

/*
**  Ändert Fileattribute gemäß plus, minus
**  zeigt die geänderten Attribute an
**  name    : Filename, kann Joker *,? enthalten
**  flags   : zu machende Änderungen
*/
static void setall ( const char* name, task flags )
  {
  filename fname;
  const int joker = separate( name, &fname );
  if( joker || subdirs )
    setdir( &fname, flags );
  else
    set( name, flags );
  }

/*
**  Schalter 's' oder 'r' auswerten
**  str    : der Schalter aus der Kommandozeile
**  subdir : dort speichern ob Subdirs durchsucht werden sollen
*/
static void schalter ( const char* str )
  {
  static const char err [] = "unknow switch '%s'\n";
  if( str[1] != '\0' )
    error( err, str );
  else
    switch( str[0] )
      {
      case 's' : subdirs = 1;
                 break;
      case 'r' : subdirs = 0;
                 break;
      case 'q' : quit = 1;
                 break;
      case 'p' : quit = 0;
                 break;
      default  : error( err, str );
      };
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  if( argc < 2 )
    {
    fputs( manual, stderr );
    exit( EXIT_FAILURE );
    }
  else
    {
    task flags;
    flags.plus = 0;
    flags.minus = 0;
    while( argv++, --argc > 0 )
      if( **argv == '+' )
        addflags( *argv + 1, &flags.plus, &flags.minus );
      else if( **argv == '-' )
        addflags( *argv + 1, &flags.minus, &flags.plus );
      else if( **argv == '/' )
        schalter( *argv + 1 );
      else
        setall( *argv, flags );
    }
  return EXIT_SUCCESS;
  }
