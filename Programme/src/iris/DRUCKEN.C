/*
**  C  ( Zortech C V2.1 / Mircosoft C 5.10 / Turbo C V 2.0 )
**
**  Ulrich Berntien .10.1992
**
**  27.10.1992  Beginn
**  05.11.1992  Korrekturen, Fehler beim Umgang mit dem Heap
**  08.01.1993  Ergänzungen, in den Wahlmöglichkeiten 'keine'
*/

/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <process.h>
#include <dos.h>

/********************************************************************/

#define namelen 80                    /* Bufferlänge für Dateinamen */

#if defined __ZTC__
  typedef struct FIND find;
  #define DATE date
  int _okbigbuf = 0;
#elif defined __TURBOC__
  #include <dir.h>
  typedef struct ffblk find;
  #define DATE ff_fdate
  #define _dos_findfirst( name,attr,data ) findfirst( name,data,attr )
#else
  typedef struct find_t find;
  #define DATE wr_date
#endif

static const char stdfile [] = "B:OUT.TXT";
static const char stdarc []  = "B:LOGFILES.LZH";
static const char printer [] = "B:FP.COM";
static const char arcer []   = "B:LHARC.EXE";

/********************************************************************/

/*
**  Nimmt ein Zeichen von der Tastatur entgegen
**  wandelt es in Kleinbuchstaben und führt Kontrolle durch
*/
static char getcharacter ( const char* erlaubt )
  {
  char buffer [100];
  int ok = 0;
  do{
    putchar( '>' );
    fgets( buffer, sizeof buffer, stdin );
    buffer[0] = tolower( buffer[0] );
    if( buffer[1] != '\n' )
      puts( "Nur ein Zeichen eingeben" );
    else if( strchr( erlaubt, buffer[0] ) == NULL )
      puts( "dieses Zeichen ist nicht erlaubt" );
    else
      ok = 1;
    }
    while( ! ok );
  return buffer[0];
  }

/*
**  Nimmt einen Dateinamen entgegen
*/
static void getname ( char* name )
  {
  char buffer [ namelen + 1 ];
  int ok = 0;
  do{
    int d = 0;
    int s = 0;
    fputs( "NAME>", stdout );
    fgets( buffer, sizeof buffer, stdin );
    while( s < namelen - 1 && buffer[s] <= 0x20 && buffer[s] != 0 ) ++s;
    while( d < namelen - 1 && buffer[s] != '\n' )
      name[d++] = toupper( buffer[s++] );
    name[d] = '\0';
    if( buffer[s] != '\n' )
      printf( "Name ist zu lang, maximal %d Zeichen\n", namelen );
    else
      ok = 1;
    }
    while( ! ok );
  }

/*
**  Stellt fest ob eine Datei existiert
**  falls ja, wird ein Zeiger auf eine find Struktur
**  diese muß wieder freigegeben werden !
*/
static find* askfile ( const char* name )
  {
  find* data = malloc( sizeof (find) );
  if( data == NULL )
    {
    puts( "Speicher zu klein (malloc gescheitert)" );
    exit( 1 );
    }
  if( _dos_findfirst( (char*) name, 0, data ) != 0 )
    {
    free( data );
    data = NULL;
    }
  return data;
  }

/*
**  Name "X:datum.LOG" erstellen
**  das >datum< wird aus 'find' gelesen, das Drive >X:< wird aus 'org' gelesen
*/
static void datumsname( const find* data, const char* org, char* name )
  {
  char drive [3];
  struct datum
    {
    unsigned tag   : 5;
    unsigned monat : 4;
    unsigned jahr  : 6;
    }
    date;

  memcpy( &date, &data->DATE, 2 );
  if( org[1] == ':' )
    {
    drive[0] = org[0];
    drive[1] = ':';
    drive[2] = '\0';
    }
  else
    drive[0] = '\0';
  sprintf( name, "%s%02d%02d%04d.LOG",
             drive, date.tag, date.monat, date.jahr+1980 );
  }

/********************************************************************/

/*
**  Eingabe eines Namens einer bestehenden Datei
**  die Variable '*data' wird von dieser Function angelegt !
*/
static find* existfile ( char* name )
  {
  find* data;
  do{
    getname( name );
    data = askfile( name );
    if( data == NULL )
      printf( "Datei '%s' existiert nicht\n", name );
    }
    while( data == NULL );
  return data;
  }

/*
**  Eingabe eines Namens einer nicht bestehenden Datei
*/
static void notexistfile ( char* name )
  {
  find* data = NULL;
  do{
    if( data != NULL ) free( data );
    getname( name );
    data = askfile( name );
    if( data != NULL )
      printf( "Datei '%s' existiert schon\n", name );
    }
    while( data != NULL );
  }

/*
**  Ausführen des Druckprogrammes
*/
static void callprinter ( const char* name )
  {
  find* data = askfile( printer );
  if( data == NULL )
    {
    printf( "Das Druckprogramm '%s' fehlt\n", printer );
    exit( 1 );
    }
  else
    free( data );
  if( spawnl( P_WAIT, (char*) printer, (char*) printer, name, NULL ) == - 1 )
    {
    printf( "Das Druckprogramm '%s' kann nicht gestartet werden\n", printer );
    exit( 1 );
    }
  }

/*
**  Datei archivieren
*/
static void archivieren ( const char* name, const char* archiv )
  {
  find* data = askfile( arcer );
  if( data == NULL )
    {
    printf( "Archivierungsprogramm '%s' nicht gefunden\n", arcer );
    exit( 1 );
    }
  else
    free( data );
  if( spawnl( P_WAIT, (char*) arcer, (char*) arcer, "a", archiv, name, NULL ) == -1 )
    {
    printf( "Das Archivprogramm '%s' kann nicht gestartet werden\n", arcer );
    exit( 1 );
    }
  }

/*
**  Datei umbennen
*/
static void umbennen ( const char* von, const char* nach )
  {
  if( rename( von, nach ) != 0 )
    {
    printf( "REN %s %s  gescheitert\n", von, nach );
    exit( 1 );
    }
  }

/*
**  Datei löschen
*/
static void loeschen ( const char* name )
  {
  if( remove( name ) != 0 )
    {
    printf( "DEL %s  gescheitert\n", name );
    exit( 1 );
    }
  }

/*
**  Dialog zum Auswählen des zu druckenden Files
**  return : != 0 wenn Datei gedruckt werden soll
*/
static int waehlprintfile ( char* name, find** pfind )
  {
  char wahl;
  find* data = askfile( name );
  if( data == NULL )
    {
    printf( "Standartdatei '%s' existiert nicht.\n"
            "Welche Datei ausdrucken ?\n" , name );
    data = existfile( name );
    }
  do{
    printf( "Datei '%s' ausdrucken ? (j)a, (a)ndere, (k)eine, (e)nde\n", name );
    wahl = getcharacter( "jake" );
    if( wahl == 'a' )
      {
      free( data );
      data = existfile( name );
      }
    else if( wahl == 'e' )
      exit( 0 );
    }
    while( wahl != 'j' && wahl != 'k' );
  *pfind = data;
  return wahl == 'j';
  }

/*
**  Auswahl des neuen Filenamens
**  return : != 0, falls Datei umbenannt werden soll
*/
static int waehlrenfile ( char* name )
  {
  char wahl;
  find* data = askfile( name );
  if( data != NULL )
    {
    printf( "Die Datei '%s' existiert bereits\n"
            "Welchen neuen Namen soll die Druckdatei bekommen ?\n"
            , name );
    notexistfile( name );
    free( data );
    }
  do{
    printf( "Druckdatei in '%s' umbennen ? (j)a, (a)anderer, (k)ein, (e)ende\n", name );
    wahl = getcharacter( "jake" );
    if( wahl == 'a' )
      notexistfile( name );
    else if( wahl == 'e' )
      exit( 0 );
    }
    while( wahl != 'j' && wahl != 'k' );
  return wahl == 'j';
  }

/*
**  Auswahl des Archivirungsfiles
**  return : != 0 wenn Datei archiviert werden soll
*/
static int waehlarcfile( char* name )
  {
  char wahl;
  do{
    printf( "In Datei '%s' archivieren ? (j)a, (a)ndere, (k)eine, (e)nde\n", name );
    wahl = getcharacter( "jake" );
    if( wahl == 'a' )
      getname( name );
    else if( wahl == 'e' )
      exit( 0 );
    }
    while( wahl != 'j' && wahl != 'k' );
  return wahl == 'j';
  }

/*
**  Nachfrage ob gelöscht werden soll
**  return : != 0 wenn Datei gelöscht werden soll
*/
static int waehldelete ( const char* name )
  {
  printf( "Datei '%s' löschen ? (j)a, (n)ein\n", name );
  return getcharacter( "jn" ) == 'j';
  }

/********************************************************************/

/*
**  M A I N
*/
int main ( int argc, const char* argv [] )
  {
  char printfile [ namelen ];
  find* printdata;
  char renfile [ namelen ];
  char arcfile [ namelen ];
  strcpy( printfile, argc == 2 ? argv[1] : stdfile );
  if( waehlprintfile( printfile, &printdata ) )
    callprinter( printfile );
  datumsname( printdata, printfile, renfile );
  if( waehlrenfile( renfile ) )
    umbennen( printfile, renfile );
  else
    strcpy( renfile, printfile );
  strcpy( arcfile, stdarc );
  if( waehlarcfile( arcfile ) )
    archivieren( renfile, arcfile );
  if( waehldelete( renfile ) )
    loeschen( renfile );
  return 0;
  }

