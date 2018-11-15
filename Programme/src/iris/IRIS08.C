/*
**  C  ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .08.1993
**
**  Funktionen zur Ansteuerung der "DT2853 high resolution frame grabber"
**  Karte mit der der "DT2859 video multiplexer" Karte. 
**
**  Die Funktionen dieses Moduls sind dem Programm IS30.C angelehnt.
**  (Programm IS30.C von D.Meiners, H.Wenz und J.Westheide)
**
**  23.08.1993  Beginn
*/

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iris.h"
#include "isdefs.h"

#ifndef MK_FP
#  define MK_FP( seg, off ) ((void _far*) ( (unsigned long)(seg) << 16 + (off)))
#endif

/*
**  Anzahl der angeschlossenen Kameras
*/
#define ANZAHLCAM 3

/*
**  freier Timer-Interrupt des IBM-BIOS
*/
#define TIMERINT 0x1C

/*
**  File mit der Adresse der Multiplexerkarte im Klartext
**  Das File wird von dem Programm C:\DT_IRIS\IS_STORE.EXE angelegt, und
**  hat das Format XXXX:XXXX (X = Hexziffer)
*/
static const char* isdriver = "C:\S_DRIV.ADR";

/*
**  Falls eine is_initialize fehlgeschlagen, Benutzer entscheidet über
**  Programmabbruch
*/
static void LOCAL isiniterror ( void )
  {
  fputs( "Fehler vom DT-IRIS Treiber bei der Initialisierung\n"
	 "Programm abbrechen", stdout );
  if( janein() )
    exit( 1 );
  else
    is_initialize();
  }

/*
**  Far-Adresse aus File lesen
**  fname  : Name des Files
**  ptr    : dort den Pointer speichern
**  return : alles ok ?
*/
static int LOCAL readadr ( const char* fname, char _far* *ptr )
  {
  FILE* file = fopen( fname, "rt" );
  int ok = file != NULL;
  if( ok )
    {
    unsigned segment, offset;
    ok = fscanf( file, "%X:%X", &segment, &offset ) == 2;
    *ptr = MK_FP( segment, offset );
    fclose( file );
    }
  if( ! ok )
    printf( "Kann IS-Driver Adresse aus File '%s' nicht lesen.", fname );
  return ok;
  }

/*
**  testet ob Adresse vom IS-Treiber ist
**  ptr    : die Adresse
**  return : stimmt sdie Adresse ?
*/
static int LOCAL okisadresse ( char _far* ptr )
  {
  return ptr[0x0010] == 0x35 && ptr[0x0011] == 0x33;
  }

/*
**  Liest einen Interruptvektor
**  nr     : Nummer der Vektors ( 0 .. 255 )
**  return : der Vektor (far-adresse)
*/
static void _far* LOCAL getvector ( int nr )
  {
  void _far* _far* tabelle = MK_FP( 0x0000, 0x0000 );
  return tabelle[ nr ];
  }

/*
**  Schreibt die Codes zum Umschalten der Multiplexerkarte
**  adr : Adresse der Karte
**  i   : Index des Framegrabbers ( 0..3 )
*/
static void schaltemulti ( char _far* adr, int i )
  {
  static char code0 [4] = { 0x30, 0x50, 0x70, 0x90 };
  static char code1 [4] = { 0xA0, 0xB0, 0xC0, 0xD0 };
  static char code2 [4] = { 0xA3, 0xB3, 0xC3, 0xD3 };
  adr[0x0016] = code0[ i ];
  adr[0x001A] = code1[ i ];
  adr[0x00CF] = code2[ i ];
  }

/*
**  Umschalten Framegrabber bei installierter Multiplexerkarte
**  Zum Umschalten muß die Adresse des Treibers der Karte bestimmt werden
**  dazu: erst über den Timer-Interrupt versuchen den Treiber zu finde,
**  sonst die Adresse aus dem Filelesen.
**  nr     : Nummer des Framegrabbers (1..4)
**  return : ist ein Fehler aufgetreten ?
*/
static int schalteis ( int nr )
  {
  static char _far* loadadresse = NULL;
  char _far* isadresse = (char _far*) getvector( TIMERINT ) - 0x0421;
  int ok;
  ok = okisadresse( isadresse );
  if( ! ok )
    {
    if( loadadresse == NULL )
      ok = readadr( isdriver, &loadadresse );
    else
      ok = 1;
    isadresse = loadadresse;
    ok = ok && okisadresse( isadresse );
    }
  if( ok )
    schaltemulti( isadresse, nr - 1 );
  else
    puts( "Kann Adresse zum Umschalten der Multiplexerkarte nicht finden." );
  return ! ok;
  }

/*
**  Warten auf Bildaufnahme, kann vom Benutzer abgebrochen werden
**  return : alles ok ? (sonst vom Benuzter abgebrochen)
*/
static int LOCAL warte ( void )
  {
  int ok = 1;
  int aufgenommen = 0;
  schalteis(1);
  while( ok && ! aufgenommen )
    {
    ok = ! getstop();
    is_check_acquisition_complete( &aufgenommen );
    }
  return ok;
  }

/*
**  Bilder aufnehmen
**  (wird für alle Kameras durchlaufen, aber nur beim letzten Durchlauf
**   mit Kamera-Nr. 0 wird auf externen Triggerpuls gewartet.)
**  frb    : Framebuffer Nummer
**  nr     : Nummer der Kamera
**  return : alles ok ?
*/
static int LOCAL lesekamera ( int frb, int nr )
  {
  int ok;
  schalteis( nr );
  is_reset();
  is_end();
  if( is_initialize() ) isiniterror();
  is_select_output_frame( frb );
  is_display( 1 );
  is_set_sync_source( 1 );
  is_passthru();
  is_acquire_external( frb, 1 );
  ok = warte();
  if( ! ok ) is_reset();
  return ok;
  }

/*
**  Initialisieren eines Kanals der Karte
**  nr : Nummer des Kanals (1..4)
*/
static void frameinit ( int nr )
  {
  if( is_initialize() ) isiniterror();
  schalteis( nr );
  is_reset();
  is_end();
  }

/*
**  Initialisieren der Bildaufnahme-Karte
**  und aktueller Framegrabber = 1.Framegrabber
*/
static void LOCAL init ( void )
  {
  int i;
  for( i = ANZAHLCAM; i > 0; --i ) frameinit( i );
  if( is_initialize() ) isiniterror();
  is_reset();
  is_select_output_frame( 0 );
  is_display( 1 );
  is_select_channel( 1 );
  }

/*
**  Framegrabber-Karte initialisieren
**  isfile : Name des Files, der Adresse der Multiplexerkarte enthält
**           falls NULL, wird der Standartname benutzt
*/
void frg_init ( const char* isfile )
  {
  fputs( "Initialisieren (löscht auch die Bilder)", stdout );
  if( isfile != NULL ) isdriver = isfile;
  if( janein() )
    init();
  else
    if( is_initialize() ) isiniterror();
  }

/*
** Framegrabber-Karte terminieren
*/
void frg_termit ( void )
  {
  is_reset();
  is_display( 0 );
  is_end();
  }

/*
**  eine Framegrabber auf den Monitor bringen
**  nummer : der Framebuffers 1 .. 4
*/
void frg_framegrabber ( int nummer )
  {
  int fehler;
  if( nummer < 1 || nummer > 4 ) nummer = 1;
  fehler = is_end()
	   || schalteis( nummer )
	   || is_initialize()
	   || is_select_channel( nummer );
  if( fehler ) getreturn();
  }

/*
**  Kameras zurücksetzen / ext. Trigger / Bilder aufnehmen
**  frb    : Framebuffer Nummer
**  return : Grab gelungen ? (sonst vom Benutzer abgebrochen)
*/
int frg_aufnehmen ( int frb )
  {
  int ok;
  fputs( "Die Bilder werden jetzt gelöscht, weiter", stdout );
  ok = janein();
  if( ok );
    {
    int i = ANZAHLCAM;
    puts( "Warten bis Bilder aufgenommen, Abbruch mit Taste 's' möglich." );
    while( i > 0 && ok ) ok = lesekamera( frb, i-- );
    frg_framegrabber( 1 );
    }
  return ok;
  }

/********************************************************************/

/*
**  Namen der Sicherungsfiles für die aufgenommenen Bilder
**  Name == NULL -> das Bild nicht sichern
*/
static char* savefilenames [ ANZAHLCAM ] = { NULL, NULL, NULL };

/*
**  Namen der Sicherungsfiles für die aufgenommen Bildern ändern
*/
void setsavefiles ( void )
  {
  int i;
  char buffer [ pathlen + namelen + extlen ];
  buffer[0] = '\0';
  puts( "Die Bilder der Kameras können nach der Aufnahme automatisch\n"
	"in Dateien gesichert werden.\n"
	"Die Dateinamen (kein Name => keine Sicherung):" );
  for( i = 0; i < ANZAHLCAM; ++i )
    {
    if( savefilenames[i] != NULL )
      {
      strcpy( buffer, savefilenames[i] );
      free( buffer );
      }
    printf( "%d>", i+1 );
    stredit( buffer, sizeof buffer );
    if( buffer[0] == '\0' )
      savefilenames[i] = NULL;
    else
      if( ( savefilenames[i] = strdup( buffer ) ) == NULL ) errmalloc();
    }
  }

/*
**  Bilder nach der Aufnahme in Files sichern
**  frb : Frmae Buffer Nummer
*/
void savefiles ( int frb )
  {
  int i;
  int lastframe = 1;
  for( i = ANZAHLCAM; i > 0; --i )
    if( savefilenames[i-1] != NULL )
      {
      printf( "Speichere Bild %d in File '%s'\n", i, savefilenames[i-1] );
      lastframe = i;
      frg_framegrabber( i );
      is_select_output_frame( frb );
      is_display( 1 );
      is_save( frb, 0,1,8, savefilenames[i-1] );
      }
  if( lastframe != 1 ) frg_framegrabber( 1 );
  }

