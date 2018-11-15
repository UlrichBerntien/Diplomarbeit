/*
**  C    ( Zortech C V2.1 / Micrsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .10.1992
**
**  Auswerten der Spektren
**
**  08.10.1992  Beginn
**  28.10.1992  Interpolation Temp(Verhältnis) statt Temp(log10(Verhältnis))
**  03.11.1992  Parameter 'int frb' in spauswertung() hinzugekommen
**  05.11.1992  'fixtemperatur' ergänzt
**  23.11.1992  Variable 'speed' in Funktion lage() ergänzt
**  01.12.1992  Funktion ausgeben() geändert
**  08.03.1993  Grafik der Verteilungen n(x) und T(x) aufgenommen
**  09.06.1993  Liniengrenzen können auch manuel bestimmt werden
**  13.06.1993  Modul iris04a.c abgespalten
**  07.07.1994  Nach dem Auswerten des Spektrums wird es in "wahre Werte"
**              umgerechnet und nocheinmal angezeigt.
**  29.07.1994  Funktion wwtabelle() ergänzt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iris.h"
#include "iris04.h"
#include "common.h"
#include "isdefs.h"

/*
**  Benutzer gibt Höhe des Kontinuums an und Lage der Linie
**  zeile     : die erste Zeiel des Spektrums auf dem Bildschirm
**  kontinuum : Eingabe : ca. Lage (Grauwert) des Kontinuums
**              Ausgabe : genaue Lage des Kntinuums
**  linie     : Eingabe : ca. Lage (Bildspalte) der Linie
**              Ausgabe : genaue Lage der Linie
*/
static void LOCAL lage ( int zeile, int* kontinuum, int* linie )
  {
  int x = *linie;
  int y = *kontinuum;
  char wahl;
  int speed = 1;
  int ende = 0;
  puts( "Cursor bewegen mit Tasten 8,4,2,6,(s) wenn fertig : Taste 5.\n"
        "Cursor auf Zeile  in der Höhe des Kontinuums,\n"
        "Cursor auf Spalte innerhalb der Linie stellen" );
  is_set_cursor_position( zeile + zpb/2 - 1 - y, x );
  is_cursor( 1 );
  while( ! ende )
    {
    is_set_cursor_position( zeile + zpb/2 - 1 - y, x );
    wahl = askcset( "84625s" );
    switch( wahl )
      {
      case 's' : speed = speed == 1 ? 4 : 1;
                 break;
      case '8' : if( y + speed < 256 ) y += speed;
                 break;
      case '2' : if( y - speed >= 0 ) y -= speed;
                 break;
      case '4' : if( x - speed >= 0 ) x -= speed;;
                 break;
      case '6' : if( x + speed < bpz ) x += speed;
                 break;
      case '5' : printf( "Kontinuum %d, Linie %d, ok", y, x );
                 ende = janein();
      }
    }
    is_cursor( 0 );
  *linie = x;
  *kontinuum = y;
  }

/*
**  Ca. Lage des Kontinuums und der Linie ermitteln
**  wert      : Die Grauwerte des Spektrums
**  kontinuum : dort die ca. Lage (Grauwert) des Kontinuums speichern
**  linie     : dort die ca. Lage (Bildspalte) der Linie speichern
*/
static void LOCAL calage ( const int wert [bpz], int* kontinuum, int* linie )
  {
  int pos, i;
  int akku = 0;
  for( i = 0; i < 10; ++i )
    akku += wert[i] + wert[bpz-1-i];
  *kontinuum = akku / 20;
  pos = 20;
  akku = wert[pos];
  for( i = pos; i < bpz - 20; ++i )
    if( wert[i] > akku )
      akku = wert[pos = i];
  *linie = pos;
  }

/*
**  einen neuen Infoblock anlegen, ggf. Fehlermeldung ausgeben
**  sp     : Zu diesem Spektrum den Infoblock anlegen
**  name   : Name des Spektrums
**  return : Zeiger auf neuangelegeten Infoblock oder NULL im Fehlerfall
*/
static infoblock* LOCAL newinfoblock ( const spektrum* sp, const char* name )
  {
  infoblock* info = malloc( sizeof (infoblock) );
  if( info == NULL )
    errmalloc();
  else
    {
    info->sp = sp;
    info->name = name;
    }
  return info;
  }

/*
**  Eine Bildschirmspalte aussuchen
**  von,bis : erlaubter Wertebreich
**  pos     : Startspalte
**  return  : Endspalte
*/
static int LOCAL erfragespalte ( int von, int bis, int pos )
  {
  const int zeile = 511;
  int speed = 1;
  char ask;
  if( pos < von || pos > bis ) pos = ( von + bis ) / 2;
  is_set_cursor_position( zeile, pos );
  is_cursor( 1 );
  do{
    ask = askcset( "456fs" );
    switch( ask )
      {
      case '4' : if( pos - speed >= von ) pos -= speed;
                 break;
      case '6' : if( pos + speed <= bis ) pos += speed;
                 break;
      case 'f' : if( speed < 500 ) speed *= 2;
                 break;
      case 's' : if( speed > 1 ) speed /= 2;
      }
    is_set_cursor_position( zeile, pos );
    }
    while( ask != '5' );
  is_cursor( 0 );
  return pos;
  }

/*
**  Grenzen der Linie im Dialog mit dem Benutzer ermitteln
**  infoblock : in dieser Struktur die Liniengrenzen eintragen,
**              vorher sind schon ca. Werte eingetragen
*/
void erfragegrenzen ( infoblock* info )
  {
  puts( "Fußbreite der Linie festlegen\n"
        "Bewegen: 4,6  Geschwindigkeit: f,s  Ende: 5" );
  do{
    puts( "Linke Grenze der Linie." );
    info->linievon = erfragespalte( 0, info->linie, info->linievon );
    puts( "Rechte Grenze der Linie." );
    info->liniebis = erfragespalte( info->linie, 511, info->liniebis );
    fputs( "Festlegung der Grenzen ok", stdout );
    }
    while( ! janein() );
  }

/*
**  Im Benutzerdialog genaue Lage des Linienmax. bestimmen
**  zeile : Bildschirmzeile, ab der das Spektrum dargestellt ist
**  info  : dort die genaue Lage (Grauwert) des Maximums eintragen,
**          vorher muß schon ein ca. Wert angegeben sein
*/
static void LOCAL sucheliniemax ( int zeile, infoblock* info )
  {
  int wert = info->spitze;
  int weiter = 1;
  puts( "Cursor auf die Zeile mit der Spitze der Linie einstellen.\n"
        "Steuerung 8 - höher,  2 - tiefer,  5 - fertig" );
  is_set_cursor_position( zeile + zpb/2 - 1 - wert, 0 );
  is_cursor( 1 );
  while( weiter )
    {
    switch( askcset( "852" ) )
      {
      case '8' : if( wert < 256 ) ++wert;
                 break;
      case '2' : if( wert > 0 ) --wert;
                 break;
      case '5' : printf( "Spitze der Linie bei %d Pixelwert, ok", wert );
                 weiter = ! janein();
      }
    is_set_cursor_position( zeile + zpb/2 - 1 - wert, 0 );
    }
  is_cursor( 0 );
  info->spitze = wert;
  }

/*
**  Art der Linie feststellen, dabei Annahme :
**      gleicher Name => gleiche Linien-Art
**  name   : Name des Spektrums
**  return : Buchstabenkürzel des Linientyps (a,b,c,k)
*/
static char LOCAL getart ( const char* name )
  {
  static char* altname = NULL;
  static char art;
  int neuelinie;
  if( altname != NULL )
    {
    neuelinie = strcmp( name, altname ) != 0;
    if( neuelinie ) free( altname );
    }
  else
    neuelinie = 1;
  if( neuelinie )
    {
    altname = strdup( name );
    puts( "Welche Linie ? a - Alpha, b - Beta, c - Gamma, k - keine davon" );
    art = getcset( "abck" );
    }
  return art;
  }

/*
**  Anfang und Ende der Linie auf Bildschirm markieren
**  frb   : Framebuffer Nummer
**  zeile : ab dieser Bildschirmzeile ist das Spektrum dargestellt
**  l1    : das ist die Anfangsspalte der Linie
**  l2    : das ist die Endspalte der Linie
*/
static void LOCAL markieren( int frb, int zeile, int l1, int l2 )
  {
  char buffer [bpz];
  const int lastzeile = zeile + zpb/2;
  while( ++zeile < lastzeile )
    {
    isb_get_pixel( frb, zeile, wpz, buffer );
    buffer[l2] = buffer[l1] = 0xff;
    isb_put_pixel( frb, zeile, wpz, buffer );
    }
  }

/*
**  Alle Informationen aus dem Spektrum erfassen
**  zeile : ab dieser Bildschirmzeile ist das Spektrum dargestellt
**  info  : dort werden die gesammelten Informationen gespeichert
**          vorher muß dort bereits das Spektrum angegeben sein
**  frb   : Framebuffer Nummer
*/
static void LOCAL erfassen ( int zeile, infoblock* info, int frb )
  {
  info->art = getart( info->name );
  calage( info->sp->wert, &info->kontinuum, &info->linie );
  lage( zeile, &info->kontinuum, &info->linie );
  liniesummieren( info );
  markieren( frb, zeile, info->linievon, info->liniebis );
  temperatur( info );
  info->spitze = info->sp->wert[ info->linie ];
  sucheliniemax( zeile, info );
  halbwertsbreite( info );
  if( info->art != 'k' ) dichte( info );
  }

/*
**  Erstellen der Ausgabe der fertigen Auswertung
**  buffer : in diesen Buffer den Ausgabetext schreiben
**  info   : die Informationen hieraus werden ausgegeben
*/
static void LOCAL erstellen ( char* buffer, const infoblock* info )
  {
  const char* art;
  buffer += sprintf( buffer,
    "\n"
    " --- Auswertung von Spektrum @F'%s'@N ---\n"
    " Grauwerte der Spektren gemittelt über Zeilen %d - %d\n"
    " Kontinuumsintensität %d Grauwert = %11.3le wahrer Wert\n"
    " Linie um Spalte %d von Spalte %d bis %d\n"
    "       integriert %11.3le wahrer Wert, ohne Kontinuum %11.3le\n"
    " Verhältnis @FLinie/Kontinuum = %11.3le@N\n"
    , info->name
    , info->sp->von, info->sp->bis
    , info->kontinuum, info->wkontinuum
    , info->linie, info->linievon, info->liniebis
    , info->liniealles, info->linieintegral
    , info->verhaeltnis );
  if( info->art != 'k' )
    {
    switch( info->art )
      {
      case 'a' : art = "Alpha";
                 break;
      case 'b' : art = "Beta";
                 break;
      case 'c' : art = "Gamma";
      }
    buffer += sprintf( buffer,
      " Bei der @F%s@N - Linie entspricht das der Temperatur\n"
      "      %11.3le K  =  @F%11.3le eV@N\n"
      , art
      , info->temperaturk, info->temperaturev );
    }
  buffer += sprintf( buffer,
    " Spitze der Linie bei %d Grauwert = %11.3le wahrer Wert (ohne K.)\n"
    "     die Hälfte ist %11.3le wahrer Wert = %d Grauwert (mit K.)\n"
    "     dieser Wert wird in Spalte %d und %d erreicht\n"
    "     das ergibt eine @FFWHM von %11.3le nm@N = %11.3le \x8F\n"
    , info->spitze, info->wspitze
    , info->whalbespitze, info->halbespitze
    , info->halbvon, info->halbbis
    , info->fwhm, info->fwhmA );
  if( info->art != 'k' )
    {
    buffer += sprintf( buffer,
      " Bei der %s - Linie entspricht das der Dichte\n"
      "      @F%11.3le cm^-3@N         ( C(Ne,Te) = %11.3le )\n"
      , art
      , info->ne, info->cnt );
    if( fixtemperatur > 0.0 )
      buffer += sprintf( buffer,
        "     dabei wurde Te = %11.3le K angenommen\n"
        , fixtemperatur );
    }
  }

/*
**  Ausgabe eines Strings, weibei im String die Symbole "@F" in den
**  den Fettdruck-Sterucode und "@N" in den Normaldruck-Steuercode
**  umgesetzt werden.
**  file   : File (write) dorthin die Ausgabe schreiben
**  str    : der auszugebende String
**  fett   : Steuercode für Umschalten zu Fettdruck
**  normal : Steuercode für Umschalten zu Normaldruck
*/
static void LOCAL printfile ( FILE* file, char* str, const char* fett, const char* normal )
  {
  char* now = str;
  while( *now != '\0' )
    if( now[0] == '@' && ( now[1] == 'N' || now[1] == 'F' ) )
      {
      if( now > str )
        {
        *now = '\0';
        fputs( str, file );
        *now = '@';
        }
      if( now[1] == 'F' )
        {
        if( fett != NULL ) fputs( fett, file );
        }
      else
        if( normal != NULL ) fputs( normal, file );
      now += 2;
      str = now;
      }
    else
      ++now;
  if( now > str ) fputs( str, file );
  }

/*
**  Ausgeben der fertigen Auswertung
**  info   : hier sind die Informationen aus der Auswertung gespeichert
**  return : Auswertung ok ?
*/
static int LOCAL ausgeben ( const infoblock* info )
  {
  char* buffer = malloc( 2048 );
  int ok;
  if( buffer != NULL )
    {
    FILE* outfile;
    erstellen( buffer, info );
    printfile( stdout, buffer, NULL, NULL );
    fputs( "Auswertung ok", stdout );
    ok = janein();
    if ( ok ||
            ( fputs( "Auswertung ins Logfile schreiben", stdout ), janein() ) )
      {
      if( ( outfile = fopen( logfile, "at" ) ) != NULL )
        {
        puts( "... schreibe ins Logfile" );
        printfile( outfile, buffer, printerfett, printernormal );
        fclose( outfile );
        }
     else
       printf( "Logfile '%s' kann nicht geöffnet werden.\n", logfile );
     }
    free( buffer );
    }
  else
    errmalloc();
  return ok;
  }

/*
**  ggf. Initialisierung durchführen
**  return : alles ok ?
*/
static int checkinit ( void )
  {
  if( ! initok )
    {
    doinit();
    initok = initok && init_pixel2wahr();
    }
  if( ! initok )
    {
    puts( "Modul 'iris04.c' kann nicht arbeiten." );
    getreturn();
    }
  return initok;
  }

/*
**  Umrechnen der Grauwerte eines Spektrums in "wahre Werte" und ausgeben
**  zeile : ab dieser Bildschirmzeile ist das Spektrum darzustellen
**  sp    : Die Werte des Spektrums, sie werden in wahre Werte umgerechnen
**  frb   : Framebuffer Nummer
*/
void spwahrewerte ( int zeile, spektrum* sp, int frb )
  {
  puts( "Das Spektrum wird nun von Grauwerten in wahre Werte\n"
        "(\"inv. Logarithmus\") umgerechnet und angezeigt." );
  spektrum2wahrwerte( sp->wert );
  spektrumout( frb, zeile, sp );
  is_draw_str( frb, zeile+20, 1, "WW" );
  }

/*
**  Auswerten eines Spektrum
**  zeile : ab dieser Bildschirmzeile ist das Spektrum dargestellt
**  sp    : Die Werte des Spektrums (Achtung, sie werden verändert)
**  frb   : Framebuffer Nummer
**  name  : Name des Spektrums
*/
void spauswertung ( int zeile, spektrum* sp, int frb, const char* name )
  {
  if( checkinit() )
    {
    int ok;
    infoblock* info = newinfoblock( sp, name );
    do{
      erfassen( zeile, info, frb );
      ok = ausgeben( info );
      if( ok )
        spwahrewerte( zeile, sp, frb );
      else
        spektrumout( frb, zeile, sp );
      }
    while( ! ok );
    free( info );
    }
  }

/*******************************************************************/

/*
**  Sollen die wahren Werte tabelliert werden ?
**  0 - keine Tabelle
**  1 - Tabelle mit Index/rel.Wellenlänge/Grauwert/wahrer Wert
**  2 - Liste nur mit wahrem Wert
*/
int wwtabellieren = 0;

/*
**  Benutzer nach der Art der der Tabelle der Wahren Werte fragen
*/
void wwtabelle ( void )
  {
  if( checkinit() )
    {
    puts( "Wie sollen die wahren Werte tabelliert werden ?\n"
        "0 - keine Ausgabe der wahren Werte ins Logfile\n"
        "1 - Tabelle mit den wahren Werte ins Logfile\n"
        "2 - Liste mit wahren Werten ins Logfile\n"
        "    (z.B. für verarbeitung durch andere Programme)" );
    wwtabellieren = getcset( "012" ) - '0';
    }
  }

/** IRIS - N - T ***************************************************/

#include "irisnt.h"

/*
**  Automatisches Auswerten eines Spektrums und Eintragen in die
**  Verteilungs-Tabelle
**  sp    : Das Spektrum wird ausgewertet
**  index : Index in den Tabellen n und t
**  data  : die Verteilungs-Daten
*/
static void LOCAL nt_auswertung ( const spektrum* sp, int index, ntspeicher* data )
  {
  infoblock* info = newinfoblock( sp, data->name );
  info->art = data->art;
  calage( info->sp->wert, &info->kontinuum, &info->linie );
  liniesummieren( info );
  temperatur( info );
  info->spitze = info->sp->wert[ info->linie ];
  halbwertsbreite( info );
  dichte( info );
  data->t[index] = info->temperaturev;
  data->n[index] = info->ne;
  printf( "%3d - %3d : T = %11.3le eV, n = %11.3le cm^-3\n"
          ,sp->von, sp->bis, info->temperaturev, info->ne );
  free( info );
  }

/*
**  Verteilungen erfassen
**  data : dort müssen noch n[] und t[] ausgefüllt werden
**  frb  : Frame Buffer number
*/
static void LOCAL nt_aufnahme ( int frb, ntspeicher* data )
  {
  int i,j;
  int zeile = data->von;
  int index = 0;
  const int diff = data->mittelueber * 2 - 2;
  uchar buffer [bpz];
  spektrum* sp = malloc( sizeof (spektrum) );
  int* spwert = malloc( bpz * sizeof (int) );
  if( spwert == NULL || sp == NULL )
    errmalloc();
  else
    {
    for( j = 0; j < bpz; ++j ) spwert[j] = 0;
    for( i = 0; i <= diff; i += 2 )
      {
      isb_get_pixel( frb, zeile, wpz, (char*) buffer );
      for( j = 0; j < zpb; ++j ) spwert[j] += buffer[j];
      }
    sp->von = data->von;
    sp->bis = data->von + diff;
    for( i = 0; i < bpz; ++i ) sp->wert[i] = spwert[i] / data->mittelueber;
    nt_auswertung( sp, index++, data );
    zeile += 2;
    while( zeile < data->bis )
      {
      isb_get_pixel( frb, zeile - 2, wpz, (char*) buffer );
      for( j = 0; j < bpz; ++j ) spwert[j] -= buffer[j];
      isb_get_pixel( frb, zeile + diff, wpz, (char*) buffer );
      for( j = 0; j < bpz; ++j ) spwert[j] += buffer[j];
      sp->von += 2;
      sp->bis += 2;
      zeile += 2;
      for( i = 0; i < bpz; ++i ) sp->wert[i] = spwert[i] / data->mittelueber;
      nt_auswertung( sp, index++, data );
      }
    }
  free( sp );
  free( spwert );
  }

/*
**  Abspeichern der Verteilungen
**  fname : Name des Files in das gespeichert wird
**  data  : diese Daten werden gespeichert
*/
static void LOCAL nt_speicher ( const char* fname, const ntspeicher* data )
  {
  FILE* file = fopen( fname, "ab" );
  if( file == NULL )
    printf( "File '%s' kann nicht zum Ergänzen geöffnet werden.\n", fname );
  else
    {
    printf( "Schreibe in File '%s'.\n", fname );
    if( fwrite( data, sizeof *data, 1, file ) != 1 )
      puts( "Fehler beim Schreiben in das File." );
    if( fclose( file ) )
      puts( "Fehler beim Schließen des Files." );
    }
  getreturn();
  }

/*
**  Bestimmen der Grenzen für die Aufnahme der Dichteverteilung
**  frb : Frame Buffer Number
**  von : dort obere Grenze speichern
**  bis : dort untere Grenze speichern
*/
static void LOCAL nt_grenzen ( int frb, int* von, int* bis )
  {
  do{
    puts( "Obere Grenze für Verteilung" );
    *von = zeilencrrs( frb, 0 );
    puts( "Untere Grenze für Verteilung" );
    *bis = zeilencrrs( frb, zpb-1 );
    if( *von > *bis )
      {
      const int akku = *von;
      *von = *bis;
      *bis = akku;
      }
    printf( "Zeilen %d bis %d ok", *von, *bis );
    }
    while( ! janein() );
  }

/*
**  Die Dichteverteilung n(x) und die Temperaturverteilung T(x)
**  über den Spalt (x) ermitteln
**  Die n und T Werte werden aus den Spektren automatisch ermittelt
**  Die gewonnen Daten werden in Datei "plotfile" geschrieben
**  frb : Framebuffer Number
*/
void nt_grafik ( int frb )
  {
  if( checkinit() )
    {
    ntspeicher data;
    fputs( "Dichte- und Temperatur-Verteilung über den Spalt ermitteln\n"
           "Das Bild muß aus zwei gleichen Halbbildern bestehen.\n"
           "Name : ", stdout );
    data.name[0] = '\0';
    stredit( data.name, sizeof data.name );
    puts( "Über wieviele Zeilen soll jedes Spektrum gemittelt werden ?" );
    data.mittelueber = getzahl( 1, zpb/2 );
    data.art = getart( data.name );
    if( data.art == 'k' )
      puts( "n,T kann dann nicht berechnet werden." );
    else
      {
      nt_grenzen( frb, &data.von, &data.bis );
      data.bis -= data.mittelueber * 2;
      nt_aufnahme( frb, &data );
      nt_speicher( plotfile, &data );
      }
    }
  }

