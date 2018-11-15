/*
**  C    ( Microsoft C V5.10 / Zortech C V2.1 / Turbo C V2.0 )
**
**  Ulrich Berntien .02.1993
**
**  Microsoft C V5.10 :
**    NICHT MIT OPTIMIERENDEN SCHALTER COMPILIEREN,
**    Optimierer beachtet nicht volatile Typ
**    Funktion 'sendcmd' arbeitet dann nicht korrekt.
**
**  Daten vom digital Speicheroszilloskop Gould 4074 abspeichern
**  Die Verbindung mit der Gould erfolgt über RS-232 Schnitstelle
**
**  08.02.1993  Beginn
**  24.06.1993  Namensvorgaben im Dialog ändern
**  24.07.1993  Vorgabefile kann auch gespeichert werden
**  26.07.1993  Funktion stredit eingesetzt
**  26.08.1993  Die Namen der Kanäle werden beim Datenempfang ausgeben
**  21.12.1994  Fragen nach "ADD12" und "ADD34" gelöscht
*/

#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "rs232.h"
#include "iris.h"

/*
**  Datenstruktur um Standart-Namen Vorgaben zu speichern
**  und Hilfsvorgaben, wenn das laden der Namen fehlschlägt
*/
enum defpositions { _autocal, _path, _ext, _experiment, _signal, _count = 8 };

static struct defnames
  {
  int maxlen;           /* \0 am Stringende mitgezählt         */
  char* name;           /* Zeiger auf den Namen                */
  int alloc;            /* Wurde für 'name' alloc ausgeführt ? */
  const char* what;     /* Bedeutung des Namens                */
  }
  standart [_count] =
  {
  4,       "OFF",      0, "Autocal nach Übertragung auslösen (OFF/ON)",
  pathlen, "",         0, "Pfad zum Speichern der OSZ-Files",
  extlen,  "OSZ",      0, "Namensextension der OSZ-Files",
  76,      "Focus",    0, "Name des Experiments",
  76,      "Signal 1", 0, "Signal auf 1. Kanal",
  76,      "Signal 2", 0, "Signal auf 2. Kanal",
  76,      "Signal 3", 0, "Signal auf 3. Kanal",
  76,      "Signal 4", 0, "Signal auf 4. Kanal"
  };

/*
**  In dieser Datei sind die Standart-Namen gespeichert
*/
TEXT standartfile [] = "IRIS_OSZ.TXT";

/*
**  allgemeine Fragen zu Einstellungen der Gould
*/
static char allgemeines [] =
  "CH*\r\n"
  "AUTCAL\r\n"
  "AVRG\r\n"
  "BWLIM\r\n"
  "HE\r\n"
  "HMOD\r\n"
  "HSA\r\n"
  "STAT\r\n"
  "SHFT\r\n"
  "TDELA\r\n"
  "TGAAUT\r\n"
  "TLA\r\n"
  "TRGCA\r\n"
  "TRGMDA\r\n"
  "TSA\r\n"
  "TSLA\r\n";

/*
**  Fragen zu einen bestimmenten Kanal
**  steht für ? die Kanalnummer
*/
TEXT prokanal [] =
  "HOLD?\r\n"
  "TRHS?A\r\n"
  "TRVS?A\r\n"
  "VC?\r\n"
  "VP?\r\n"
  "TRC?A\r\n";

/*
**  einen Befehl zur Gould senden, nicht auf Antwort warten
**  cmd    : diesen String zur Gould senden
**  return : Wurde Sendung abgebrochen ?
*/
static int LOCAL sendcmd ( const char* cmd )
  {
  const char *volatile *ptr = rs232send( cmd );
  while( *ptr != NULL && ! getstop() ) {}
  rs232clear();
  return *ptr != NULL;
  }

/*
**  Sucht nach Ende der Frage '\n', setzte dahinter ein '\0'
**  frage  : die Anfrage
**  ptr    : dort wird das erstzte Zeichen gesichert
**  return : Zeiger aus das '\0' am Ende der Frage
**           oder NULL, wenn ein Fehler aufgetreten
*/
static char* LOCAL macheende ( char* frage, char* ptr )
  {
  char* ende = strchr( frage, '\n' );
  if( ende == NULL )
    fputs( "IRIS06 : interner Fehler in 'macheende'\n", stderr );
  else
    {
    *ptr = *(++ende);
    *ende = '\0';
    }
  return ende;
  }

/*
**  eine Liste von Anfragen zur Gould übertragen
**  und die Antworten entgegennehmen
**  fragen : String, die einzelnen Fragen durch \r\n getrennt
**           die Zeichen im String werden zwischendurch verändert
**  buffer : für den Antwort String, die einzelnen Antworten
**           durch ; getrennt
**  return : auf erstes Zeichen im Buffer hinter den Antworten
**           auf Anfang des Buffers, wenn etwas fehlerhaft war
*/
static char* LOCAL frageantwort ( char* liste, char *const buffer )
  {
  int ok = 1;
  char* restbuffer = buffer;
  while( ok && *liste != '\0' )
    {
    volatile rs232buffer* ptr = rs232setbuffer( restbuffer );
    char akku;
    char *const endefrage = macheende( liste, &akku );
    const char *volatile *sptr = rs232send( liste );
    ok = endefrage != NULL;
    if( ok )
      {
      fputs( "\r  Sende  ", stdout );
      while( ok && *sptr != NULL ) ok = ! getstop();
      fputs( "\r  Empfang", stdout );
      while( ok && ptr->lfs == 0 )
        {
        char akku = *ptr->now;
        putchar( akku > 0x20 ? akku : '.' );
        putchar( '\b' );
        ok = ! getstop();
        }
      *endefrage = akku;
      liste = endefrage;
      }
    if( ptr->parityerrors || ptr->overflowerrors )
      printf( "\r  %d Bytes mit %d Parity und %d Overflow-Errors\n",
              ptr->now - ptr->start, ptr->parityerrors, ptr->overflowerrors );
    restbuffer = ptr->now;
    rs232clear();
    if( ok )
      {
      --restbuffer;
      while( restbuffer >= buffer && ! isprint( *restbuffer ) ) --restbuffer;
      *(++restbuffer) = ';';
      *(++restbuffer) = '\0';
      }
    }
  putchar( '\n' );
  return ok ? restbuffer : NULL;
  }

/*
**  String normieren nach Einlesen mit gets, \n am Ende entfernen
**  str    : der String
**  return : Länge des Strings mit '\0' oder -1, wenn \n im String fehlt
*/
static int LOCAL normstr ( char* str )
  {
  char* ptr = str;
  int result;
  while( *ptr != '\n' && *ptr != '\0' ) ++ptr;
  if( *ptr == '\0' )
    result = -1;
  else
    {
    *ptr = '\0';
    result = ptr - str + 1;
    }
  return result;
  }

/*
**  ';' im String durch ',' ersetzen
**  str : in diesem String wird die Ersetzung durchgeführt
*/
static void LOCAL delsemicolons ( char* str )
  {
  int i;
  for( i = 0; str[i] != '\0'; ++i )
    if( str[i] == ';' ) str[i] = ',';
  }

/*
**  Fehlermeldung im Zusammenhang mit Filezugriff ausgeben
**  msg  : Fehlermeldung, enthält eine %s für den Filenamen
**  file : Name des Files
*/
static void LOCAL fileerror ( const char* msg, const char* file )
  {
  fprintf( stderr, msg, file );
  fprintf( stderr, "Libary-Function meldet : %s\n", strerror( errno ) );
  }

/*
**  Einlesen der Standart-Namen aus dem File
*/
static void LOCAL initdefnames ( void )
  {
  FILE* input = fopen( standartfile, "r" );
  if( input == NULL )
    fileerror( "Kann Vorgabe-File '%s' nicht lesen.\n", standartfile );
  else
    {
    char buffer [200];
    int ok = 1;
    int i;
    printf( "Lese aus Namen-Vorgabe-File '%s'.\n", standartfile );
    for( i = 0; ok && i < _count; ++i )
      {
      char* ptr;
      int len;
      do{
        ok = fgets( buffer, sizeof buffer, input ) != NULL;
        if( ! ok )
          fileerror( "Fehler beim lesen aus File '%s'.\n", standartfile );
        else
          {
          ok = ( len = normstr( buffer ) ) >= 0;
          if( ! ok ) fputs( "Buffer überlauf beim Einlesen.\n", stderr );
          }
        }
        while( ok && buffer[0] == ';' );
      if( ok )
        {
        ok = len <= standart[i].maxlen;
        if( ! ok ) fputs( "String zu lang.\n", stderr );
        }
      if( ok )
        {
        ok = ( ptr = malloc( len ) ) != NULL;
        if( ! ok ) errmalloc();
        }
      if( ok )
        {
        standart[i].name = ptr;
        delsemicolons( buffer );
        strcpy( ptr, buffer );
        }
      }
    }
  fclose( input );
  }

/*
**  Einstellen der Schnittstelle falls noch nicht erfolgt
**  force  : Initialisierung erzwingen
**  return : 1, wenn initialisiert wurde
*/
static int LOCAL init232 ( int force )
  {
  static int initok = 0;
  const int result = ! initok || force;
  if( result )
    {
    rs232parameter parameter;
    parameter.com = 1;
    parameter.baud = 9600;
    parameter.databits = 7;
    parameter.stopbits = 1;
    parameter.parity = _rs232even;
    puts( "COM1 : 9600,7,even,1 XON/XOFF" );
    initok = ! rs232setparameter( &parameter );
    }
  return result;
  }

/*
**  Die Gould initialisieren
**  . erster Befehl wird manchmal nicht bearbeitet von der Gould
**  . die großen Zahlenmengen in hex übertragen
*/
static void LOCAL initgould ( void )
  {
  sendcmd( "NB=HEX\r\n"
           "NB=HEX\r\n" );
  }

/*
**  erzeugt aus der globalen Konstante 'prokanal' einen String
**  für einen bestimmten Kanal
**  kanal  : Nummer der Kanals 0..9
**  return : NULL, wenn etwas nicht stimmt
**           sonst Zeiger auf den String, malloc muß später erfolgen (!)
*/
static char* LOCAL fragekanal ( int kanal )
  {
  char *const buffer = malloc( sizeof prokanal );
  if( buffer == NULL )
    errmalloc();
  else
    {
    const char ckanal = (char) kanal + '0';
    const char* source = prokanal;
    char* dest = buffer;
    char akku;
    while( ( akku = *dest++ = *source++ ) != '\0' )
      if( akku == '?' ) dest[-1] = ckanal;
    }
  return buffer;
  }

/*
**  Sucht in einem String nach einem Teilstring
**  in     : in diesem String wird gesucht
**  was    : nach diesem String wird gesucht
**  return : Zeiger hinter die passende Stelle oder NULL
*/
static const char* LOCAL strfind ( const char* in, const char* was )
  {
  while( *in )
    {
    int i = 0;
    while( in[i] == was[i] && was[i] != '\0' ) ++i;
    if( was[i] == '\0' ) return in + i;
    ++in;
    }
  return NULL;
  }

/*
**  Sucht im String nach einer Zeichenkette stellt fest was dahinter steht
**  suche  : der String in dem gesucht wird
**  was    : der String nach dem gesucht wird
**  return : 1, falls hinter der Zeichenkette "ON" steht
*/
static int LOCAL askon ( const char* suche, const char* was )
  {
  int result;
  const char* hit = strfind( suche, was );
  if( hit != NULL )
    result = strncmp( hit, "ON", 2 ) == 0;
  else
    result = 0;
  return result;
  }

/*
**  Alle gewünschten Daten von der Gould abfragen
**  buffer : Buffer für die Daten
**  return : Zeiger auf empfangene Daten oder NULL, wenn Abfrage fehlgeschlagen
*/
static const char* LOCAL getdaten ( char* buffer )
  {
  char* rest = frageantwort( allgemeines, buffer );
  if( rest != NULL )
    {
    static char kanal [5] = "CHx=";
    int i;
    for( i = 1; i < 5 && rest != NULL; ++i )
      {
      kanal[2] = (char) i + '0';
      if( askon( buffer, kanal ) )
        {
        char* anfrage = fragekanal( i );
        rest += sprintf( rest, "CH%cSIGNAL=%s;", i + '0', standart[i+_signal-1].name );
	printf( "Kanal %d \"%s\"\n", i, standart[_signal+i-1].name );
        if( anfrage != NULL )
          rest = frageantwort( anfrage, rest );
        free( anfrage );
        }
      }
    if( rest != NULL ) rest[-1] = '\0';
    }
  return rest == NULL ? NULL : buffer;
  }

/*
**  Schutztest ob die zu schreibende Datei bereits existiert
**  fname  : Name der Datei (mit Pfad und Erweiterung)
**  return : Wurde der Name abgelehnt ?
*/
static int LOCAL dateischutz ( const char* fname )
  {
  int result;
  if( access( fname, 0 ) == 0 )
    {
    printf( "File '%s' existiert bereits, überschreiben", fname );
    result = ! janein();
    }
  else
    result = 0;
  return result;
  }

/*
**  Schreibt Datum und Uhrzeit als Strings in das File
**  output : File, write
*/
static void LOCAL fdatetime ( FILE* output )
  {
  const time_t gmt = time( NULL );
  const struct tm *local = localtime( &gmt );
  fprintf( output, "DATE=%d.%02d.%d;TIME=%02d:%02d;"
                 , local->tm_mday, local->tm_mon + 1, local->tm_year + 1900
                 , local->tm_hour, local->tm_min );
  }

/*
**  Daten von Gould in File abspeichern
**  data : String mit den Daten
*/
static void LOCAL speicher ( const char* daten )
  {
  static char name [ namelen ];
  char path [ namelen + pathlen + extlen ];
  FILE* output;
  int saved = 0;
  do{
    do{
      fputs( "Schußnummer = Datei-", stdout );
      getfname( name, namelen );
      strcpy( path, standart[_path].name );
      dateiname( path, name, standart[_ext].name );
      }
      while( dateischutz( path ) );
    output = fopen( path, "wb" );
    if( output == NULL )
      fileerror( "Kann File '%s' nicht erstellen.\n", path );
    else
      {
      printf( "Schreibe Daten in File '%s'.\n", path );
      fprintf( output, "SCHUSSNR=%s;EXPNAME=%s;", name, standart[_experiment].name );
      fdatetime( output );
      if( fputs( daten, output ) )
        fputs( "Fehler beim Schreiben.\n", stderr );
      else
        saved = 1;
      fclose( output );
      }
    if( ! saved ) fputs( "Daten nicht gespeichert, weiter", stdout );
    }
    while( ! saved && ! janein() );
  }

/*
**  ggf. Nachfragen ob Namen gelesen werden sollen
**  fage   : Soll nachgefragt werden ?
**  return : Sollen die Namen gelesen werden ?
*/
static int LOCAL fragenamen ( int frage )
  {
  int result;
  if( frage )
    {
    fputs( "Namen der Signale aus Vorgabefile lesen", stdout );
    result = janein();
    }
  else
    result = 1;
  return result;
  }

/*
**  Daten von der Gould holen und auf Disk speichern
**  forceinit : Initialisieung der Schnittstelle erzwingen
*/
void goulddaten ( int forceinit, int asknames )
  {
  char* buffer = malloc( 17 * 1024 );
  puts( "Datentransfer von Gould, kann mit 's' abgebrochen werden." );
  if( init232( forceinit ) )
    {
    if( fragenamen( asknames ) ) initdefnames();
    initgould();
    }
/* hier darf kein printf o.ä. stehen, sonst **
** funktioniert der Datenempfang nicht      */
  if( buffer == NULL )
    errmalloc();
  else
    {
    const char* daten = getdaten( buffer );
    if( daten )
      {
      printf( "Experiment \"%s\"\n", standart[_experiment].name );
      speicher( daten );
      if( strcmp( standart[_autocal].name, "ON" ) == 0 )
        sendcmd( "AUTCAL=FORCE\r\n" );
      }
    }
  free( buffer );
  }

/*
**  Einen Standartnamen vom Benutzer abfragen
**  ptr : Zeiger auf die Struktur mit dem Standartnamen
*/
static void LOCAL changename ( struct defnames* ptr )
  {
  char buffer[80];
  char* dup;
  printf( "  %s\n>", ptr->what );
  strcpy( buffer, ptr->name );
  stredit( buffer, ptr->maxlen );
  dup = strdup( buffer );
  if( dup == NULL )
    puts( "Out of Heap, kann neuen Namen nicht speichern." );
  else
    {
    if( ptr->alloc )
      free( ptr->name );
    else
      ptr->alloc = 1;
    ptr->name = dup;
    }
  }

/*
**  die Vorgaben 'standart' wieder im 'standartfile' speichern
*/
static void LOCAL savenamen ( void )
  {
  FILE* file = fopen( standartfile, "w" );
  if( file != NULL )
    {
    int i;
    errno = 0;
    fputs( "; Namen für IRIS-Programm, (IRIS06) Datenübertragung Gould -> PC\n",
           file );
    for( i = 0; i < _count; ++i )
      fprintf( file, "; %s\n%s\n", standart[i].what, standart[i].name );
    if( errno != 0 )
      {
      fileerror( "Fehler beim Schreiben in Datei '%s'\n", standartfile );
      getreturn();
      }
    }
  else
    {
    fileerror( "Kann die Datei '%s' nicht zum Schreiben öffnen\n", standartfile );
    getreturn();
    }
  fclose( file );
  }

/*
**  Namensvorgaben für Experiment und Kanäle ändern
*/
void gouldnamen ( void )
  {
  int i;
  puts( "Namensvorgaben für das OSZ-File ändern.\n"
	"Hilfe zum Editfeld mit Taste <F01>" );
  if( init232( 0 ) && fragenamen( 1 ) ) initdefnames();
  for( i = 0; i < _count; ++i ) changename( standart + i );
  printf( "Geänderte Namen im Vorgabefile '%s' speichern", standartfile );
  if( janein() ) savenamen();
  }

