/*
**  C    ( Zortech C V2.1  /  Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .12.1992
**
**  Darstellen der Helligkeitsverteilung über den Spalt
**  Ausgeben der Grauwerte eines 16*16 Felds
**  Zoomen (2-fache Bildvergrößerung)
**
**  01.12.1992  Beginn
**  07.12.1992  Differenz zur Geraden hinzugefügt
**  31.08.1993  Funktion 'displaygrauwerte' hinzugefügt
**  16.09.1993  die Funktion 'displaygrauwerte' erweitert
**  20.09.1993  die Funktion 'displaygrauwerte' erweitert
**  22.07.1994  die Funktion 'erstellen' um möglichen Aufruf von
**              'spwahrewerte' erweitert
**  25.12.1994  linear Interpolieren (d.h. hier mitteln) beim Zoomen
**              der Grauwerte zwischen den Pixelen
*/

#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iris.h"
#include "common.h"
#include "isdefs.h"

/********************************************************************/
TEXT text [] =
  "Die Helligkeit über den Spalt wird über die Spalten links..rechts\n"
  "gemittelt. In der ausgegebenen Verteilung werden die 256 Zeilen\n"
  "durch einfache Mittelung als 512 Werte dargestellt.\n"
  "Steuerung des Spaltencursors: 4 - nach links  6 - nach rechts\n"
  "                  5 - fertig  f - schneller   s - langsamer\n";

/*
**  Spaltencursor
**  spalte : Startposition des Cursors
**  return : Endposition des Cursors
*/
static int LOCAL spaltencrrs ( int spalte )
  {
  char taste;
  int speed = 1;
  is_set_cursor_position( zpb/2, spalte );
  is_cursor( 1 );
  do{
    is_set_cursor_position( zpb/2, spalte );
    taste = askcset( "79456" );
    switch( taste )
      {
      case '4' : if( spalte - speed >= 0 ) spalte -= speed;
                 break;
      case '6' : if( spalte + speed < bpz ) spalte += speed;
                 break;
      case 'f' : if( speed < 500 ) speed *= 2;
                 break;
      case 's' : if( speed > 1 ) speed /= 2;
                 break;
      }
    }
    while( taste != '5' );
  is_cursor( 0 );
  return spalte;
  }

/*
**  Markiert eine Spalte in obere Bildhälfte
**  sichert überschriebenen Bereich im Buffer
**  frb    : Framebuffer Nummer
**  splate : Diese Bildspalte soll markiert werden
**  buffer : die Grauwerte der Bildspalte hier sichern
*/
static void LOCAL markiere ( int frb, int spalte, char* buffer )
  {
  char akku [bpz];
  char *const change = akku + spalte;
  int i;
  for( i = 0; i < zpb/2; ++i )
    {
    isb_get_pixel( frb, i, wpz, akku );
    buffer[i] = *change;
    *change = *change < 0 ? (char) 0x00 : (char) 0xff;
    isb_put_pixel( frb, i, wpz, akku );
    }
  }

/*
**  Markierungen wieder rückgängig machen ,Werte aus Buffer lesen
**  frb    : Framebuffer Nummer
**  spalte : Nummer der zuschreiben Bildspalte
**  buffer : Buffer mit den Grauwerten der Bildspalte
*/
static void LOCAL revmarkiere ( int frb, int spalte, const char* buffer )
  {
  char akku [bpz];
  char *const change = akku + spalte;
  int i;

  for( i = 0; i < zpb/2; ++i )
    {
    isb_get_pixel( frb, i, wpz, akku );
    *change = buffer[i];
    isb_put_pixel( frb, i, wpz, akku );
    }
  }

/*
**  Speichert gemittelte Helligkeitsverteilung in 'sp'
**  Felder 'von' und 'bis' in 'sp' müssen gesetzt sein
**  frb       : Framebuffer Nummer
**  sp        : die Datenstruktur Spektrum wird hier mißbraucht
**              für die Aufnahme der Helligkeitsverteilung
**  vonbuffer : die Grauwerte der ersten Bildspalte ist hier gespeichert
**  bisbuffer : die Grauwerte der letzten Bildspalte sind hier gespeichert
*/
static void verteilung ( int frb, spektrum* sp, char vonbuffer [bpz], char bisbuffer[bpz] )
  {
  const int von = sp->von + 1;
  const int bis = sp->bis;
  const int count = bis - von + 2;
  int* wert = sp->wert;
  int zeile;
  for( zeile = 0; zeile < zpb/2; ++zeile )
    {
    int i;
    char buffer [bpz];
    int akku = (uchar) vonbuffer [zeile] + (uchar) bisbuffer [zeile];

    isb_get_pixel( frb, zeile, wpz, buffer );
    for( i = von; i < bis; ++i ) akku += (uchar) buffer[i];

    *wert = akku / count;
    wert += 2;
    }
  wert = sp->wert;
  for( zeile = 0; zeile < zpb/2-1; ++zeile )
    {
    wert[1] = ( wert[0] + wert[2] ) / 2;
    wert += 2;
    }
  wert[1] = wert[0];
  }

/*
**  Felder 'von', 'bis' und 'next' in 'sp' setzen
**  sp  : Felder in dieser Datenstruktur werden gesetzt
**  von : erste Bildschirmspalte
**  bis : letzte Bildschirmspalte
**   (von,bis sind nicht sortiert, d.h. bis < von möglich)
*/
static void LOCAL prespektrum ( spektrum* sp, int von, int bis )
  {
  sp->next = NULL;
  if( bis > von )
    {
    sp->bis = bis;
    sp->von = von;
    }
  else
    {
    sp->bis = von;
    sp->von = bis;
    }
  }

/*
**  Löscht unteren Teil des Bildes
**  frb : Framebuffer Nummer
*/
static void LOCAL clearunten ( int frb )
  {
  char buffer [bpz];
  int i;
  memset( buffer, 0, sizeof buffer );
  for( i = zpb/2; i < zpb; ++i )
    isb_put_pixel( frb, i, wpz, buffer );
  }

/*
**  Einen Bereich Spaltenbereich ohne Markierungen aussuchen
**  pvon : dort die Startspalte speichern
**  pbis : dort die Endspalte speichern
**   (*pbis, *pvon sind sortiert, d.h. *pbis <= *pvon)
*/
static void LOCAL bereich ( int* pvon, int* pbis )
  {
  int von = bpz/2;
  int bis;
  von = spaltencrrs( von );
  bis = spaltencrrs( von );
  if( bis > von )
    {
    *pbis = bis;
    *pvon = von;
    }
  else
    {
    *pbis = von;
    *pvon = bis;
    }
  }

/*
**  Berechnet Regressionsgerade
**  wert     : Funktionswerte an die, die Gerade angepaßt werden soll
**              wert[i] = a * i + b
**  von      : erster Funktionswert wert[von],
**  bis      : letzter Funktionswert wert[bis] über die, die Gerade gelegt wird
**  steigung : dort die Steigung der Regrassionsgerade zurückgeben
**  abstand  : dort den Achsenabstand der Regressionsgeraden zurückgeben
*/
static void LOCAL regression( const int* wert, int von, int bis,
                                          double* steigung, double* abstand )
  {
  int n = bis - von + 1;
  long Sx = 0l;
  long Sy = 0l;
  long Sxy = 0l;
  long Sxx = 0l;
  double M,P;
  int i;
  for( i = von; i <= bis; ++i )
    {
    Sx  += (long) i;
    Sy  += (long) wert[i];
    Sxy += (long) i * (long) wert[i];
    Sxx += (long) i * (long) i;
    }
  M = (double) n * (double) Sxx - (double) Sx * (double) Sx;
  P = (double) n * (double) Sxy - (double) Sx * (double) Sy;
  *abstand = ( M * (double) Sy - P * (double) Sx ) / ( (double) n * M );
  *steigung = P / M;
  }

/*
**  Ermittelt Regressionsgerade in einem Bereich der Helligkeitsverteilung
**      grauwert[i] = steigung * i + abstand
**  sp       : Die Datenstruktur enthält die Helligkeitsverteilung
**  steigung : dort die Steigung der Gerade zurückgeben
**  abstand  : dort den Achsenabstand der Gerade zurückgeben
*/
static void LOCAL bereichsregression ( const spektrum* sp,
                                       double* steigung, double* abstand )
  {
  int von,bis;
  do{
    bereich( &von, &bis );
    printf( "Bereich %d ... %d, ok", von, bis );
    }
    while( ! janein() );
  regression( sp->wert, von, bis, steigung, abstand );
  }

/*
**  Spezielle Ausgabe für die Differenzwerte
**  frb : Framebuffernummer
**  sp  : die Datenstruktur enthält die Helligkeitsverteilung
*/
static void LOCAL diffsout ( int frb, const spektrum* sp )
  {
  char buffer [bpz];
  int zeile;
  const int *const wert = sp->wert;
  buffer[0] = hell;
  for( zeile = 0; zeile < zpb/2; ++zeile )
    {
    int i;
    for( i = 1; i < bpz; ++i )
      if( wert[i] < 0 )
        buffer[i] = zeile < 127 || wert[i] > 128 - zeile
                    ? (char) dunkel : (char) hell;
      else
        buffer[i] = zeile > 128 || wert[i] < 128 - zeile
                    ? (char) dunkel : (char) hell;
    isb_put_pixel( frb, zeile + zpb/2, wpz, buffer );
    }
  sprintf( buffer, "Q-R %d - %d", sp->von, sp->bis );
  is_draw_str( frb, zpb/2, 1, buffer );
  }

/*
**  Mittlere Regressionsgerade von den Werten subtrahieren und darstellen
**  Bereich über den Gerade gebildet wird im Benutzerdialog festlegen
**  frb : Framebuffer Nummer
**  sp  : Datenstruktur mit den Helligkeitswerten
*/
static void LOCAL subgerade ( int frb, spektrum* sp )
  {
  double steigung, abstand;
  int i;
  fputs( "Regressionsgerade über alle Punkte", stdout );
  if( janein() )
   regression( sp->wert, 0, bpz, &steigung, &abstand );
  else
    {
    double zone1steigung, zone1abstand;
    double zone2steigung, zone2abstand;
    puts( "Bereich für 1. Regressionsgerade festlegen" );
    bereichsregression( sp, &zone1steigung, &zone1abstand );
    puts( "Bereich für 2. Regressionsgerade festlegen" );
    bereichsregression( sp, &zone2steigung, &zone2abstand );
    steigung = ( zone1steigung + zone2steigung ) / 2.0;
    abstand = ( zone1abstand + zone2abstand ) / 2.0;
    }
  for( i = 0; i < bpz; ++i )
    sp->wert[i] -= (int) ( abstand + steigung * (double) i );
  diffsout( frb, sp );
  }

/*
**  Erstellen und Anzeigen der Helligkeitsverteilung
**  frb : Framebuffer Nummer
*/
static void LOCAL erstellen ( int frb )
  {
  int ok;
  int von = bpz/2;
  int bis;
  char vonbuffer [zpb/2];
  char bisbuffer [zpb/2];
  spektrum sp;
  puts( text );
  clearunten( frb );
  do{
    puts( "Linke Grenze" );
    von = spaltencrrs( von );
    markiere( frb, von, vonbuffer );
    puts( "Rechte Grenze" );
    bis = spaltencrrs( von );
    markiere( frb, bis, bisbuffer );
    printf( "Ïber Spalten %d .. %d mitteln", von, bis );
    if( janein() )
      {
      if( bis == von ) memcpy( bisbuffer, vonbuffer, sizeof bisbuffer );
      prespektrum( &sp, von, bis );
      verteilung( frb, &sp, vonbuffer, bisbuffer );
      fputs( "Grauwerte in \"wahre Werte\" umrechnen", stdout );
      if( janein() )
        spwahrewerte( zpb/2, &sp, frb );
      else
        spektrumout( frb, zpb/2, &sp );
      is_draw_str( frb, zpb/2, 0, "QUER" );
      fputs( "Differenz zur mittleren Regressionsgerade darstellen", stdout );
      if( janein() ) subgerade ( frb, &sp );
      fputs( "Verteilung ok, weiter", stdout );
      ok = janein();
      }
    revmarkiere( frb, bis, bisbuffer );
    revmarkiere( frb, von, vonbuffer );
    }
    while( ! ok );
  }

/*
**  Helligkeitsverteilung über den Spalte darstellen
**  frb : Framebuffer Nummer
*/
void helligkeit ( int frb )
  {
  char antwort;
  puts( "Es wird nur ein Halbbild ausgewertet, daher:\n"
        " o - Halbbild befindet sich in obere Bildhälfte\n"
        " h - vom Gesamtbild ein Halbbild in obere Hälfte bringen\n"
        " z - zurück"  );
  antwort = getcset( "ohz" );
  if( antwort == 'h' ) press( frb );
  if( antwort != 'z' ) erstellen( frb );
  }

/********************************************************************/

/*
**  Bereich aus dem Grauwerte dargestellt werden +RANGE..0..-RANGE
*/
#define RANGE 8

/*
**  Schrittweite um die der Cursor bewegt wird
*/
#define STEP 3

/*
**  Wert als Dezimalzahl auf Bildschirm ausgeben
**  x,y  : Position auf Bildschirm, (0,0) ist die Mitte
**  wert : diese Zahl ausgeben (0..999)
*/
static void LOCAL displaywert ( int x, int y, int wert )
  {
  char buffer [4];
  int i = 3;
  buffer[i] = 0;
  do{
    div_t x;
    x = div( wert, 10 );
    buffer[--i] = (char) x.rem + '0';
    wert = x.quot;
    }
    while( i > 0 && wert > 0 );
  while( i > 0 ) buffer[--i] = ' ';
  putsxy( ( x * 4 ) + 40, y + 12, buffer );
  }

/*
**  Ausgabe der Cursorposition, in Zahl und als Fadenkreuz
**  x,y : die Cursorposition
*/
static void LOCAL ausgabecrrs ( int x, int y )
  {
  is_set_cursor_position( y, x );
  displaywert( -RANGE - 1, 0, y );
  displaywert( 0, -RANGE - 1, x );
  }

/*
**
*/
static int LOCAL fragewert ( const char* frage, int wert, int min, int max )
  {
  char buffer [7];
  int result;
  int fragelen = strlen( frage );
  putsxy( 5, 23, frage );
  itoa( wert, buffer, 10 );
  streditxy( buffer, sizeof buffer, 6 + fragelen, 23 );
  result = atoi( buffer );
  if( result > max ) result = max;
  if( result < min ) result = min;
  return result;
  }

/*
**  Gewünschte Cursorposition vom Benutzer holen
**  x,y : dort die Position zurückgeben
*/
static void LOCAL direkteingabe ( int* x, int* y )
  {
  cll( 23 );
  *y = fragewert( "Cursor Zeile  :", *y, RANGE, zpb - RANGE - 1 );
  *x = fragewert( "Cursor Spalte :", *x, RANGE, bpz - RANGE - 1 );
  cll( 23 );
  }

/*
**  Grauwerte in File schreiben
**  frb    : frame buffer number
**  out    : Ausagbefile, write text
**  mittex : Grauwerte um diese Spalte ausgeben
**  mittey : Grauwerte um diese Zeile ausgeben
**  return : ist ein Fehler aufgetreten ?
*/
static int LOCAL druckengrau ( int frb, FILE* out, int mittex, int mittey )
  {
  const int lenx = 19;
  const int leny = 57;
  int startx,starty;
  int stopx, stopy;
  int y;
  int error = 0;
  startx = mittex - lenx/2;
  if( startx < 0 ) startx = 0;
  if( startx + lenx >= bpz ) startx = bpz - lenx - 1;
  starty = mittey - leny/2;
  if( starty < 0 ) starty = 0;
  if( starty + leny >= zpb ) starty = zpb - leny - 1;
  stopx = startx + lenx;
  stopy = starty + leny;
  error = fprintf( out, "    Spalten von %d bis %d, Zeilen von %d bis %d\n\n",
                                   startx, stopx,        starty, stopy ) == EOF;
  for( y = starty; y <= stopy && ! error; ++y )
    {
    uchar buffer [bpz];
    int x;
    isb_get_pixel( frb, y, wpz, (char*) buffer );
    error = fputs( "  ", out ) == EOF;
    for( x = startx; x < stopx && ! error; ++x )
      error = fprintf( out, "%4d", buffer[x] ) == EOF;
    error = error || fputc( '\n', out ) == EOF;
    }
  error = error || fputc( '\f', out ) == EOF;
  return error;
  }

/*
**  Eine vom Benutzer gewählte Überschrift in das File schreiben
**  out    : Ausgabefile, write text
**  return : ist ein Fehler aufgtreten ?
*/
static int LOCAL druckenueberschrift ( FILE* out )
  {
  static const char prompt [] = "Überschrift: ";
  int error = 0;
  char buffer[60];
  cll( 23 );
  putsxy( 1,23, prompt );
  buffer[0] = '\0';
  streditxy( buffer, sizeof buffer, 1+strlen(prompt), 23 );
  cll( 23 );
  if( buffer[0] != '\0' )
    error = fprintf( out, "    %s\n", buffer ) == EOF;
  return error;
  }

/*
**  Fragt nach dem Namen des Ausagbefiles und öffnet es
**  return : das Ausgabefile, write text
*/
static FILE* LOCAL fragefilenamen ( void )
  {
  static const char prompt [] = "Ausgabefile: ";
  char fname [ 60 ];
  FILE* outfile = NULL;
  cll( 23 );
  putsxy( 1,23, prompt );
  fname[0] = '\0';
  streditxy( fname, sizeof fname, 1 + strlen( prompt ), 23 );
  cll( 23 );
  if( fname[0] != '\0' )
    {
    outfile = fopen( fname, "a" );
    if( outfile == NULL ) putsxy( 5, 23, "Kann File nicht öffnen/anlegen" );
    }
  return outfile;
  }

/*
**  Grauwerte auf Drucker/File ausgeben
**  frb : frame buffer number
**  x,y : um diese Position herum die Werte ausgeben
*/
static void LOCAL drucken ( int frb, int x, int y )
  {
  FILE* ausgabefile = fragefilenamen();
  if( ausgabefile != NULL )
    {
    if( druckenueberschrift( ausgabefile )
        || druckengrau( frb, ausgabefile, x, y ) )
      putsxy( 5,23, "Fehler bei der Ausgabe aufgetreten." );
    else
      putsxy( 10,23, "Ausgabe fertig." );
    fclose( ausgabefile );
    }
  }

/*
**  2-fache Vergrößerung einer Bildzeile
**  zeile  : diese Bildzeile vergrößern
**  buffer : Speicher für die vergrößerte Zeile
**  x      : diese Spalte soll Mitte der vergr╠¯erten Zeile sein
*/
static void LOCAL zoomzeile ( const uchar zeile [bpz], uchar buffer [bpz], int x )
  {
  int i;
  x -= bpz/4;
  for( i = 0; i < bpz; i += 2, ++x )
    {
    buffer[i] = ( uchar) ( ( (int) zeile[x-1] + (int) zeile[x] ) >> 1 );
    buffer[i+1] = zeile[x];
    }
  }

/*
**  Mitteln zweier Bildzeilen
**  ina : erste Eingabe Bildzeile
**  inb : zweite Eingabe Bildzeile
**  out : die gemittelte Bildzeile
*/
static void LOCAL mittelzeile ( const uchar ina[bpz], const uchar inb[bpz], uchar out[bpz] )
  {
  int i;
  for( i = 0; i < bpz; ++i )
    out[i] = (uchar) ( ( (int) ina[i] + (int) inb[i] ) >> 1 );
  }

/*
**  Zoomen, 2-fache Bildvergrößerung
**  frb : frame buffer number
**  x,y : Dieser Punkt soll Mittelpunkt des vergrößerten Bildes sein
**        der Punkt muß so gewählt sein, daß der Zoombereich im Bild liegt
*/
static void LOCAL zoombild ( int frb, int x, int y )
  {
  uchar* buffer = malloc( bpz * 3 );
  if( buffer != NULL )
    {
    uchar* bufferA = buffer;
    uchar* bufferB = bufferA + bpz;
    uchar* bufferC = bufferB + bpz;
    uchar* tmp;
    int oben = y - zpb/4;
    int unten = y + zpb/4 -1;
    int dest = 0;
    isb_get_pixel( frb, oben-1, wpz, (char*) bufferA );
    zoomzeile( bufferA, bufferC, x );
    while( oben <= unten && dest < oben )
      {
      isb_get_pixel( frb, oben++, wpz, (char*) bufferA );
      zoomzeile( bufferA, bufferB, x );
      mittelzeile( bufferB, bufferC, bufferA );
      isb_put_pixel( frb, dest++, wpz, (char*) bufferA );
      isb_put_pixel( frb, dest++, wpz, (char*) bufferB );
      tmp = bufferB;
      bufferB = bufferC;
      bufferC = tmp;
      }
    dest = zpb - 1;
    isb_get_pixel( frb, unten+1, wpz, (char*) bufferA );
    zoomzeile( bufferA, bufferC, x );
    while( unten >= oben && dest > unten )
      {
      isb_get_pixel( frb, unten--, wpz, (char*) bufferA );
      zoomzeile( bufferA, bufferB, x );
      mittelzeile( bufferB, bufferC, bufferA );
      isb_put_pixel( frb, dest--, wpz, (char*) bufferA );
      isb_put_pixel( frb, dest--, wpz, (char*) bufferB );
      tmp = bufferB;
      bufferB = bufferC;
      bufferC = tmp;
      }
    }
  else
    errmalloc();
  free( buffer );
  }

/*
**  Benutzerdialog zum Zoomen (2-fache Bildvergrößerung)
**  frb : frame buffer number
**  x,y : Position des Cursors
*/
static void LOCAL zoom ( int frb, int x, int y )
  {
  static const char frage [] = "Zoomen, Bild wird dabei überschrieben (j/n):";
  char ask [2];
  if( x < bpz/4 +1 ) x = bpz/4 +1;
  if( x > bpz - bpz/4 ) x = bpz - bpz/4;
  if( y < zpb/4 +1 ) y = zpb/4 +1;
  if( y > zpb - zpb/4 ) y = zpb - zpb/4;
  cll( 23 );
  putsxy( 5,23, frage );
  ask[0] = 'j'; ask[1] = '\0';
  streditxy( ask, sizeof ask, 5 + strlen(frage), 23 );
  cll( 23 );
  if( tolower(ask[0]) == 'j' ) zoombild( frb, x, y );
  }

/*
**  Verschiebung des Cursors nach Benutzereingabe
**  frb    : frame buffer number
**  x,y    : Position des Cursors, wird aktualisiert
**  return : soll Anzeige weiter erfolgen ?
*/
static int LOCAL movexy ( int frb, int* x, int* y )
  {
  int key = getch();
  int xalt = *x;
  int yalt = *y;
  switch( key )
    {
    case '8' : *y = yalt > RANGE + STEP ? yalt - STEP : RANGE;
               break;
    case '2' : *y = yalt < zpb - RANGE - STEP ? yalt + STEP : zpb - RANGE - 1;
               break;
    case '4' : *x = xalt > RANGE + STEP ? xalt - STEP : RANGE;
               break;
    case '6' : *x = xalt < bpz - RANGE - STEP ? xalt + STEP : bpz - RANGE - 1;
               break;
    case 'p' : direkteingabe( x, y );
               break;
    case 'd' : drucken( frb, *x, *y );
               break;
    case 'z' : zoom( frb, *x, *y );
               break;
    case '\x1B' : key = '5';
    }
  if( xalt != *y || yalt != *x ) ausgabecrrs( *x, *y );
  return key != '5';
  }

/*
**  Ausgabe der Texte am Rand des Zahlenfelds auf dem Bildschirm
*/
static void LOCAL ausgaberahmen ( void )
  {
  putsxy( 32, 3, "Spalte:" );
  putsxy( 0, 11, "Zeile:" );
  putsxy( 15,22, "CRRS 8246 / Ende 5 / position p / drucken d / zoom z" );
  }

/*
**  Grauwerte eines 16 * 16 Feldes in Dezimalzahlen ausgeben
**  Das Feld kann vom Benutzer verschoben werden
**  frb : Framebuffer Nummer
*/
void displaygrauwerte ( int frb )
  {
  static int x = bpz/2;
  static int y = zpb/2;
  int xi = -RANGE;
  int yi = -RANGE;
  int buffert = -1;
  char buffer [bpz];
  ausgaberahmen();
  ausgabecrrs( x,y );
  is_cursor( 1 );
  while( ! kbhit() || movexy( frb, &x, &y ) )
    {
    int zeile = y + yi;
    if( buffert != zeile ) isb_get_pixel( frb, buffert = zeile, wpz, buffer );
    displaywert( xi, yi, (uchar) buffer[ x + xi ] );
    if( xi < RANGE )
      ++xi;
    else
      {
      xi = -RANGE;
      yi = yi < RANGE ? yi + 1 : -RANGE;
      }
    }
  is_cursor( 0 );
  }
