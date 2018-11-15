/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  Einlesen des Plotfiles
**
**  29.04.1993  Beginn
**  23.08.1993  Größe der Graphen auf 4000 Werte erhöht
*/

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "tgp.h"
#include "tg.h"

/*
**  Handle des Plotfiles
*/
static FILE* plotfile = NULL;

/*
**  Name des Plotfiles
*/
static const char nofile [] = "No File";
static const char* filename = nofile;

/*
**  Buffer der Größe anlegen
*/
#define BUFFERSIZE 20 KB

/*
**  Im Buffer sollten wenn möglich LOWPEGEL Bytes sein
**  Es muß dafür gesorgt sein, das der aktuelle Datensatz komplett
**  im Buffer steht.
*/
#define LOWPEGEL 5 KB

/*
**   Eingabebuffer
*/
static char* buffer = NULL;       /* Zeiger auf Buffer          */
static int readindex;             /* Index, nächstes Byte lesen */
static int fuellindex;            /* Anzahl Bytes im Buffer     */

/*
**  Fehlermeldung weiterreichen bei Fehler mit der Datei
**    msg : String, bezeichnet die Art des Fehlers
*/
static void LOCAL serror ( const char* msg )
  {
  error( "in TG00 Modul : File '%s' %s\n"
	 "Libary meldet : %s\n"
	, filename, msg, strerror( errno ) );
  }

/*
**  Fehermeldung weiterreichen bei sonstigen Fehler
**    msg : die Fehlermeldung
*/
static void LOCAL merror ( const char* msg )
  {
  error( "in TG00 Modul : %s.\n", msg );
  }

/*
**  Buffer aus File auffuellen
*/
static void nachfuellen ( void )
  {
#ifdef DEBUG
  puts( "nachfuellen()" );
#endif
  memmove( buffer, buffer + readindex, fuellindex - readindex );
  fuellindex -= readindex;
  readindex = 0;
  fuellindex +=
    fread( buffer + fuellindex, 1, BUFFERSIZE - fuellindex, plotfile );
  }

/*
**  Plotfile zum Lesen öffnen
**    name : Name des Plotfiles, muß während des gesamten Programmlaufs
**           aktuell bleiben
*/
void plotfile_open ( const char* name )
  {
#ifdef DEBUG
  printf( "plotfile_open( %s )\n", name );
#endif
  if( plotfile != NULL ) plotfile_close();
  filename = name;
  plotfile = fopen( name, "rb" );
  if( plotfile == NULL ) serror( "kann nicht geöffnet werden." );
  buffer = malloc( BUFFERSIZE );
  if( buffer == NULL ) merror( "kann Buffer nicht anlegen, out of heap" );
  readindex = 0;
  fuellindex = 0;
  nachfuellen();
  }

/*
**  Plotfile schließen
*/
void plotfile_close ( void )
  {
#ifdef DEBUG
  puts( "plotfile_close()" );
#endif
  if( fclose( plotfile ) != 0 ) serror( "kann nicht schließen" );
  free( buffer );
  }

/*
**  Typ des nächsten Datensatz feststellen
**    return : der Typ, speziell tgp_eof wenn Dateiende erreicht
*/
enum tgp plotfile_next ( void )
  {
  enum tgp result;
#ifdef DEBUG
  puts( "plotfile_next()" );
#endif
  if( fuellindex - readindex < LOWPEGEL ) nachfuellen();
  if( fuellindex - readindex < 2 )
    result = _tgp_eof;
  else
    {
    result = * ( const enum tgp* ) ( buffer + readindex );
    if( result < 0 || result >= _tgp_eof ) merror( "ungültiges Typenfeld" );
    }
  return result;
  }

/*
**  Lesen einer Zeile aus dem Plotfile
**    return : Zeiger auf die Daten, die Daten müssen nach dem
**             nächsten plotfile_... Aufruf nicht mehr gültig
**             sein
*/
zeile* plotfile_getzeile ( void )
  {
  zeile* ptr;
  size_t need;
  int i;
#ifdef DEBUG
  fputs( "plotfile_getzeile()", stdout );
#endif
  if( fuellindex - readindex < LOWPEGEL ) nachfuellen();
  ptr = (zeile*) ( buffer + readindex );
  if( ptr->typ != _tgp_zeile ) merror( "getzeile bei falschem Typ" );
  for( i = 0; i < sizeof ptr->data && ptr->data[i] != '\0'; ++i ) {}
  need = sizeof (zeile) - sizeof ptr->data + i + 1;
  if( fuellindex - readindex < need ) merror( "getzeile über Fileende" );
  readindex += need;
#ifdef DEBUG
  printf( " = %s\n", ptr->data );
#endif
  return ptr;
  }

/*
**  Lesen eines Graphen aus dem Plotfile
**    return : (siehe plotfile_getzeile)
*/
graf* plotfile_getgraf ( void )
  {
  graf* ptr;
  size_t need;
#ifdef DEBUG
  puts( "plotfile_getgraf()" );
#endif
  if( fuellindex - readindex < LOWPEGEL ) nachfuellen();
  ptr = (graf*) ( buffer + readindex );
  if( ptr->typ != _tgp_graf ) merror( "getgraf bei falschem Typ" );
  if( ptr->count < 0 || ptr->count > TGPGRAFLEN )
    merror( "getgraf falscher 'count' Wert" );
  need = sizeof (graf) - sizeof ptr->data + ptr->count;
  if( fuellindex - readindex < need ) merror( "getgraf über Fileende" );
  readindex += need;
  return ptr;
  }

/*
**  Lesen eines FF aus dem Plotfile
**    return : (siehe plotfile_getzeile)
*/
ff* plotfile_getff ( void )
  {
  ff* ptr;
#ifdef DEBUG
  puts( "plotfile_getff()" );
#endif
  if( fuellindex - readindex < LOWPEGEL ) nachfuellen();
  ptr = (ff*) ( buffer + readindex );
  if( ptr->typ != _tgp_ff ) merror( "getff bei falschem Typ" );
  if( fuellindex - readindex < sizeof (ff) ) merror( "getff über Fileende" );
  readindex += sizeof (ff);
  return ptr;
  }
