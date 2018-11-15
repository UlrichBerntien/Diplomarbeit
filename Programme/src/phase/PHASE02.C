/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .03.1993  Ulrich Berntien
**
**  Ausgabe in Plotfile
**  Das Plotfile kann mit TGPLOT.C auf einen HP-LaserJet ausgegeben
**  werden oder mit TGTEX.C in TeX-Format umgewandelt werden.
**
**  20.03.1993  Beginn
**  30.03.1993  Modul phase02.c von phase1.c abgetrennt
**  18.08.1993  Längen der Daten variabel
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "phase.h"
#include "tgp.h"

#ifdef __ZTC__
#  include <stddef.h>
#else
#  define offsetof(t,i) ( (size_t) ( (char*) &( (t*)0)->i - (char*)0 ) )
#endif

/*
**  Name und Zugriff auf das Plotfile
*/
static const char* filename;
static FILE* file;

/*
**  Plotfile schließen
*/
static void termit ( void )
  {
  static int error = 0;
  if( fclose( file ) && ! error )
    {
    error = 1;
    verror( "%sbeim Plotfile '%s' Schließen aufgetreten.\n", txterr, filename );
    }
  }

/*
**  Plotfile öffnen, nur einmal in Programmlauf aufrufen
**    fname : Names des Plotfiles
*/
void _cdecl plotfile_init ( const char* fname )
  {
  file = fopen( fname, "ab" );
  if( file == NULL )
    verror( "%sbeim Anhängen an Plotfile '%s'\n", txterr, fname );
  filename = fname;
  atexit( termit );
  }

/*
**  Daten ins Plotfile schreiben
**    size : Größe des Datenblocks in Bytes
**    data : die Daten
*/
static void LOCAL writef ( size_t size, const void* data )
  {
  if( fwrite( data, size, 1, file ) != 1 )
    verror( "%sbeim Schreiben in Plotfile '%s' aufgetreten.\n", txterr, filename );
  }

/*
**  Eine Graphen ins Plotfile schreiben
**    len  : Anzahl der Werte in data
**    data : der Graph
*/
void _cdecl plotfile_graf ( int len, const uchar* data )
  {
  struct tgp_graf graf;
  graf.typ = _tgp_graf;
  graf.count = len;
  writef( offsetof( struct tgp_graf, data[0] ), &graf );
  writef( len, data );
  }

/*
**  einen long-Graph ins Plotfileschreiben
**    x      : der Graph
**    return : Punkte (0..255) von x->min bis x->max
*/
int _cdecl plotfile_grafl ( const grafl* x )
  {
  int i;
  const int len = x->len;
  const long min = x->min;
  uchar* graf = malloc( len * sizeof (uchar) );
  long teiler = x->max - min;
  if( graf == NULL ) error( noheap );
  teiler = teiler == 0 ? 1l : ( teiler + 254 )/ 255;
  for( i = 0; i < len; ++i )
    graf[i] = (uchar) ( ( x->data[i] - min ) / teiler );
  plotfile_graf( len, graf );
  free( graf );
  return (int) ( ( x->max - min ) / teiler );
  }

/*
**  Kommentar des Benutzers wird ins Plotfile geschrieben
**    frage : String wird als Einleitung auf Console ausgegeben
*/
void _cdecl plotfile_komment ( const char* frage )
  {
  int len;
  struct tgp_zeile zeile;
  puts( frage );
  zeile.typ = _tgp_zeile;
  do{
    zeile.data[0] = '\0';
    len = readstring( sizeof zeile.data, zeile.data );
    if( len > 0 )
       writef( len + offsetof( struct tgp_zeile, data[0] ), &zeile );
    }
    while( len > 0 );
  }

/*
**  Ausgabe über vsprintf ins Plotfile schreiben
*/
void _cdecl plotfile_printf ( const char* form, ... )
  {
  struct tgp_zeile zeile;
  int len;
  va_list arg_ptr;
  zeile.typ = _tgp_zeile;
  va_start( arg_ptr, form );
  len = vsprintf( zeile.data, form, arg_ptr ) + 1;
  writef( len + offsetof( struct tgp_zeile, data[0] ), &zeile );
  va_end( arg_ptr );
  }

/*
**  Formfeed ins Plotfile schreiben
*/
void _cdecl plotfile_ff ( void )
  {
  struct tgp_ff ff;
  ff.typ = _tgp_ff;
  writef( sizeof ff, &ff );
  }

