/*
**  Zortech C 2.1 / Microsoft C 5.1
**  .08.1994 Ulrich Berntien + Jens Raacke
**
**  Ausgabe eines IMG-(Halb)Bilds auf einer EGA-Karte
**
**  15.08.1994  Beginn
*/

#include <dos.h>
#include <stdlib.h>
#include "imgview.h"

#ifndef __ZTC__
#  include <conio.h>
#  include <stddef.h>
#endif

/*
**  Aufbau eines Farbpaletten-Registers der EGA-Karte
**  0. Bit : Blau (intensiv)
**  1. Bit : Grün (intensiv)
**  2. Bit : Rot  (intensiv)
**  3. Bit : Blau (weniger intensiv)
**  4. Bit : Grün (weniger intensiv)
**  5. Bit : Rot  (weniger intensiv)
**  6. und 7. Bit unbenutzt
*/
#define RGB(r,g,b) (byte) ( (r&1) << 5 | (r&2) << 1 | (g&1) << 4 | \
			    (g&2)      | (b&1) << 3 | (b&2) >> 1  )

/*
**  Segment des Video Speichers
*/
#define VSEGMENT 0xa000

/*
**  Portadresse des "Miscellaneous Output"
*/
#define MOUTPUT 0x03cc

/*
**  Portadresse der Graphics-Controllers
*/
#define CONTROLLER 0x03ce

/*
**  auf den Port des Graphicscontroller wird immer ein WORD ausgegeben,
**  dabei indiziert das Lowbyte ein Register auf der Videokarte und das
**  Highbyte wird dabei in dieses Register geschrieben.
*/
#define FUNCTIONSELECT  0x03
#define GRAPHICSMODE    0x05
#define BITMASK         0x08

/*
**  Interrupt zum Aufruf des Video-BIOS des IMB-PCs
*/
static const int videoBios = 0x10;

/********************************************************************/

/*
**  Datenstruktur um alle Einstellungen der EGA-Karte zu sichern
*/
typedef struct egaInfoTAG
  {
  byte videoMode;
  }
  egaInfo;

/*
**  Datenbereich wird von ega_retten() angelegt und von
**  ega_ruecksetzen() gelesen und freigegeben
*/
static egaInfo* saveArea = NULL;

/********************************************************************/

/*
**  Ausfüllen der Tabelle für die Umsetzung 256 Graustufen nach
**  16 Farbstufen
**  zeilen      : das Bild
**  farbtabelle : dort die Farbtabelle speichern
*/
static void LOCAL setFarbtabelle ( const byte* zeilen [hoehe],
                                   byte farbtabelle [256] )
  {
  int i;
  for( i = 0; i < 256; ++i ) farbtabelle[i] = (byte) ( i/16 );
  UNUSED( zeilen );
  }

/*
**  setzen der Palettenregister auf einen "Regenbogen" um die
**  Graustufen des IMG-Bilds zu simulieren.
*/
static void LOCAL regenbogenPalette ( void )
  {
  static const byte palette [17]  =
    {
    RGB(0,0,0),          /* 16 Farbpaletten-Register */
    RGB(0,0,1),
    RGB(0,0,2),
    RGB(0,0,3),
    RGB(0,1,3),
    RGB(0,2,3),
    RGB(0,3,2),
    RGB(0,3,1),
    RGB(1,3,0),
    RGB(2,3,0),
    RGB(3,2,0),
    RGB(3,1,0),
    RGB(3,0,0),
    RGB(3,1,1),
    RGB(3,2,2),
    RGB(3,3,3),
    RGB(0,0,0)           /* Overscan-Register = schwarz */
    };
  union REGS regs;
  struct SREGS sregs;
  regs.h.ah = 0x10;                   /* Funktion 0x10      */
  regs.h.al = 0x02;                   /* Unterfunktion 0x02 */
  regs.x.dx = FP_OFF( palette );
  sregs.es = FP_SEG( palette );
  int86x( videoBios, &regs, &regs, &sregs );
  }

/********************************************************************/

/*
**  Testen ob eine EGA-Karte aktiv ist
**  return : Ist eine EGA-Karte aktiv ?
*/
int ega_testen ( void )
  {
  union REGS regs;
  regs.h.ah = 0x12;                           /* Funktion 0x12      */
  regs.h.bl = 0x10;                           /* Unterfunktion 0x10 */
  regs.h.bh = 0xff;
  int86( videoBios, &regs, &regs );
  return ( regs.h.bh == 0 || regs.h.bl == 1 ) /* Color oder SW-Monitor */
         && ( regs.h.bl > 0 );                /* mehr als 64KByte      */
  }

/*
**  Zustand der EGA-Karte retten
*/
void ega_retten ( void )
  {
  if( saveArea == NULL )
    {
    saveArea = mallocOk( sizeof (egaInfo) );
    saveArea->videoMode = getVideoMode();
    }
  }

/*
**  den mit ega_retten() geretteten Zustand wiederherstellen
*/
void ega_ruecksetzen ( void )
  {
  if( saveArea != NULL )
    {
    setVideoMode( saveArea->videoMode );
    free( saveArea );
    saveArea = NULL;
    }
  }

/*
**  die EGA-Karte für Ausgabe des IMG-(Halb)Bilds initialisieren
**  Mode 0x10 : 640 x 350 mit 16 Farben (da nur EGA > 64KByte akzeptiert)
*/
void ega_initialisieren ( void )
  {
  setVideoMode( 0x10 );
  /* der CPU Zugriff auf den Speicher der Karte erlauben */
  outp( MOUTPUT, inp( MOUTPUT ) | 0x02 );
  regenbogenPalette();
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die EGA-Karte
*/
void ega_anzeigen ( const byte* zeilen [hoehe] )
#ifdef NO_ASSEMBLER
  {
  byte* video = MK_FP( VSEGMENT, 0x0000 );
  static byte farbtabelle [256]; /* ohne static liefert MS-C einen internen Fehler */
  unsigned bitmask = 0x8000;                    /* Bitmaske sitzt im Highbyte */
  int bit;
  setFarbtabelle( zeilen, farbtabelle );
  outpw( CONTROLLER, 0x0200 + GRAPHICSMODE );   /* read modus 0, write modus 2 */
  outpw( CONTROLLER, 0x0000 + FUNCTIONSELECT ); /* replace mode */
  for( bit = 0; bit < 8; ++bit )
    {
    int y;
    int videooffset = 0;
    outpw( CONTROLLER, bitmask + BITMASK );
    bitmask >>= 1;
    for( y = 0; y < hoehe; ++y, videooffset += 640/8 )
      {
      int x = bit;
      int i = videooffset;
      const byte *const zeile = zeilen[y];
      while( x < breite )
        {
        byte akku = video[i];
        video[i] = farbtabelle[ zeile[x] ];
        ++i;
        x += 8;
        }
      }

    }
  }
#else
  {
  byte farbtabelle [256];
  setFarbtabelle( zeilen, farbtabelle );
  egaOut( zeilen[0], farbtabelle );
  }
#endif
