/*
**  Zortech C 2.1 / Turbo C 2.0 / MS QuickC 1.01 / MS C 5.10
**
**  .04.1995  Ulrich Berntien
**            Insitut für Experimentalphysik, Fokusgruppe
**
**  11.04.95  Beginn
*/

#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __TURBOC__
#  include <sys\types.h>
#  include <sys\stat.h>
#  define EXIT_FAILURE 1
#  define EXIT_SUCCESS 0
#  define _open open
#  define _creat creat
#  define _write write
#  define _read read
#  define _close close
#  define CREAT_FLAG (S_IREAD|S_IWRITE)
#else
#  define CREAT_FLAG (0)
#endif

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "osz2col   -  .04.1995 U.Berntien\n"
  "\n"
  "Einlesen eines OSZ-Files, schreiben eines COL-Files.\n"
  "* Das OSZ-File enthält Daten vom DSO GOULD in der im\n"
  "  GOULD Handbuch beschriebenen Form.\n"
  "  Ausgewertet werden nur die Angaben TRC1A=, TRC2A=,\n"
  "  TRC3A= und TRC4A=.\n"
  "* In das COL-File werden die Daten der vier genannten\n"
  "  Kanäle geschrieben. Die Werte werden als Dezimalzahlen\n"
  "  in durch Leerzeichen getrennte Spalten geschrieben.\n"
  "\n"
  "Aufrufformat:    osz2col <Eingabefile> <Ausgabefile>"
  "";

/*
**  Datenstruktur zur Aufnahme eines Files
**  Zur einfachen Bearberitung wird hier das File komplett im Speicher
**  gehalten.
*/
typedef struct inBufferTAG
  {
  size_t size;         /* Bytes im 'buffer' */
  char buffer [1];     /* Dummy Länge       */
  }
  inBuffer;

/*
**  Handling eines Kanals
*/
typedef struct aChannelTAG
  {
  char* ptr;         /* dort den nächsten Wert lesen              */
		     /* oder NULL, falls der Kanal nicht gefunden */
  char* ende;        /* dort hört der Buffer auf                  */
  int base;          /* Zahlenbasis, (2=binär)                    */
  }
  aChannel;

/*
**  Anzahl dser Kanäle maximal
*/
enum enumTAG { maxChannel = 4 };

/*
**  Ausgabe einer Fehlermeldung und Programm beenden
**  form : Ausgabe über printf
*/
static void error ( const char* form, ... )
  {
  va_list args;
  fputs( "osz2col ERROR: ", stderr );
  va_start( args, form );
  vfprintf( stderr, form, args );
  va_end( args );
  exit( EXIT_FAILURE );
  }

/*
**  Ausgabe einer Warnmeldung
**  form : Ausgabe über printf
*/
static void warning ( const char* form, ... )
  {
  va_list args;
  fputs( "osz2col WARNING: ", stderr );
  va_start( args, form );
  vfprintf( stderr, form, args );
  va_end( args );
  }

/*
**  Löschen eines Input-Buffers
**  ib : diesen Löschen
*/
static void delInBuffer ( inBuffer* ib )
  {
  free( ib );
  }

/*
**  Einlesen eines Files komplett in den Speicher
**  fname  : Name des Files
**  return : Zeiger auf die Daten des Files
*/
static inBuffer* loadFile ( const char* fname )
  {
  inBuffer* result;
  long flength;
  size_t size;
  int handle = _open( fname, O_RDONLY + O_BINARY );
  if( handle < 0 ) error( "Can't open file \"%s\".\n", fname );
  flength = filelength( handle );
  if( flength < 0 ) error( "Can't read file length of \"%s\".\n", fname );
  /* Dateilänge ggf. abschneiden, damit in Datenstruktur paßt */
  size = (size_t) ( flength + (long) sizeof (inBuffer) );
  size -= sizeof (inBuffer);
  if( (long) size < flength ) warning( "File \"%s\" truncated %ld -> %ld.\n", fname, flength, (long) size );
  result = malloc( size + sizeof (inBuffer) );
  if( result == NULL ) error( "File \"%s\" too big to fit in memory.\n", fname );
  result->size = size;
  errno = 0;
  _read( handle, result->buffer, size );
  if( errno ) error( "Can't read file \"%s\" (System: %s)\n", fname, strerror(errno) );
  _close( handle );
  return result;
  }

/*
**  Benutzeranfrage: Ingonieren oder Quit
**  return : Soll ignoriert werden ?
*/
static int ignore ( void )
  {
  char answer;
  do{
    fputs( "(i)gnore or (q)uit>", stdout );
    answer = (char) getchar();
    answer = tolower( answer );
    }
    while( answer != 'i' && answer != 'q' );
  return answer == 'i';
  }

/*
**  neues File zum schreiben anlegen
**  fname  : Name des Files
**  return : Handle des geöffneten Files
*/
static int newFile ( const char* fname )
  {
  int handle;
  handle = _open( fname, O_RDONLY + O_BINARY );
  if( handle > 0 )
    {
    _close( handle );
    warning( "Output file \"%s\" exists.\n", fname );
    if( ignore() )
      handle = _open( fname, O_WRONLY + O_BINARY + O_TRUNC );
    else
      exit( EXIT_FAILURE );
    }
  else
    handle = _creat( fname, CREAT_FLAG );
  if( handle < 0 ) error( "Can't create file \"%s\".\n", fname );
  return handle;
  }

/*
**  Schreiben einer Integerzahl ins File, Zahl min. 3 Stellen, ein
**  Leezeichen folgt.
**  fh     : file handle, in das File schreiben
**  number : zu schreibenden Zahl
*/
static void writeInt ( int fh, int number )
  {
  char buffer [20];
  size_t len = sprintf( buffer, "%3d ", number );
  if( _write( fh, buffer, len ) != len )
    error( "Can't write in output file (system: %s).\n", strerror(errno) );
  }

/*
**  suchen nach einem String
**  str    : diesen String suchen
**  fb     : in diesem File buffer suchen
**  return : hinter den gesuchen String, oder NULL falls nicht gefunden
*/
static char* find ( const char* str, inBuffer* fb )
  {
  char* result;
  unsigned i;
  unsigned len;
  unsigned bis;
  len = strlen(str);
  bis = fb->size > len ? fb->size - len : 0;
  i = 0;
  while( i < bis && memcmp( str, fb->buffer + i, len ) != 0 ) ++i;
  if( i < bis )
    result = fb->buffer + i + len;
  else
    result = NULL;
  return result;
  }

/*
**  Initialisieren des Lesens eines Kanals
**  ch     : Datenstruktur wird initialisiert
**  nummer : Nummer des Kanals bei der Gould 1,2,..
**  fb     : in diesem Filebuffer stehen die Daten
*/
static void initChannel ( aChannel* ch, int nummer, inBuffer* fb )
  {
  char* ptr;
  char str [20];
  ch->ende = fb->buffer + fb->size;
  sprintf( str, "TRC%dA=", nummer );
  ptr = find( str, fb );
  if( ptr == NULL )
    {
    ch->ptr = NULL;
    warning( "No channel %d (\"TRC%dA\" not found).\n", nummer, nummer );
    }
  else
    {
    if( ptr[0] == '#' )
      {
      ch->ptr = ptr + 2;
      switch( ptr[1] )
	{
	case 'B' : ch->base = 2;
		   ch->ende = ch->ptr + 1008;
		   break;
	case 'O' : ch->base = 8;
		   break;
	case 'H' : ch->base = 16;
		   break;
	default  : warning( "Channel %d, unknown number base\n", nummer );
		   ch->ptr = NULL;
	}
      }
    else
      {
      ch->ptr = ptr;
      ch->base = 10;
      }
    }
  }

/*
**  Lesen der nächsten Zahl aus einem Kanal
**  ch     : Datenstruktur eines Kanals
**  return : nächster Wert 0..255, oder -1 falls kein weiterer Wert
*/
static int readChannel ( aChannel* ch )
  {
  int result;
  if( ch->ptr != NULL && ch->ptr < ch->ende )
    {
    if( ch->base == 2 )
      result = (int) ( (unsigned char) *ch->ptr++ );
    else
      {
      char* tmp = ch->ptr;
      result = (int) strtol( ch->ptr, &tmp, ch->base );
      if( ch->base == 10 ) result += 128;
      if( tmp == ch->ptr )
	{
	result = -1;
	ch->ptr = ch->ende;
	}
      else
	{
	if( tmp < ch->ende )
	  ch->ptr = *tmp == ',' ? tmp+1 : ch->ende;
	}
      }
    }
  else
    result = -1;
  return result;
  }

/*
**  Steuerung der Konvertierung
**  inFile  : Name des Eingabefiles
**  outFile : Name des ausgabefiles
*/
static void osz2col ( const char* inFile, const char* outFile )
  {
  int i, ok;
  inBuffer* input;
  int output;
  aChannel ch[maxChannel];
  input = loadFile( inFile );
  output = newFile( outFile );
  for( i = 0; i < maxChannel; ++i ) initChannel( ch+i, i+1, input );
  i = 0;
  while( i < maxChannel && ch->ptr == NULL ) ++i;
  if( i >= maxChannel ) error( "no channel found (no TRC*A).\n" );
  ok = 1;
  while( ok )
    {
    for( i = 0; i < maxChannel; ++i )
      if( ch[i].ptr )
	{
	int akku = readChannel( ch + i );
	if( akku < 0 )
	  {
	  ok = 0;
	  break;
	  }
	writeInt( output, akku );
	}
    _write( output, "\r\n", 2 );
    }
  _close( output );
  delInBuffer( input );
  }

/*
**  M A I N
*/
int main ( int argc, char** argv )
  {
  if( argc != 3 )
    {
    puts( manual );
    exit( EXIT_FAILURE );
    }
  else
    osz2col( argv[1], argv[2] );
  return EXIT_SUCCESS;
  }
