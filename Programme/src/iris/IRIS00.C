/*
**  C    ( Zortech C V2.1 / Micrsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .10.1992
**
**  Benutzereingaben von der Tastatur entgegennehmen
**  und andere kleine Hilfsfunktionen für den Benutzerdialog
**
**  03.10.1992  Beginn
**  08.02.1993  Funktion 'getstop' ergänzt
**  09.02.1993  Funktionen 'getpath' und 'dateiname' aus iris02 nach iris00
**  26.07.1993  Funktion 'stredit' benutzt
**  24.08.1993  Funktion 'mygetch' mit Bildschirmschoner eingebaut
**  02.09.1994  Funktion 'drawstr_background' eingebaut
*/

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "iris.h"
#include "common.h"
#include "isdefs.h"

/********************************************************************/

/*
**  gleiche Funktion wie getch() aus der Standartbibliothek
**  return : gedrückte Taste
*/
static int LOCAL dosgetch ( void )
  {
  union REGS regs;
  regs.h.ah = 0x07;
  intdos( &regs, &regs );
  return regs.h.al;
  }

/*
**  Ersetzt im Folgenden die Standartfunktion getch()
**
*/
int mygetch ( void )
  {
  while( ! kbhit() )
    {
    clock_t limit = clock() + (clock_t) 240 * CLK_TCK;
    while( ! kbhit() && clock() < limit ) {}
    if( ! kbhit() )
      {
      screensaver();
      clearkbd();
      }
    }
  return dosgetch();
  }

/*
**  Fehlermeldung ausgeben, nachdem 'malloc' gescheitert
*/
void errmalloc ( void )
  {
  puts( "Kein Hauptspeicher mehr frei (malloc gescheitert)." );
  }

/*
**  Löschen von Tastendrücken im Buffer
*/
void clearkbd ( void )
  {
  fflush( stdin );
  while( kbhit() ) dosgetch();
  }

/*
**  Fragt Benutzer nach Kenntnisnahme durch
**  Drücken der Return Taste
*/
void getreturn ( void )
  {
  fputs( "<ENTER>", stdout );
  clearkbd();
  while( getchar() != '\n' ) {}
  }

/*
**  Fragt Benutzer nach 'j' oder 'n' Bestätigung
**  return : Wurde 'j' getippt ?
*/
int janein ( void )
  {
  char buffer[20];
  char akku;
  char* ptr;
  int falsch;
  do{
    fputs( " (j/n) : ", stdout );
    fgets( buffer, sizeof buffer, stdin );
    for( ptr = buffer; *ptr == 0x20; ++ptr ) {}
    akku = tolower( *buffer );
    falsch = akku != 'j' && akku != 'n';
    if( falsch ) fputs( "nicht auswertbar, wiederholen", stdout );
    }
    while( falsch );
  return akku == 'j';
  }

/*
**  Eine Ganzezahl vom Benutzer abfragen
**  von    : kleinste erlaubte Zahl
**  bis    : größte erlaubte Zahl
**  return : die eingegebene Zahl
*/
int getzahl ( int von, int bis )
  {
  int zahl;
  char buffer[20];
  int falsch;
  do{
    printf( "%d..%d :", von, bis );
    fgets( buffer, sizeof buffer, stdin );
    zahl = atoi( buffer );
    falsch = zahl < von || zahl > bis;
    if( falsch ) puts( "Ungültige Zahl" );
    }
    while( falsch );
  return zahl;
  }

/*
**  Ein einzelnes Zeichen vom Benutzer abfragen ohne Groß/Klein-Unterscheidung
**  set    : String mit erlaubten Zeichen (in Kleinbuchstaben)
**  return : das eingegeben Zeichen
*/
char getcset( const char* set )
  {
  char buffer [20];
  int falsch;
  do{
    putchar( '>' );
    fgets( buffer, sizeof buffer, stdin );
    falsch = buffer[2] != '\0';
    if( falsch )
      puts( "Es muß genau ein Zeichen eingegeben werden" );
    else
      {
      buffer[0] = tolower( buffer[0] );
      falsch = strchr( set, buffer[0] ) == NULL;
      if( falsch )
        printf( "Das Zeichen '%c' ist nicht erlaubt,\n", buffer[0] );
      }
    }
    while( falsch );
  return buffer[0];
  }

/*
**  ähnlich wie 'getcset'
**  Benutzt aber getch zur direkten Tastaturabfrage und gibt keine
**  Fehlermeldung aus
**  set    : String mit erlaubten Zeichen (Kleinbuchstaben)
**  return : das eingebene Zeichen
*/
char askcset( const char* set )
  {
  char akku;
  do{
    akku = (char) getch();
    akku = tolower( akku );
    }
  while( strchr( set, akku ) == NULL );
  return akku;
  }

/*
**  Prüft, ob der 'string' ein verbotenes Zeichen aus 'set' enthält
**  string : String, der überprüft wird
**  set    : String mit verbotenen Zeichen (case sensitiv)
**  return : Ist der String ungültig ?
*/
int charinvalid ( const char* string, const char* set )
  {
  string = strpbrk( string, set );
  if( string != NULL )
    printf( "Ungültiges Zeichen im Namen '%c'\n", *string );
  return string != NULL;
  }

/*
**  gültigen Filename einlesen
**  fname : Buffer, Inhalt wird zum editieren benutzt
**  len   : Länge des Buffers ( = max. Länge des Filesnames + 1 )
*/
void getfname ( char* fname, int len )
  {
  TEXT invalid [] =  ":.<>/\\|";
  int i;
  do{
    fputs( "Name : ", stdout );
    stredit( fname, len );
    for( i = 0; fname[i] != '\0'; ++i ) fname[i] = toupper( fname[i] );
    }
    while( charinvalid( fname, invalid ) );
  }

/*
**  Liest Path (ggf. mit Drive) vom Benutzer ein
**  path : Buffer, Inhalt wird zum Editieren genutzt
**  len  : Länge des Buffers ( = max. Länge des Pfads + 1 )
*/
void getpath ( char* path, int len )
  {
  TEXT invalid [] =  ".<>|";
  do{
    int i;
    fputs( "Pathname : ", stdout );
    stredit( path, len );
    for( i = 0;  path[i] != '\0'; ++i ) path[i] = toupper( path[i] );
    if( i > 0 && path[i-1] != ':' && path[i-1] != '\\' )
      {
      path[i++] = i == 1 ? (char) ':' : (char) '\\';
      path[i] = '\0';
      }
    }
    while( charinvalid( path, invalid ) );
  }

/*
**  setzt Dateinamen aus 'path', 'name' und 'ext' zusammen
**  path   : Pfadnamne
**           und Buffer für den zusammengebauten Namen
**  name   : Filenamen ohne Pfad und ohne Extension
**  ext    : Extension
**  return : auf das letzte Zeichen des Dateinamens zurück
*/
char* dateiname ( char* path, const char* name, const char* ext )
  {
  while( *path != '\0' ) ++path;
  while( ( *path = *name++ ) != '\0' ) ++path;
  if( ext[0] != '.' ) *path++ = '.';
  while( ( *path = *ext++ ) != '\0' ) ++path;
  return path - 1;
  }

/*
**  Abfrage der Tastatur ob die Taste 's' oder 'S' gedrückt wurde
**  return : 0, wenn nicht gedrückt wurde
*/
int getstop ( void )
  {
  int akku = '\0';
  if( kbhit() ) akku = getch();
  return akku == 's' || akku == 'S';
  }

/********************************************************************/

/*
**  Soll der Hintergrund bei Textausgaben in das Bild 
**  geschwärzt werden ?
*/
static int drawstr_schwarz = 0;

/*
**  Einen Bildschirmbereich schwärzen
**  frb     : Framebuffer Nummer
**  zeile   : Startzeile in Pixel
**  spalte  : Startspalte in Pixel
**  zeilen  : Anzahl der Zeilen in Pixel
**  spalten : Anzahl der Spalten in Pixel
*/
static void LOCAL schwaerze
  ( int frb, int zeile, int spalte, int zeilen, int spalten )
  {
  char buffer [bpz];
  int z;
  for( z = zeile; z < zeile + zeilen; ++z )
    {
    isb_get_pixel( frb, z, wpz, buffer );
    memset( buffer + spalte, 0, spalten );
    isb_put_pixel( frb, z, wpz, buffer );
    }
  }

/*
**  String ausgeben über is_draw_text
**  frb      : Framebuffer Nummer
**  rbueding : soll String rechtbüding ausgeben werden ?
**  str      : der auszugebende String
*/
void is_draw_str ( int frb, int zeile, int rbuendig, const char* str )
  {
  const int len = strlen( str);
  int* tmp = malloc( len * sizeof (int) );
  if( tmp )
    {
    const int spalte = rbuendig ? 495 - len * 7 : 8;
    int i;
    if( drawstr_schwarz ) schwaerze( frb, zeile, spalte, 16, len * 8 );
    is_set_graphic_position( zeile, spalte );
    for( i = 0; i < len; ++i ) tmp[i] = (int) str[i];
    is_draw_text( frb, len, tmp );
    }
  else
    errmalloc();
  free( tmp );
  }

/*
**  Umschalten von 'drawstr_schwarz' und Ausgabe für Benutzer
*/
void drawstr_background ( void )
  {
  static const char *msg[2] =
    { "Hintergrund bei Textausgaben transparent",
      "Hintergrund bei Textausgaben schwarz" };
  drawstr_schwarz = ! drawstr_schwarz;
  putsxy( 20, 10, msg[ drawstr_schwarz ? 1 : 0 ] );
  setcrrspos( 20, 15 );  
  getreturn();
  }

