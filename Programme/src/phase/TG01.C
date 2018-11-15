/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  Ausgaben in File mit Fehlerkontrolle
**
**  29.04.1993  Beginn
*/

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tgp.h"
#include "tg.h"

/*
**  Die Ausgabedatei
*/
static FILE* outfile = NULL;

/*
**  Name der Ausgabedatei
*/
static const char* filename = "no file";

/*
**  Fehlerausgabe im Zusammenhang mit Ausgabefile
**    msg : Art des Fehlers
*/
static void merror ( const char* msg )
  {
  error( "in TG01 Fehler : %s Datei '%s'\n"
	 "Libary meldet : %s\n"
	 , msg, filename, strerror( errno ) );
  }

/*
**  Datei zur Ausgabe öffnen, write binary
**    name : der Name der Datei
*/
void out_open ( const char* name )
  {
  size_t size = 20 KB;
  if( outfile != NULL ) out_close();
  filename = strdup( name );
  outfile = fopen( name, "wb" );
  if( outfile == NULL ) merror( "kann nicht öffnen" );
  while( setvbuf( outfile, NULL, _IOFBF, 20 KB ) && size > 1 KB ) size /= 2;
  }

/*
**  Datei schließen
*/
void out_close ( void )
  {
  if( fclose( outfile ) ) merror( "kann nicht schließen" );
  free( filename );
  }

/*
**  Filehandle der Ausgabedeatei erfahren
**    return : Filehandle der Ausgabedatei
*/
FILE* out_getfile ( void )
  {
  return outfile;
  }

/*
**  Ausgabe eines Strings ins Ausgabefile mit Fehlerkontrolle
**    str : der String wird ausgegeben
*/
void out_puts ( const char* str )
  {
  if( fputs( str, outfile ) ) merror( "beim Schreiben (puts) in" );
  }

/*
**  Ausgabe über vfprintf in das Ausgabefile mit Fehlerkontrolle
**    Parameter für vfprintf
*/
void out_printf ( const char* form,... )
  {
  va_list argptr;
  va_start( argptr, form );
  if( vfprintf( outfile, form, argptr ) < 0 )
    merror( "beim Schreiben (vfprintf) in" );
  va_end( argptr );
  }

