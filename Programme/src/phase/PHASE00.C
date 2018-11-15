/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .03.1993  Ulrich Berntien
**
**  Schnittstelle zur Konsole
**
**  20.03.1993  Beginn
**  30.03.1993  Modul phase00.c von phase1.c abgespalten
**  18.08.1993  Modul stredit.c eingebunden
*/

#include <conio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "phase.h"

/*
**  Leert Tastaturbuffer
*/
static void LOCAL leeretasten ( void )
  {
  fflush( stdin );
  while( kbhit() ) getch();
  }

/*
**  Wartet, bis eine Taste gedrückt wird
**    return : die gedrückte Taste
*/
int _cdecl waitkey ( void )
  {
  leeretasten();
  return getch();
  }

/*
**  Abfrage ob Taste 's' gedrückt wurde
**    wait   : falls != 0, nach Drücken von 'w' auf nächste Taste warten
**    return : != 0, wenn Taste 's' gedrückt
*/
int _cdecl stoptaste ( int wait )
  {
  int key = kbhit() ? getch() : 0;
  if( wait && ( key == 'w' || key == 'W' ) ) key = getch();
  return key == 's' || key == 'S';
  }

/*
**  Antwort auf Ja/Nein Frage holen
**    return : != 0, wenn Ja
*/
int _cdecl janein ( void )
  {
  int ja;
  char buffer [10];
  int fehler;
  do{
    fputs( " (J/N) ? ", stdout );
    leeretasten();
    fehler = fgets( buffer, sizeof buffer, stdin ) == NULL || buffer[1] != '\n';
    if( fehler )
      fputs( "Antwort nicht auswertbar :", stdout );
    else
      ja = buffer[0] == 'j' || buffer[0] == 'J';
    }
    while( fehler );
  return ja;
  }

/*
**  String vom Benutzer einlesen
**    len    : Länge des Buffers = maximale Stringlänge + 1
**    buffer : Buffer für den String
**    return : Länge des gelesen Strings mit dem '\0' am Ende
*/
int _cdecl readstring ( const int len, char* buffer )
  {
  putchar( '>' );
  leeretasten();
  stredit( buffer, len );
  return strlen( buffer );
  }

/*
**  Fehlermeldung ausgegeben und Programm beenden
**  benutzt globale Varibale txterr
**    msg : die Meldung
*/
void _cdecl error ( const char* msg )
  {
  fprintf( stderr, "%s%s\n", txterr, msg );
  exit( 1 );
  }

/*
**  Ausgabe einer Meldung über vfprintf und Programm beenden
**    form : Form-String für vfprintf
*/
void _cdecl verror ( const char* form, ... )
  {
  va_list argptr;
  va_start( argptr, form );
  vfprintf( stderr, form, argptr );
  va_end( argptr );
  exit( 1 );
  }

