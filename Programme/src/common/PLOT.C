/*
**  C  ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .02.1993
**
**  Plotroutine für HP-Laserjet und NEC Pinwriter
**
**  11.02.1993  Beginn
**  03.02.1998  Erweitert um Pinwriter Unterstützung
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "printer.h"
#include "plot.h"

/********************************************************************/

#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#define LOCAL _near _pascal

/*
**  DAtentypen zur Abkürzung
*/
typedef unsigned char uchar;
typedef unsigned int  uint;

enum zwischenlinien
  {
  _nurdot = 0,         /* ein normaler Punkt ist zu zeichnen */
  _vorne = 1,          /* eine hor. Linie */
  _hinten = 2          /* eine hor. Linie */
  };

/*
**  Information darüber welcher Art der Punkt in der Zeile ist
*/
typedef struct dotTAG
  {
  enum zwischenlinien ziehen;   /* Verbindungslinien nach unten */
  int next;                     /* nächster Punkt in der Zeile  */
  }                             /* -1, kein Punkt mehr          */
  dot;

/*
**  Alle Informationen für den Plot
**  Plot aus 255 horizontalen Zeilen
**  für jede Zeile wird angegeben wo die Punkte zu setzen sind
*/
typedef struct plotdataTAG
  {
  const uchar* wert;          /* die zu plottenden Werte                         */
  int len;                    /* Anzahl der Werte                                */
  int offset;                 /* Zeilenindex = Offset - wert[i]                  */
  int index [255];            /* Zeilen 0..254, Index erster Punkt in der Zeile  */
  dot feld [1];               /* dummy Wert 1, Index = Position in der Zeile     */
  }
  plotdata;

/********************************************************************/

/*
**  gesetzter plotmode : leere Zeile am oberen oder unteren Rand
**  können auf Wunsch abgeschnitten werden
*/
static enum plot_t plotmode = 0;

/*
**  Punkte pro Division auf horizontaler/vertikaler Achse
**  d.h. die Teilstriche auf den Achsen werden alle div_h (bzw. div_v)
**  Punkte gesetzt.
*/
static int div_h = 100;
static int div_v =  28;

/*
**  Bitmuster fürs Zeichnen
*/
static uint* tafeln[3];

/*
**  Fehlermeldung ausgeben
**  msg : String mit Fehlermeldung
*/
static void LOCAL error ( const char* msg )
  {
  fprintf( stderr, "PLOT : %s\n", msg );
  }

/*
**  Initialisieren des Moduls
**  falls ein Fehler auftritt wird das Programm beendet
*/
void plot_init ( void )
  {
  uint akku [3];
  int i, j;
  for( i = 0; i < 3; ++i )
    if( ( tafeln[i] = malloc( 4 * sizeof (uint) ) ) == NULL )
      {
      error( "Out of Heap, Bit-Tafeln können nicht angelegt werden" );
      exit( 1 );
      }
  akku[_nurdot] = 0xe000;
  akku[_vorne]  = 0x8000;
  akku[_hinten] = 0x2000;
  for( i = 0; i < 4; ++i )
    for( j = 0; j < 3; ++j )
      {
      tafeln[j][i] = ( akku[j] >> 8 ) | ( akku[j] << 8 );
      akku[j] >>= 2;
      }
  }

/*
**  Vorbereiten der Daten für den Plot
**  doti   : Listen werden erstellen : welcher Wert wird an welcher Position
**           angenommen
**  werte  : zu plottende Werte
**  len    : Anzahl der Werte
**  return : != 0, wenn alles ok
*/
static int LOCAL makedot( plotdata** pdoti, const unsigned char* wert, const int len )
  {
  plotdata* doti = malloc( sizeof (plotdata) + len * sizeof (dot) );
  int ok = doti != NULL;
  if( ok )
    {
    int i;
    uchar akku;
    *pdoti = doti;
    doti->len = len;
    doti->wert = wert;
    doti->offset = 255;
    memset( doti->index, 0xff, sizeof doti->index );
    for( i = 0; i < len && ok; ++i )
      if( ( akku = wert[i] ) > 0 )
        {
        const int ii = 255 - akku;
        doti->feld[i].next = doti->index[ii];
        doti->index[ii] = i;
        doti->feld[i].ziehen = i > 0 && wert[i-1] < akku ? _vorne : 0;
        if( i+1 < len && wert[i+1] < akku )
          doti->feld[i].ziehen += _hinten;
        }
    }
  else
    error( "Out of Heap, kann Tabelle mit Plotdaten nicht anlegen." );
  return ok;
  }

/*
**  Die Grafik-Zeilen nach unten schieben, so daß unten im Plot keine
**  leeren Zeilen sind
**  doti : Angaben der Dots
*/
static void LOCAL cutbottom ( plotdata *const doti )
  {
  int i = 254;
  while( i > 1 && doti->index[i] < 0 ) --i;
  if( i < 254 )
    {
    const int delta = 254 - i;
    memmove( doti->index + delta, doti->index, ( i + 1 ) * sizeof (int) );
    memset( doti->index, 0xff, delta * sizeof (int) );
    doti->offset += delta;
    }
  }

/*
**  Ausgeben einer Raster-Zeile-Grafik-Daten
**  bytes  : die auszugebenden Raster-Zeile
**  len    : Anzahl der Bytes
**  delta  : in diesen Einheiten darf die Zeile gekürzt werden
**  ofile  : File, binary write
**  return : != 0, wenn alles ok
*/
static int LOCAL rasterout ( const uchar* bytes, int len, const int delta, FILE* ofile )
  {
  int ok;
  int i = 0;
  while( len > i && bytes[len-i] == 0 ) ++i;
  len -= ( i / delta ) * delta;
  ok = printer_stripeout( (const char*) bytes, len, ofile );
  if( ! ok ) error( "Fehler beim Schreiben einer Rasterzeile." );
  return ok;
  }

/*
**  Ausführen des Plots bei Grafiktyp:
**      eine Zeile besteht aus einer Reihe von Bytes
**      darin bedeutet ein gesetztes Bit, Punkt im Ausdruck
**      das höchtswertige Bit (0x80) steht links
**      das niederwertigste Bit (0x01) steht rechts
**  ofile  : File, write binary
**  doti   : Angaben der Dots
**  zdots  : Anzahl der Zwischenzeilen
**  return : != 0, wenn alles ok
*/
static int LOCAL zeichne_bitZeileL ( FILE* ofile, const plotdata *const doti, const int zdots )
  {
  const int zeilenlen = ( doti->len + 3 ) / 4 + 2;
                        /* Runden auf Bytes, 4 Pixel pro Byte, 2 Bytes für Achse */
  int i = 0;            /* Zähler der Grafikzeilen */
  uchar* hauptzeile = malloc( 2 * zeilenlen );
  uchar* zwischenzeile = hauptzeile + zeilenlen;
  int ok = hauptzeile != NULL;
  if( ! ok ) error( "Out of Heap, kann Zeilenspeicher nicht anlegen." );
  if(  plotmode & _plotcuttop )
    while( i < 255 && doti->index[i] < 0 ) ++i;
  if( ok )
    {
    memset( zwischenzeile, 0, zeilenlen );
    /* linke Koordinatenachse */
    zwischenzeile[0] = 0x01;
    }
  while( i < 255  && ok )
    {
    int j;
    int index = doti->index[i];
    memcpy( hauptzeile, zwischenzeile, zeilenlen );
    /* Ticks an die linke Achse */
    if( ( 256 - i ) % div_v == 0 ) hauptzeile[0] = 0xff;
    /* Die Liste der Punkte in einer Zeile durchlaufen */
    while( index >= 0 )
      {
      const int offset = ( index >> 2 ) + 1;
      const int tafel = index & 0x03;
      uint *const haupt = (uint*) ( hauptzeile + offset );
      uint *const zwischen = (uint*) ( zwischenzeile + offset );
      *haupt |= tafeln[_nurdot][tafel];
      *zwischen &= ~ tafeln[_nurdot][tafel];
      if( doti->feld[index].ziehen & _vorne  ) *zwischen |= tafeln[_vorne][tafel];
      if( doti->feld[index].ziehen & _hinten ) *zwischen |= tafeln[_hinten][tafel];
      index = doti->feld[index].next;
      }
    ok = rasterout( hauptzeile, zeilenlen, 1, ofile );
    for( j = 0; j < zdots && ok; ++j )
      ok = rasterout( zwischenzeile, zeilenlen, 1, ofile );
    ++i;
    }
  if( ok )
    {
    /* untere Koordinatenachse */
    i = ( doti->len + 3 ) / 4 + 1;
    memset( hauptzeile, 0xff, i );
    ok = rasterout( hauptzeile, i, 1, ofile );
    memset( hauptzeile, 0x00, zeilenlen );
    for( i = 0; i < doti->len; i += div_h )
      {
      uint *const ptr = (uint*) ( hauptzeile + ( i >> 2 ) + 1 );
      *ptr |= tafeln[_vorne][ i & 0x03 ];
      }
    for( i = 0; i < 8 && ok; ++i )
      ok = rasterout( hauptzeile, zeilenlen, 1, ofile );
    }
  free( hauptzeile );
  return ok;
  }

/*
**  Ausführen des Plots bei Grafiktyp:
**      24 Zeilen pro Streifen
**      die drei Bytes des Streifens in der Folge: oben, mitte, unten
**      Bit 0x80 ist oben  
**  ofile  : File, write binary
**  doti   : Angaben der Dots
**  zdots  : Anzahl der Zwischenzeilen - wird z.Z. ignoriert
**  return : != 0, wenn alles ok
*/
static int LOCAL zeichne_24bitZeileO ( FILE* ofile, const plotdata *const doti, const int zdots )
  {
  const int bps = 3;                            /* 3 Bytes pro Streifenhöhe         */
  static uchar nadel = 0x80;                    /* Bitmuster für die oberste Nadel  */
  int i = 0;                                    /* Zähler der Grafikzeilen          */
  const int bufferlen = ( doti->len + 10 )*bps; /* + 10 Spalten für Achse           */
  uchar* buffer = malloc( bufferlen );
  int ok = buffer != NULL;
  if( ! ok ) error( "Out of Heap, kann Zwischenspeicher nicht anlegen." );
  /* vor jedem Plot die Nadeln austauschen */
  nadel = nadel == (uchar) 0x80 ? (uchar) 0x40 : (uchar) 0x80;
  if( plotmode & _plotcuttop )
    while( i < 255 && doti->index[i] < 0 ) ++i;
  /* Schleife über 255 Grafikzeilen + 5 Zeilen für untere Achse */
  while( ok && i < 255 + 5 )
    {
    int j;
    uchar akku = nadel;        /* Buffer für Bitmuster    */
    int z = 0;                 /* Nr. der Zeile aus Bytes */   
    memset( buffer, 0, bufferlen );
    while( ok && i < 255 + 5 && z < bps )
      {
      if( i < 255 )
        {
        const uchar w = (uchar) ( doti->offset - i );
        /* Die Liste der Punkte in einer Zeile durchlaufen */
        int index = doti->index[i];
        while( index >= 0 )
          {
          buffer[ z + 10 * bps + index * bps ] |= akku;
          index = doti->feld[index].next;
          }
        /* Zwischenstücke zeichnen */
        for( index = 1; index < doti->len; ++index )
          if( ( doti->wert[index-1] < w ) == ( doti->wert[index] > w ) )
            buffer[ z + 10 * bps + index * bps ] |= akku;
        /* Ticks an die linke Achse */
        if( ( 256 - i ) % div_v == 0 )
          for( index = 0; index < 10*bps; index += bps ) buffer[z+index] |= akku;
        } 
      else if( i == 255 )
        {
        /* untere Koordinatenachse */
        int index;
        for( index = 0; index < doti->len + 10; ++index )
          buffer[ z + index * bps ] |= akku;
        }
      else
        {
        /* Ticks an untere Koordinatenachse */
        int index;
        for( index = 0; index < doti->len; index += div_h )
          buffer[ z + 10 * bps + index * bps ] |= akku;
        }
      ++i;
      akku >>= 2;
      if( akku == (uchar) 0 ) 
        {
        akku = nadel;
        ++z;
        }
      }
    /* linke Koordinatenachse */
    for( j = 0; j < bps; ++j ) buffer[10*bps+j] = 0xff;
    ok = rasterout( buffer, bufferlen, bps, ofile );
    }
  free( buffer );
  return ok;
  }

/*
**  Ausführen des Plots
**  ofile  : File, write binary
**  doti   : Angaben der Dots
**  zdots  : Anzahl der Zwischenzeilen
**  return : != 0, wenn alles ok
*/
static int LOCAL zeichne ( FILE* ofile, const plotdata *const doti, const int zdots )
  {
  int ok;
  switch( printer_graphic() )
    {
    case _bitZeileL   : ok = zeichne_bitZeileL( ofile, doti, zdots );
                        break;
    case _24bitZeileO : ok = zeichne_24bitZeileO( ofile, doti, zdots );
                        break;
    default           : error( "Das Grafikformat des Druckers wird nicht unterstützt." );
                        ok = 0;
                        break;
   }
  return ok;
  }

/*
**  Einstellen des Plotmodes für die folgenden Plots
**  mode   : der neue Plotmode
**  return : der alte Plotmode
*/
enum plot_t plot_setmode ( enum plot_t mode )
  {
  enum plot_t result = plotmode;
  plotmode = mode;
  return result;
  }

/*
**  Einstellen Punkte pro Divison, Teilstriche an den Achsen werden alle
**  h (bzw. v) Punkte gesetzt
**  h : Punkte pro Division auf horizontaler Achse
**  v : Punkte pro Division auf vertikaler Achse
*/
void plot_div ( int h, int v )
  {        
  div_h = h;
  div_v = v;
  }

/*
**  Ausgabe eines Plots in das Ausgabefile
**  In horizontaler Richtung 2 Dots pro Zahlenwert benutzt
**  ofile  : Ausgabe in dieses File, write binary
**  werte  : zu plottende Werte
**  len    : Anzahl der Werte
**  zdots  : Anzahl der Zeilen zwischen zwei Werten
**  return : 0, wenn die Ausgabe gelungen ist
*/
int plot ( FILE* ofile, const unsigned char* werte, int len, int zdots )
  {
  plotdata* doti;
  int ok = len > 0;
  if( ! ok ) error( "Anzahl der Werte < 1" );
  if( ok && ( ok = zdots >= 0 ) == 0 ) error( "Anzahl der Zwischenzeilen < 0" );
  if( ok ) ok = makedot( &doti, werte, len );
  if( ok )
    {
    if( plotmode & _plotcutbottom ) cutbottom ( doti );
    printer_code( _graphicOn, ofile );
    ok = zeichne( ofile, doti, zdots );
    printer_code( _graphicOff, ofile );
    }
  free( doti );
  return ! ok;
  }

