/*
**  Zortech C 2.1 / Mircosoft C 5.1
**  .09.1994 Ulrich Berntien + Jens Raacke
**
**  Ausgabe eines IMG-(Halb)Bilds auf einer TSENG-Karte
**
**  06.09.1994  Beginn
*/

#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include "imgview.h"

#ifndef __ZTC__
#  include <conio.h>
#  include <stddef.h>
#endif

/*
**  die Nummer des Video-Interrupts des IBM-BIOS
*/
static const int videoBios = 0x10;

/*
**  Typ der gefundenen TSENG-Karte
**  Bei Karten mit möglicher True-Color-Unterstützung ist der Code für den
**  True-Color-Mode = normaler Code + 1.
*/
static enum chipsetTAG
  {
  _none, _et3000, _et4000, _et4000true, _w32, _w32true
  }
  chipset = _none;

/********************************************************************/

/*
**  Fehlermeldung und Programmende, wenn bei dem Zugriff auf
**  TSENG-Karte, diese nicht gefunden wurde
*/
static void LOCAL noTseng ( void )
  {
  error( "keine TSENG-Videokarte vorhanden.\n" );
  }

/*
**  Schreiben in Register der Video-Karte über Angeben des Indices
**  pt     : Register für Ausgabe des Index
**  index  : Index des Video-Registers
**  wert   : diese Wert schreiben
*/
static void LOCAL outIndexed ( unsigned pt, byte index, byte wert )
  {
  outpw( pt, (unsigned) index + (unsigned) wert << 8 );
  }

/*
**  Lesen aus Register der Video-Karte über Angeben des Indices
**  pt     : Register für Ausgabe des Index
**  index  : Index des Video-Registers
**  return : gelesener Wert
*/
static byte LOCAL inIndexed ( unsigned pt, byte index )
  {
  outp( pt, index );
  return (byte) inp( pt+1 );
  }

/*
**  Testet ob Bits eines Registers lese/schreibbar sind
**  reg    : Nummer des Registers
**  maske  : die Bits dieser Maske testen
**  return : Sind die Bits lese/schreibbar ?
*/
static int LOCAL registerRWtest ( unsigned reg, byte maske )
  {
  const byte old = (byte) inp( reg );
  byte a, b;
  outp( reg, old & (~maske) );
  a = (byte) inp( reg ) & maske;
  outp( reg, old | maske );
  b = (byte) inp( reg ) & maske;
  outp( reg, old );
  return ( a == 0 ) && ( b == maske );
  }

/*
**  ähnlich registerRWtest() für indiziertes Video-Register
**  pt     : in dieses Register den Index schreiben
**  index  : Index des Video-Register
**  maske  : die Bits dieser Maske testen
**  return : Sind die Bits lese/schreibbar ?
*/
static int LOCAL indexedRWtest ( unsigned pt, byte index, byte maske )
  {
  outp( pt, index );
  return registerRWtest( pt+1, maske );
  }

/*
**  Erfragen der Typ-Nummer des verwendeten DAC-Chips über Tseng-Bios
**  (nicht bei ET-3000)
**  return : Typ-Nummer des DACs
*/
static int LOCAL getDACtype ( void )
  {
  union REGS reg;
  reg.x.ax = 0x10f1;
  int86( videoBios, &reg, &reg );
  return reg.x.ax == 0x10 ? reg.h.bl : 0;
  }

/*
**  Setzen des TrueColor-Modes über das Tseng-Bios
**  mode   : Grafikmodus
**  return : alles ok ?
*/
static int LOCAL setTrueColorMode ( byte mode )
  {
  union REGS reg;
  reg.x.ax = 0x10f0;
  reg.h.bl = 0xff;
  reg.h.bh = mode;
  int86( videoBios, &reg, &reg );
  return reg.x.ax == 0x10;
  }

/*
**  Setzen der Position des Fensters der TSENG-Karte
**  segment : Position in 64KByte-Einheiten bzgl. Anfang des Speichers
*/
static void LOCAL setSegment ( unsigned segment )
  {
  switch( chipset )
    {
    case _et3000     : outp( 0x3cd, 0x40 | segment | (segment<<3) );
                       break;
    case _et4000     :
    case _et4000true : outp( 0x3cd, segment | (segment<<4) );
                       break;
    case _w32        :
    case _w32true    : {
                       const unsigned low = segment & 0x0f;
                       const unsigned high = segment >> 4;
                       outp( 0x3cd, low | (low<<4) );
                       outp( 0x3cb, high | (high<<4) );
                       break;
                       }
    default          : noTseng();
    }
  }

/*
**  Erfragen der Anzahl der Bytes pro Bildschirmzeile
**  return : Abstand der Bildschirmzeilen in Bytes
*/
static unsigned getBytesPerLine ( void )
  {
  unsigned size;
  const byte len = inIndexed( 0x3d4, 0x13 ); /* in 2,4, oder 8 Bytes */
  const byte bytemode = inIndexed( 0x3d4, 0x17 ) & 0x40;
  const byte doubleword = inIndexed( 0x3d4, 0x14 ) & 0x40;
  if( doubleword )
    size = (unsigned) len * 8;
  else
    if( bytemode )
      size = (unsigned) len * 2;
    else
      size = (unsigned) len * 4;
  return size;
  }

/*
**  Chipset der Tseng-Karte bestimmen und in chipset speichern
**  return : Konnte ein Chipset bestimmt werden ?
*/
static int LOCAL setChipset ( void )
  {
  const int oldKey = inp( 0x3d8 );
  outp( 0x3bf, 0x03 );
  outp( 0x3d8, 0xa0 );  /* Key setzen */
  if( registerRWtest( 0x3cd, 0x3f ) )
    {
    if( indexedRWtest( 0x3d4, 0x33, 0x0f ) )
      if( registerRWtest( 0x3cb, 0x33 ) )
        chipset = _w32;
      else
        chipset = _et4000;
    else
      chipset = _et3000;
    outp( 0x3d8, oldKey );
    if( chipset != _et3000 && getDACtype() >= 3 ) ++chipset;
    }
  else
    chipset = _none;
  return chipset != _none;
  }

/********************************************************************/

/*
**  Testen ob eine TSENG-Karte aktiv ist
**  return : Ist eine TSENG-Karte aktiv ?
*/
int tseng_testen ( void )
  {
  int test1, test2;
  outp( 0x3bf, 0 );
  test1 = inp( 0x3d8 ) & 0x40;
  outp(  0x3bf, 2 );
  test2 = inp( 0x3d8 ) & 0x40;
  return ( ! test1 ) && test2 && setChipset();
  }

/*
**  Zustand der TSENG-Karte retten
*/
void tseng_retten ( void )
  {
  ega_retten();
  }

/*
**  Den mit tseng_retten() geretteten Zustand wiederherstellen.
*/
void tseng_ruecksetzen ( void )
  {
  ega_ruecksetzen();
  }

/*
**  die TSENG-Karte für Ausgabe des IMG-(Halb)Bilds initialisieren
*/
void tseng_initialisieren ( void )
  {
  if( chipset == _none ) setChipset();
  switch( chipset )
    {
    case _et4000true :
    case _w32true    : if( setTrueColorMode( 0x2e ) )
                         break;
                       else
                         --chipset;
    case _et3000     :
    case _et4000     :
    case _w32        : setVideoMode( 0x2e );
                       setGrautabelle();
                       break;
    default          : noTseng();
    }
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die TSENG-Karte
**  in 256 Farben-Modus
*/
static void LOCAL anzeigen_256 ( const byte* zeilen [hoehe] )
  {
  const unsigned bytesPerLine = getBytesPerLine();
  const unsigned long laengeFenster = 64 * 1024L;
  unsigned segmentFenster = 0;
  unsigned long endeFenster = laengeFenster;
  byte* video = MK_FP( 0xa000, 0 );
  unsigned long position = 0L;
  int zeile;
  setSegment( segmentFenster );
  for( zeile = 0; zeile < hoehe; ++zeile )
    if( position + breite <= endeFenster )
      {
      memcpy( video, zeilen[zeile], breite );
      video += bytesPerLine;
      position += bytesPerLine;
      }
    else
      {
      int teil1 = (unsigned) ( endeFenster - position );
      /* Anfang der Zeile ins 'alte Fenster' kopieren */
      if( teil1 > 0 )
        {
        memcpy( video, zeilen[zeile], teil1 );
        video += teil1;
        position += teil1;
        }
      else
        teil1 = 0;
      /* Verschieben des Fensters */
      endeFenster += laengeFenster;
      setSegment( ++segmentFenster );
      /* Ende der Zeile ins 'neue Fenster' kopieren */
      memcpy( video, zeilen[zeile] + teil1, breite - teil1 );
      video += bytesPerLine - teil1;
      position += bytesPerLine - teil1;
      }
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die TSENG-Karte
**  im TRUE-Color-Modus
*/
static void LOCAL anzeigen_true ( const byte* zeilen [hoehe] )
  {
  const unsigned bytesPerLine = getBytesPerLine();
  const unsigned long laengeFenster = 64 * 1024L;
  unsigned segmentFenster = 0;
  unsigned long endeFenster = laengeFenster;
  byte* video = MK_FP( 0xa000, 0 );
  unsigned long position = 0L;
  int zeile;
  setSegment( segmentFenster );
  for( zeile = 0; zeile < hoehe; ++zeile )
    if( position + breite*3 <= endeFenster )
      {
      multiCopy( video, zeilen[zeile], breite );
      video += bytesPerLine;
      position += bytesPerLine;
      }
    else
      {
      int teil1bytes = (unsigned) ( endeFenster - position );
      div_t teil1;
      /* Anfang der Zeile ins 'alte Fenster' kopieren */
      if( teil1bytes > 0 )
	{
        int i;
        teil1 = div( teil1bytes, 3 );
	multiCopy( video, zeilen[zeile], teil1.quot );
	video += teil1.quot * 3;
        for( i = 0; i < teil1.rem; ++i ) *video++ = zeilen[zeile][teil1.quot];
        }
      else
	{
	teil1.quot = teil1.rem = 0;
	teil1bytes = 0;
	}
      /* Verschieben des Fensters */
      endeFenster += laengeFenster;
      setSegment( ++segmentFenster );
      /* Ende der Zeile ins 'neue Fenster' kopieren */
      if( teil1.rem > 0 )
	{
        int i;
        const int len = 3 - teil1.rem;
        for( i = 0; i < len; ++i ) video[i] = zeilen[zeile][teil1.quot];
	video += len;
        teil1bytes += len;
	++teil1.quot;
	}
      multiCopy( video, zeilen[zeile] + teil1.quot, breite - teil1.quot );
      video += bytesPerLine - teil1bytes;
      position += bytesPerLine;
      }
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die TSENG-Karte
*/
void tseng_anzeigen ( const byte* zeilen [hoehe] )
  {
  switch( chipset )
    {
    case _et3000     :
    case _et4000     :
    case _w32        : anzeigen_256( zeilen );
		       break;
    case _et4000true :
    case _w32true    : anzeigen_true( zeilen );
		       break;
    default          : noTseng();
    }
  }
