/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / TURBO C V 2.0 )
**
**  keine Optimierungen durch den Compiler erlauben,
**  sonst funktioniert der Datenempfang nicht
**
**  .03.1993  Ulrich Berntien
**
**  Datenübertragung Gould <-> PC über RS 232 Schnittstelle
**
**  20.03.1993  Beginn
**  30.03.1993  Modul phase01.c von phase1.c abgespalten
**  18.08.1993  Länge der Daten variabel
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rs232.h"
#include "phase.h"

/*
**  Anzahl der Werte, die Gould pro Kanal sendet
*/
#define LENGOULD 1008

/*
**  Fehlercode
*/
#define NAK 0x15

/*
**  Gould-Befehle
*/
static char ask_timebase [] = "TRHS?A\n";
static char ask_kanal    [] = "TRC?A\n";

static char set_lockon  [] = "LOCK=ON\n";
static char set_lockoff [] = "LOCK=OFF\n";
static char set_hex     [] = "NB=HEX\n";

/*
**  Initialisierung der Schnittstelle
**    return : != 0, wenn Fehler auftrat
*/
static int LOCAL init ( void )
  {
  static notinit = 1;
  if( notinit )
    {
    rs232parameter parameter;
    parameter.com = 1;
    parameter.baud = 9600;
    parameter.databits = 7;
    parameter.stopbits = 1;
    parameter.parity = _rs232even;
    notinit = rs232setparameter( &parameter );
    if( notinit )
      error( "kann serielle-Schnittstelle nicht einstellen" );
    else
      puts( "Gould an COM1 9600,7,1,even CTRL-S/Q" );
    }
  return notinit;
  }

/*
**  String zur Gould senden, Rückkehr nach erfolgter Übertragung
**    str    : der String
**    return : != 0, wenn ein Fehler auftrat
*/
static int LOCAL stringout ( const char* str )
  {
  int error = 0;
  const char *volatile *ptr = rs232send( str );
  const char* control = str;
  const char* cmp;
  time_t last;
  while( ( cmp = *ptr ) != NULL && ! error )
    {
    if( stoptaste(0) )
      {
      puts( "Datensendung abgebrochen" );
      error = 1;
      }
    if( cmp > control )
      {
      time( &last );
      control = cmp;
      }
    else if( time(NULL) - last > 2 )
      {
      puts( "Zeitüberschreitung bei Datensendung" );
      error = 1;
      }
    }
  return error;
  }

/*
**  Lesezeiger aus dem Buffer
*/
static const volatile char* readptr;

/*
**  Zeiger für den Zugriff auf den Buffer
*/
static volatile rs232buffer* gouldptr;

/*
**  Buffer für Datenempfang anlegen
**  setzt globale Varibalen 'gouldptr' und 'readptr'
**    return : Zeiger auf den angelegten Buffer
*/
static char* LOCAL input ( void )
  {
  char* buffer = malloc( ( LENGOULD * 3 + 100 ) * sizeof (char) );
  if( buffer == NULL ) error( noheap );
  gouldptr = rs232setbuffer( buffer );
  readptr = buffer;
  return buffer;
  }

/*
**  Nächsten Zeichen aus dem Eingabekanal holen
**  return : das gelesene Zeichen
**           oder NAK wenn Fehler aufgetreten
*/
static char LOCAL charin ( void )
  {
  char error = 0;
  if( gouldptr->now <= readptr )
    {
    const time_t start = time(NULL);
    while( gouldptr->now <= readptr && ! error )
      {
      if( stoptaste(0) )
        {
        puts( "Datenempfang abgebrochen" );
        error = 1;
        }
      if( time(NULL) - start > 2 )
        {
        puts( "Zeitüberschreitung beim Datenempfang" );
        error = 1;
        }
      }
    }
  if( ! error )
    {
    if( gouldptr->parityerrors )
      {
      puts( "Parity-Error beim Datenempfang" );
      error = 1;
      }
    if( gouldptr->overflowerrors )
      {
      puts( "Overflow-Error beim Datenempfang" );
      error = 1;
      }
    }
  return ! error ? *readptr++ : NAK;
  }

/*
**  empfangene Zeichen von der Gould müssen den Zeichen im String entsprechen
**    str       : die vorgegebenen Zeichen
**    strtermit : dieses Zeichen beendet den String i.a. \0 aber auch \n
**    return    : != 0, wenn Zeichen nicht passen
*/
static int LOCAL match ( const char* str, char strtermit )
  {
  int error = 0;
  while( *str != strtermit && ! error )
    {
    const char akku = charin();
    error = akku == NAK || akku != *str++;
    }
  return error;
  }

/*
**  Daten aus einem Kanal von der Gould entgegennehmen
**    x      : dort die Daten speichern, das Datenfeld wird angelegt !
**    return : != 0, wenn ein Fehler auftrat
*/
static int LOCAL datain ( grafuc* x )
  {
  int errorflag;
  int i = 0;
  int min = 255;
  int max = 0;
  x->len = LENGOULD;
  x->data = malloc( LENGOULD * sizeof (uchar) );
  if( x->data == NULL ) error( noheap );
  errorflag = match( ask_kanal, '\n' ) || match( "=#H", '\0' );
  while( i < LENGOULD && ! errorflag )
    {
    int akku = 0;
    int ziffern = 2;
    while( --ziffern >= 0 && ! errorflag )
      {
      int ziffer = (int) charin();
      ziffer -= ziffer > '9' ? 'A' - 10 : '0';
      akku = akku * 16 + ziffer;
      errorflag = ziffer > 15 || ziffer < 0;
      }
    x->data[i++] = (uchar) akku;
    if( akku < min ) min = akku;
    if( akku > max ) max = akku;
    errorflag = errorflag || match( i == LENGOULD ? "\r\n" : ",", '\0' );
    }
  x->min = (uchar) min;
  x->max = (uchar) max;
  if( errorflag ) puts( "Empfangene Daten ungültig" );
  return errorflag;
  }

/*
**  Daten aus einem Kanal der Gould lesen
**    kanal  : Nummer des Kanal 1 .. 4
**    x      : dort die Daten speichern, data-Feld wird angelegt
**    return : != 0, wenn ein Fehler bei der Übertragung auftrat
*/
int _cdecl gould_read ( int kanal, grafuc* x )
  {
  int error = init();
  ask_kanal[3] = (char) kanal + '0';
  if( ! error )
    {
    char* buffer = input();
    error = stringout( set_hex ) || stringout( ask_kanal ) || datain( x );
    rs232clear();
    free(buffer);
    }
  return error;
  }

/*
**  Zeitbasis von Gould entgegennehmen
**    timebase : dort die Zeitbasis speichern
**    return   : != 0, wenn Fehler auftrat
*/
static int LOCAL timein ( float* timebase )
  {
  char akku;
  char buffer [20];
  int i = 0;
  int error = match( ask_timebase, '\n' ) || match( "=", '\0' );
  if( ! error )
    {
    while( ( akku = charin() ) > 0x20 && i < 20 ) buffer[i++] = akku;
    buffer[i] = '\0';
    error = akku != '\r' || match( "\n", '\0' );
    }
  if( ! error )
    error = sscanf( buffer, "%e", timebase ) != 1;
  return error;
  }

/*
**  Zeitbasis eines Kanals von der Gould lesen
**    kanal  : Nummer des Kanal 1 ..4
**    return : sec/Div. oder 0, wenn Fehler auftrat
*/
float _cdecl gould_timebase ( int kanal )
  {
  float result;
  int error = init();
  ask_timebase[4] = (char) kanal + '0';
  if( ! error )
    {
    char* buffer = input();
    error = stringout( ask_timebase ) || timein( &result );
    rs232clear();
    free( buffer );
    }
  return error ? (float) 0 : result;
  }

/*
**  "LOCK=ON" zur Gould senden
**    return : != 0, wenn ein Fehler bei der Übertragung auftrat
*/
int _cdecl gould_lock ( void )
  {
  return init() || stringout( set_lockon );
  }

/*
**  "LOCK=OFF" zur Gould senden
**    return : != 0, wenn ein Fehler auftrat
*/
int _cdecl gould_unlock ( void )
  {
  return init() || stringout( set_lockoff );
  }

