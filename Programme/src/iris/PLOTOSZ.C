/*
**  C   ( Zortech V2.1 / Microsoft V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .02.1993
**
**  Plotten der *.OSZ Dateien mit den Oszilloskop-Signalen
**
**  10.03.1993  Beginn, entstanden aus TRAFOOSZ.C Version vom 28.02.1993
**  15.05.1993  Joker in Eingabefilenamen erlaubt
**  16.07.1993  Schalter im Kommandozeile
**  18.08.1993  Modul DIRGET.C eingebunden
**  27.08.1993  Ausgabeform der Zahlen verändert
**  20.09.1993  Ausgabe von einzelnen Kanälen sperren
**  18.12.1994  Integration von Signalen
**  03.02.1998  Unterstützung verschiedener Drucker über das printer Modul
*/

#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "printer.h"
#include "plot.h"

#ifndef __ZTC__
#  include <fcntl.h>
#endif

#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

/********************************************************************/

#define LOCAL _pascal _near

static const char* manual [] =
  {
  "PLOTOSZ   .02.1992  Ulrich Berntien",
  "Aufrufformat:",
  "  PLOTOSZ [-<Schalter>] <files>",
  "<files>    : Namen der zu plottenden OSZ Files (*? erlaubt)",
  "<Schalter> :",
  "\t  o<Name>    Ausgabefile (Default: PRN)",
  "\t  p<Typ>     Druckertyp: HP oder NEC (Default: HP)",
  "\t  d+         OSZ-Files nach Ausgabe löschen",
  "\t  d-         OSZ-Files nach Ausgabe nicht löschen",
  "\t  k<Zahl>    Kanal 1..4 für Ausgabe sperren",
  "\t  k          alle Kanäle wieder freigeben",
  "\t  i<Zahl>    Kanal 1..4 wird auch zeitintegriert ausgegeben",
  "\t             als Nullpunkt wird die VP?-Angabe der Gould benutzt",
  "\t  I<Zahl>    wie Kanal 1..4, aber als Nullpunkt wird der Mittel-",
  "\t             der ersten 20 Sampels genommen.",
  "\t  e<Zahl>    Horizontale Expansion um Faktor 1..20",
  "\t  e          Horizontale Expansion aus dem OSZ-File",
  "\t  s<Zahl>    Horizontaler Shift -500..+500",
  "\t             -500(+500) Ende (Anfang) des Trace in derm Bildmitte",
  "\t  s          Horizontaler Shift aus dem OSZ-File",
  "Default:",
  "\t  -oPRN -d- -e -s",
  NULL
  };

/*
**  Daten erzeugt mit den Programmen SELNAMES.C und BUILDB.C
**  und per Hand optyp eingesetzt
\********************************************************************/

typedef enum TAGgruppen
  {
  _steuerpc,     /* Das Datum wurde vom Steuer-PC eingeführt        */
  _gould,        /* Wert von der Gould, siehe Gould-Handbuch        */
  _kanal         /* Werte von der Gould, einmal pro Kanal vorhanden */
  }
  gruppen;

typedef enum TAGoptyp
  {
  _none,         /* Das Feld wird nicht ausgewertet               */
  _hscale,       /* Skalierung horizontal in Sekunden/Einheit     */
  _vscale,       /* Skalierung vertikal in Volt/Einheit           */
  _signal,       /* Name des Signals (bzw. des Kanals)            */
  _trace,        /* Daten des Signals (bzw. aus dem Kanal)        */
  _date,         /* Datum und                                     */
  _time,         /* Uhrzeit der Aufnahme/Filerstellung            */
  _name,         /* Name des Experiments                          */
  _nummer,       /* Nummer des Schusses / des Experiments         */
  _shift,        /* horizontale Verschiebung auf dem Scope-Schirm */
  _expansion,    /* Dehnung der Signale auf dem Scope-Schirm      */
  _position      /* vertikale Position eines Kanals               */
  }
  optyp;

typedef struct TAGnamen
  {
  const char* name;
  gruppen gruppe;
  int lt,gt;
  optyp op;
  }
  namen;

/*
**  Wurzel des Baums mit den Namen im Feld 'listnamen'
*/
static const int rootnamen = 18;

/*
**  Array mit den möglichen Namen, die einzelnen Felder sind
**  über listnamen[?].lt und listnamen[?].gt zu einem binären
**  Baum verknüpft.
**  Ein '?' im Namen steht für die Nummer des Kanals '1' bis '4'.
*/
static const namen listnamen [29] =
  {
  { "DATE",     _steuerpc, 21, -1, _date },
  { "TIME",     _steuerpc, -1, -1, _time },
  { "EXPNAME",  _steuerpc, 22,  3, _name },
  { "SCHUSSNR", _steuerpc, 10, 12, _nummer },
  { "ADD12",    _gould, -1, -1, _none },
  { "ADD34",    _gould,  4,  6, _none },
  { "AUTCAL",   _gould, -1, -1, _none },
  { "AVRG",     _gould,  5,  8, _none },
  { "BWLIM",    _gould, -1, -1, _none },
  { "HE",       _gould, -1, -1, _expansion },
  { "HMOD",     _gould,  9, 11, _none },
  { "HSA",      _gould, 23, -1, _none },
  { "STAT",     _gould, 13, 14, _none },
  { "SHFT",     _gould, -1, -1, _shift },
  { "TDELA",    _gould, -1, -1, _none },
  { "TGAAUT",   _gould,  2, 16, _none },
  { "TLA",      _gould,  1, 17, _none },
  { "TRGCA",    _gould, 28, -1, _none },
  { "TRGMDA",   _gould, 15, 19, _none },
  { "TSA",      _gould, 24, 26, _none },
  { "TSLA",     _gould, -1, -1, _none },
  { "CH?SIGNAL",_kanal, -1, -1, _signal },
  { "CH?",      _kanal,  7,  0, _none },
  { "HOLD?",    _kanal, -1, -1, _none },
  { "TRHS?A",   _kanal, -1, 25, _hscale },
  { "TRVS?A",   _kanal, -1, -1, _vscale },
  { "VC?",      _kanal, 20, 27, _none },
  { "VP?",      _kanal, -1, -1, _position },
  { "TRC?A",    _kanal, -1, -1, _trace }
  };

/********************************************************************/

/*
**  Anzahl der Samples aus der Gould von einem Kanal
*/
#define TRACELEN 1008

/*
**  Maximale Anzahl der Kanäle
*/
#define MAXCHANNELS 4

/*
**  Da der #Operator für Macros nicht immer zur Verfügung steht
*/
#define MAXCHANNELS_STR "4"

/*
**  Anzahl der Samples, die maximal ausgeben werden
**  (z.B. nach Expansion)
*/
#define PLOTLEN 1150

/*
**  SHFT Variable der Gould (horizontal shift) im Bereich
**  +MAXSHIFT .. -MAXSHIFT möglich
*/
#define MAXSHIFT 500

/*
**  Daten betreffend den Versuch
*/
typedef struct TAGversuch
  {
  const char* name;       /* Name des Versuchs als Text    */
  const char* date;       /* Aufnahmedatum als Text        */
  const char* time;       /* Aufnahmezeit als Text         */
  const char* nummer;     /* Schussnummer als Text         */
  int shift;              /* Horizontal shift              */
  int expansion;          /* Horizontal expansion 1..20    */
  }
  versuch;

/*
**  Daten betreffend einen Kanal
*/
typedef struct TAGkanal
  {
  const char* hscale;     /* Angabe TRHS?A der Gould         */
  const char* vscale;     /* Angabe TRVS?A der Gould         */
  const char* position;   /* Angabe VP? der Gould            */
  const char* signal;     /* Name des Signals                */
  const char* trace;      /* Massendaten CH?SIGNAL der Gould */
  }
  kanal;

/*
**  Massendaten aus einem Trace in dieser Form speichern
**  (vorher ggf. expandiert, daher ist die Länge variabel)
*/
typedef struct TAGtracedata
  {
  int len;
  unsigned char data [PLOTLEN];
  }
  tracedata;

/*
**  Horizontal shift (SHFT) Angabe der Gould kann überschrieben werden
**  INT_MAX -> die Angabe der Gould verwenden, oder 0 wenn die Daten der
**             Gould die Angabe nicht enthält
*/
static int cmdline_shift = INT_MAX;

/*
**  Horizontal expansion (HE) Angabe der Gould kann überschrieben werden
**  INT_MAX -> die Angabe der Gould verwenden, oder 1 wenn die Daten der
**             Gould die Angabe nicht enthält
*/
static int cmdline_expansion = INT_MAX;

/*
**  Ausgabefile
**  falls == NULL, zur Ausgabe 'PRN' öffnen
*/
static FILE* ofile = NULL;

/*
**  OSZ-Files nach Ausgabe löschen ?
*/
static int deleteosz = 0;

/*
**  Gesperrte Kanäle
**  Default: mit 0 initialisiert
*/
static int gesperrt[MAXCHANNELS];

/*
**  zu integrierende Kanäle
**  0 -> nicht integrieren
**  1 -> integrieren, als Nullpunkt den Wert der Gould
**  2 -> integrieren, als Nullpunkt den Mittelwert der ersten 20 Sample
**  Default: mit 0 initialisiert
*/
static int integrieren[MAXCHANNELS];

/********************************************************************/

/*
**  Fehlermeldung ausgeben
**  msg : Meldungstext
*/
static void _near error ( const char* format, ... )
  {
  va_list arg_ptr;
  fputs( "\nPLOT-OSZ Fehler : ", stderr );
  va_start( arg_ptr, format );
  vfprintf( stderr, format, arg_ptr );
  va_end( arg_ptr );
  fputc( '\n', stderr );
  exit( 1 );
  }

/*
**  malloc mit Fehlerkontrolle
**  size   : benötigte Anzahl von Bytes
**  use    : Verwendungszweck
**  return : Zeiger analog malloc, immer erfolgreich
*/
static void* LOCAL Malloc ( size_t size, const char* use )
  {
  void* result = malloc( size );
  if( result == NULL ) error( "Out of Heap, %s.", use );
  return result;
  }

/*
**  analog strcmp, aber '?' steht für beliebiges Zeichen
**  a : String
**  b : String kann den Joker '?' enthalten
**  return analog strcmp
*/
static int LOCAL strfcmp ( const char* a, const char* b )
  {
  int i = 0;
  while( ( ( a[i] == b[i] ) || ( b[i] == '?' ) ) && b[i] != '\0' ) ++i;
  return a[i] - b[i];
  }

/*
**  Suchen in dem Baum nach dem Index
**  name   : nach diesem Namen suchen
**  return : der Index
*/
static int LOCAL search ( const char* name )
  {
  int index = rootnamen;
  const namen* ptr = listnamen + rootnamen;
  int cmp;
  while( ( cmp = strfcmp( name, ptr->name ) ) != 0 )
    {
    index = cmp < 0 ? ptr->lt : ptr->gt;
    if( index < 0 ) error( "Teil-Name '%s' nicht bekannt", name );
    ptr = listnamen + index;
    }
  return index;
  }

/*
**  Öffne Datei zum Lesen, beendet bei Fehler das Programm
**  fname  : Dateiname
**  return : DOS-Handle des Files
*/
static int LOCAL openfile ( const char* fname )
  {
  const int handle =
  #ifdef __ZTC__
    dos_open( fname, O_RDONLY );
  #else
    open((char*)  fname, O_RDONLY | O_BINARY );
  #endif
  if( handle < 0 )
    error( "File '%s' kann nicht zum Lesen geöffnet werden.\n", fname );
  return handle;
  }

/*
**  Ermittelt die Größe des Files, beendet Programm falls Datei zu groß
**  handle : DOS-Handle des Files
**  return : Bytes im File < 32 KByte
*/
static size_t LOCAL getsize ( int handle )
  {
  const long size = filelength( handle );
  if( size >= 32 * 1024l )
    error( "Dateilänge muß unter 32 KByte sein !" );
  return (size_t) size;
  }

/*
**  Das .OSZ File in den Speicher laden
**  fname  : Dateiname
**  return : Zeiger auf die Daten im Speicher, mit '\0' abgeschlossen
*/
static char* LOCAL ladeosz ( const char* fname )
  {
  int handle = openfile( fname );
  const size_t size = getsize( handle );
  char* daten = Malloc( size  + 1, "Buffer für OSZ_File" );
  if( read( handle, daten, size ) != size )
    error( "Fehler beim Einlesen der Datei." );
  daten[size] = '\0';
  if( close( handle ) < 0 )
    error( "Fehler beim Schließen der Datei." );
  return daten;
  }

/*
**  Trennt nächsten Namen oder Datenteil aus den Daten im .OSZ Format
**  pdaten  : Zeiger auf den Zeiger mit dem Rest der Daten
**            wird von der Funktion aktualisiert
**  stopper : Das Zeichen beendet den Teil ( '=' oder ';' )
**  return  : Zeiger auf den gelösten String
**            oder NULL, wenn Daten zu Ende
*/
static const char* LOCAL getpart ( char** pdaten, char stopper )
  {
  const char* result = *pdaten;
  if( *result == '\0' )
    result = NULL;
  else
    {
    char* ptr = *pdaten;
    char akku;
    while( ( akku = *ptr ) != stopper && akku != '\0' ) ++ptr;
    *ptr = '\0';
    *pdaten = ptr + ( akku != 0 );
    }
  return result;
  }

/*
**  Durchsucht einen String nach einer Zifffer
**  str    : der zu durchsuchende String
**  return : Wert der zuerst gefundenen Ziffer, oder -1 falls keine gefunden
*/
static int LOCAL getziffer ( const char* str )
  {
  while( *str != '\0' && ! isdigit( *str ) ) ++str;
  return isdigit( *str ) ? *str - '0' : -1;
  }

/*
**  Information zu einem Kanal speichern
**  op     : Op-Code zu den Daten
**  kanal  : dort die Daten zu diesem Kanal speichern
**  data   : die Daten von der Gould
**  return : ist der Op-code ungültig ?
*/
static int LOCAL lesekanal ( optyp op, kanal* kanal, const char* data )
  {
  int result = 0;
  switch( op )
    {
    case _hscale   : kanal->hscale = data;
                     break;
    case _vscale   : kanal->vscale = data;
                     break;
    case _signal   : kanal->signal = data;
                     break;
    case _trace    : kanal->trace = data;
                     break;
    case _position : kanal->position = data;
                     break;
    case _none     : break;
    default        : result = 1;
    }
  return result;
  }

/*
**  Informationen vom Steuer-PC speichern
**  op     : Op-Code zu den Daten
**  exp    : dort die Informationen über den Versuch speichern
**  data   : die Daten von der Gould
**  return : ist der OP-Code ungültig ?
*/
static int LOCAL lesesteuerpc ( optyp op, versuch* exp, const char* data )
  {
  int result = 0;
  switch( op )
    {
    case _date   : exp->date = data;
                   break;
    case _time   : exp->time = data;
                   break;
    case _name   : exp->name = data;
                   break;
    case _nummer : exp->nummer = data;
                   break;
    case _none   : break;
    default      : result = 1;
    }
  return result;
  }

/*
**  allg. Informationen von Gould auswerten
**  op     : der Op-Code zu den Daten
**  exp    : dort die Informationen über den Versuch speichern
**  data   : die Daten von der Gould
**  return : ist der Op-Code ungültig ?
*/
static int LOCAL lesegould ( optyp op, versuch* exp, const char* data )
  {
  int result = 0;
  switch( op )
    {
    case _shift     : {
                      float shift;
                      sscanf( data, "%f", &shift );
                      exp->shift = (int) ( 100 * shift );
                      break;
                      }
    case _expansion : sscanf( data, "%i", &exp->expansion );
                      break;
    case 0          : break;
    default         : result = 1;
    }
  return result;
  }

/*
**  Sucht die benötigten Daten aus OSZ File heraus
**  daten   : das eingelesen OSZ-File
**  kanaele : dort die Infomationen über die Kanäle
**  exp     : dort die Informationen über das Experiment
*/
static void LOCAL lese ( char* daten, kanal kanaele[MAXCHANNELS], versuch* exp )
  {
  static const char operror [] = "Ungültiger Op-Code bei Name = '%s'";
  const char* name;
  while( ( name = getpart( &daten, '=' ) ) != NULL )
    {
    const namen* was = listnamen + search( name );
    const char* data = getpart( &daten, ';' );
    if( data == NULL ) error( "Datenteil zu '%s' fehlt.", name );
    switch( was->gruppe )
      {
      case _kanal :    {
                       const int welcher = getziffer( name ) - 1;
                       if( welcher < 0 || welcher > 3 )
                         error( "ungültiger Kanal %d in '%s'", welcher+1, name );
                       else
                         if( lesekanal( was->op, kanaele + welcher, data ) )
                           error( operror, name );
                       break;
                       }
      case _steuerpc : if( lesesteuerpc( was->op, exp, data ) ) error( operror, name );
                       break;
      case _gould    : if( lesegould( was->op, exp, data ) ) error( operror, name );
                       break;
      default        : error( "Unbekannte Gruppe, Name = '%s'", name );
      }
    }
  }

/*
**  Öffnen der Ausgabe
**  fname  : Filename
**  return : das File, write binary
*/
static FILE* LOCAL openout ( const char* fname )
  {
  FILE* file = fopen( fname, "ab" );
  if( file == NULL )
    error( "Kann File '%s' nicht zum Schreiben öffnen", fname );
  return file;
  }

/*
**  Ausgaben einer Zahl mit Buchstabe postfix
**  x     : diese Zahl ausgeben
**  ofile : in dieses Files ausgeben
*/
static void LOCAL ingprint ( double x, FILE* ofile )
  {
  static const unsigned char letter [] =
     "\aTGMk mµnpfa";
  static const double power [] =
     { 1e15,1e12,1e9,1e6,1e3,1e0,1e-3,1e-6,1e-9,1e-12,1e-15,1e-18 };
  if( x != 0.0 )
    {
    unsigned char buchstabe;
    double abs = fabs( x );
    int index = 0;
    while( letter[index] != '\0' && abs < power[index] ) ++index;
    buchstabe = letter[index];
    if( buchstabe > '\a' )
      {
      const char* form = "%2.0lf ";
      abs = fabs( x /= power[index] );
      if( abs > 99.99 )
        form = "%4.0lf ";
      else if( abs > 9.999 )
        form = "%3.0lf ";
      fprintf( ofile, form, x );
      if( buchstabe != ' ' ) fputc( buchstabe, ofile );
      }
    else
      fprintf( ofile, "%9.3le ", x );
    }
  else
    fputs( "0 ", ofile );
  }

/*
**  Ausgeben der Horizontalen Skalierungsangabe
**  str  : der String mit HS Angabe von der Gould
**  ex   : Horizontale Expansion
**  file : in das File ausgeben, write binary
*/
static void LOCAL aushscale ( const char* str, int ex, FILE* file )
  {
  float akku;
  if( str != NULL && sscanf( str, "%G", &akku ) == 1 )
    {
    fputs( "HOR ", file );
    ingprint( akku/ex, file );
    fputs( "sec/div, ", file );
    }
  else
    fputs( "HOR nicht angegeben, ", file );
  if( ex > 1 )
    fprintf( file, "(mit Expansion %d) ", ex );
  }

/*
**  Ausgeben der Vertikalen Skalierungsangabe
**  str  : der String mit VS Angabe von der Gould
**  file : in das File ausgeben, write binary
*/
static void LOCAL ausvscale ( const char* str, FILE* file )
  {
  if( str != NULL )
    {
    double x;
    int uncal;
    int minus = *str == '-';
    if( minus ) ++str;
    uncal = *str == '<' || *str == '>';
    if( uncal ) ++str;
    fputs( minus ? "VER -" : "VER +", file );
    if( sscanf( str, "%lf", &x ) != 1 )
      fputs( str, file );
    else
      ingprint( x, file );
    fputs( "V/div", file );
    if( uncal ) fputs( " uncal", file );
    }
  else
    fputs( "VER nicht angegeben", file );
  }

/*
**  Auslesen der nächsten Zahl aus dem String der Massendaten der Gould
**  das ',' hinter der Zahl wird mitgelesen
**  die Dezimalzahlen -128..+127 werden nach 0..255 umgesetzt
**  pstr   : Zeiger auf Position im String, wird aktualisiert
**  base   : Zahlenbasis
**  return : die gelesene Zahl 0..255
*/
static int LOCAL getn ( const char** pstr, int base )
  {
  static const char errtxt [] = "beim Lesen einer Zahl aus den Massendaten";
  int result;
  if( base == 2 )
    result = *(const unsigned char*) ( (*pstr)++ );
  else
    {
    long zahl = strtol( *pstr, pstr, base );
    if( labs( zahl ) > 255 ||
      ( **pstr != '\0' && *(*pstr)++ != ',' ) ) error( errtxt );
    result = (int) zahl;
    if( base == 10 ) result += 128;
    }
  if( result < 0 || result > 255 ) error( errtxt );
  return result;
  }

/*
**  Feststellen in welcher Zahlenbasis der Trace gespeichert ist
**  pstr   : Zeiger auf String, enthält Daten eines Trace der Gould
**           *ptr wird hinter die Zahlenbasisangabe gestellt
**  return : Zahlenbasis
*/
static int LOCAL numberbase ( const char** pstr )
  {
  int base = 10;
  if( **pstr == '#' )
    {
    switch( *(++*pstr) )
      {
      case 'B': base = 2;
                *pstr += 2;
                break;
      case 'O': base = 8;
                break;
      case 'H': base = 16;
                break;
      default : error( "Unbekannte Zahlen-Basis" );
      }
    ++*pstr;
    }
  return base;
  }

/*
**  Einlesen eines Kanals
**  str    : Transferierte Daten von der Gould
**  shift  : Horizontal shift +MAXSHIFT .. -MAXSHIFT
**           Shift gibt die Position der Bildmitte im Trace
**           +MAXSHIFT : Anfang-Trace in der Bildmitte,
**           -MAXSHIFT : Ende-Trace in der Bildmitte
**  ex     : Horizontal Expansion 1 .. 20
**           Vergrößerungsfaktor
**  buffer : dort die Daten ggf. expandiert speichern
*/
static void LOCAL readtrace ( const char* str, int shift, int ex, tracedata* buffer )
  {
  const int len = ex == 1 ? TRACELEN : PLOTLEN;
  const int base = numberbase( &str );
  int akku,start,i,j;
  long c = ( MAXSHIFT - shift ) * (long) TRACELEN;
  i = (int) ( c / ( 2 * MAXSHIFT ) );
  j = len / ex;
  start = i - j / 2;
  if( start < 0 ) start = 0;
  if( start + j > len ) start = TRACELEN - j;
  for( i = 0; i <= start; ++i ) akku = getn( &str, base );
  i = 0;
  while( i < len )
    {
    const int next = getn( &str, base );
    const int diff = next - akku;
    int zaehler = 0;
    for( j = 0; j < ex && i < len; ++j )
      {
      buffer->data[i++] = (unsigned char) ( akku + zaehler / ex );
      zaehler += diff;
      }
    akku = next;
    }
  buffer->len = len;
  }

/*
**  Ausgeben der Texte zu den Kanälen
**  kanaele : die auszugebenden Daten
**  ex      : die horizontale Expansion
**  output  : in das File ausgeben, write binary
*/
static void LOCAL austexte ( const kanal kanaele[MAXCHANNELS], int ex, FILE* output )
  {
  int i;
  for( i = 0; i < MAXCHANNELS; ++i )
    if( kanaele[i].trace != NULL && gesperrt[i] == 0 )
      {
      fprintf( output, "Kanal %d : ", i+1 );
      if( kanaele[i].signal != NULL )
        {
        fputs( kanaele[i].signal, output );
        printer_code( _lineFeed, output );
        }
      aushscale( kanaele[i].hscale, ex, output );
      ausvscale( kanaele[i].vscale, output );
      if( integrieren[i] )
        {
        printer_code( _lineFeed, output );
        fprintf( output, "Kanal %d zeitintegriert.", i+1 );
        }
      printer_code( _lineFeed, output );
      }
  }

/*
**  Ausgeben der Daten über das Experiment
**  exp    : die auszugebenden Daten
**  output : in das File ausgeben, write binary
*/
static void LOCAL ausversuch ( const versuch* exp, FILE* output )
  {
  if( exp->name != NULL )
    {
    printer_code( _boldOn, output );
    fputs( exp->name, output );
    printer_code( _boldOff, output );
    }
  else
    fputs( "Daten gelesen", output );
  if( exp->date != NULL )
    fprintf( output, " am %s", exp->date );
  if( exp->time != NULL )
    fprintf( output, " um %s", exp->time );
  printer_code( _lineFeed, output );
  if( exp->nummer != NULL )
    {
    fputs( "Nummer: ", output );
    printer_code( _boldOn, output );
    fputs( exp->nummer, output );
    printer_code( _boldOff, output );
    printer_code( _lineFeed, output );
    }
  }

/*
**  Ausplotten eines Trace in das Ausagbefile
**  file   : in das File ausgeben, write binary
**  buffer : Daten mit dem Trace
**  unit   : Pixel pro Einheit 
*/
static void LOCAL austrace ( FILE* file, const tracedata* buffer, int unit )
  {
  if( plot( file, buffer->data, buffer->len, unit ) )
     error( "gemeldet vom Plot-Modul" );
  }

/*
**  zeitliches integrieren eines Kanals
**  buffer   : Daten des Kanals, dort auch das integrierte Signal speichern
**  position : String mit Angabe vertikal Position des Kanals auf dem Scope
**             oder NULL, dann wird die Lage aus den ersten 20 Samples 
**             durch Mitteln bestimmt.   
*/
static void LOCAL integrieretrace ( tracedata* buffer, const char* position )
  {
  long iposition;
  int i;
  long akku, max, min, divi;
  if( position != NULL )
    {
    float fposition;
    if( sscanf( position, "%f", &fposition ) != 1 )
      iposition = 0L;
    else
      iposition = (long) ( ( fposition + 4.5 ) * (255.0/9.0) * 20.0 );
    }
  else
    {
    iposition = 0L;
    for( i = 0; i < 20; ++i ) iposition += (long) buffer->data[i];
    }
  akku = (int) buffer->data[0];
  max = min = akku - iposition/20;
  for( i = 1; i < buffer->len; ++i )
    {
    long p;
    akku += (int) buffer->data[i];
    p = akku - ( iposition * (long) i )/20;
    if( p < min )
      min = p;
    else if ( p > max )
      max = p;
    }
  divi = ( max - min + 254 ) / 255;
  if( divi == 0L ) divi = 1L;
  akku = 0L;
  for( i = 0; i < buffer->len; ++i )
    {
    long p;
    akku += (int) buffer->data[i];
    p = akku - ( iposition * (long) i )/20;
    buffer->data[i] = (unsigned char) ( ( p - min )/ divi );
    }
  }

/*
**  Bearbeiten eines OSZ-Files
**  fname : Name des OSZ-Files
**  ofile : Ausgabefile zum Drucker
*/
static void LOCAL verarbeitung ( const char* fname, FILE* ofile )
  {
  kanal kanaele [MAXCHANNELS];
  versuch exp;
  int i;
  int shift = cmdline_shift;
  int expansion = cmdline_expansion;
  char* daten = ladeosz( fname );
  tracedata* buffer = Malloc( sizeof (tracedata), "Buffer für einen Trace" );
  memset( kanaele, 0, sizeof kanaele );
  memset( &exp, 0, sizeof exp );
  exp.expansion = 1;
  lese( daten, kanaele, &exp );
  if( cmdline_expansion == INT_MAX ) expansion = exp.expansion;
  if( cmdline_shift == INT_MAX ) shift = exp.shift;
  printer_code( _reset, ofile );
  printer_code( _ibmzeichen, ofile );
  ausversuch( &exp, ofile );
  for( i = 0; i < MAXCHANNELS; ++i )
    if( kanaele[i].trace != NULL && gesperrt[i] == 0 )
      {
      readtrace( kanaele[i].trace, shift, expansion, buffer );
      austrace( ofile, buffer, 2 );
      if( integrieren[i] )
        {
        integrieretrace( buffer, integrieren[i] == 1 ? kanaele[i].position : NULL );
        austrace( ofile, buffer, 1 );
        }
      }
  austexte( kanaele, expansion, ofile );
  printer_code( _pageFeed, ofile );
  printer_code( _reset, ofile );
  free( daten );
  }

/*
**  Liest eine Zahl aus einem String und Überprüft den Wertebereich
**  arg  : String mit der Zahl
**  min  : minimal erlaubter Wert
**  max  : maximal erlaubter Wert
**  name : Name der Größe (für Fehlermeldung)
*/
static int LOCAL checkrange ( const char* arg, int min, int max, const char* name )
  {
  int result = atoi( arg );
  if( result < min )
    error( "%s mindestens %d.", name, min );
  if( result > max )
    error( "%s maximal %d.", name, max );
  return result;
  }

/*
**  Horizontale Expansions Angabe aus Kommanodzeile verarbeiten
**  arg : String, die gewünschte Expansion
*/
static void LOCAL setexpansion ( const char* arg )
  {
  if( *arg == '\0' )
    cmdline_expansion = INT_MAX;
  else
    cmdline_expansion = checkrange( arg, 1, 20, "Horizontale Expansion" );
  }

/*
**  Horizontale Shift Angabe aus der Kommandozeile lesen
**  arg : String, der gewünschte Shift
*/
static void LOCAL setshift ( const char* arg )
  {
  if( *arg == '\0' )
    cmdline_shift = INT_MAX;
  else
    cmdline_shift = checkrange( arg, -MAXSHIFT, +MAXSHIFT, "Horizontaler Shift" );
  }

/*
**  Angabe zum Löschen der OSZ-Files lesen
**  str : String, die Angabe aus der Kommandozeile
*/
static void LOCAL setdelete ( const char* str )
  {
  if( str[1] != '\0' || ( str[0] != '+' && str[0] != '-' ) )
    error( "Falsche Delete-Angabe: '%s', nur + oder - erlaubt.", str );
  else
    deleteosz = str[0] == '+';
  }

/*
**  Kanäle für die Ausgabe sperren
**  str : String, die Nummer aus der Kommanmdozeile
*/
static void LOCAL setsperre ( const char* str )
  {
  if( *str == '\0' )
    {
    int i;
    for( i = 0; i < MAXCHANNELS; ++i ) gesperrt[i] = 0;
    }
  else
    {
    const int akku =  atoi( str );
    if( akku > MAXCHANNELS || akku < 1 )
      error( "Nur Kanäle 1.." MAXCHANNELS_STR " können gesperrt werden." );
    else
      gesperrt[akku-1] = 1;
    }
  }

/*
**  Kanal für das Integrieren auswählen
**  str : String, die Nummer aus der Kommandozeile
**  art : Art der Nullpunktswahl 
*/
static void setintegrieren ( const char* str, int art )
  {
  const int i = atoi( str );
  if( i < 1 || i > MAXCHANNELS )
    error( "Nur Kanäle 1.." MAXCHANNELS_STR " können integriert werden." );
  else
    integrieren[i-1] = art;
  }

/*
**  Druckertyp auswählen
**  name : Name des Druckers
*/
static void setprinter ( const char* name )
  {
  const enum printer_t typ = printer_search( name );
  if( typ == _noPrinter )
    error( "Unbekannter Druckertyp angegeben." );
  else
    printer_set( typ );
  }

/*
**  Schalter aus Kommandozeile auswerten
**  str : String mit dem Schalter
*/
static void LOCAL schalter ( const char* str )
  {
  switch( str[0] )
    {
    case 'o' : if( ofile != NULL ) fclose( ofile );
               ofile = openout( str + 1 );
               break;
    case 'p' : setprinter( str + 1 );
               break;
    case 'e' : setexpansion( str + 1 );
               break;
    case 's' : setshift( str + 1 );
               break;
    case 'd' : setdelete( str + 1 );
               break;
    case 'k' : setsperre( str + 1 );
               break;
    case 'i' : setintegrieren( str + 1, 1 );
               break;
    case 'I' : setintegrieren( str + 1, 2 );
               break;
    default  : error( "Unbekannter Schalter %s.", str );
    }
  }

/*
**  Loeschen eines Files und Kontrollausgabe
**  fname : Name des Files (mit Pfad und Extension)
*/
static void loesche ( const char* fname )
  {
  const int status = unlink( fname );
  printf( "\r%40s %s", fname, status ? "   can't delete" : "   deleted" );
  }

/*
**  OSZ-Filenamen aus der Kommando-Zeile bearbeiten
**  fname : Name des OSZ-Files, kann Joker *, ? enthalten
*/
static void LOCAL oszfiles ( const char* fname )
  {
  const char* filename;
  if( ofile == NULL ) ofile = openout( "PRN" );
  filename = dirget_first( fname );
  while( filename != NULL )
    {
    printf( "\n%40s", filename );
    verarbeitung( filename, ofile );
    if( deleteosz ) loesche( filename );
    filename = dirget_next();
    }
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  if( argc < 2)
    {
    int i;
    for( i = 0; manual[i] != NULL; ++i )
      puts( manual[i] );
    exit( 1 );
    }
  plot_init();
  plot_setmode( _plotcuttop | _plotcutbottom );
  while( ++argv, --argc )
    if( **argv == '-' )
      schalter( *argv + 1 );
    else
      oszfiles( *argv );
  if( ofile != NULL ) fclose( ofile );
  return 0;
  }
