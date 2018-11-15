/*
**  Zortech C 2.1 / Microsoft C 5.1
**  .08.1994 Ulrich Berntien
**
**  Ausgabe eines IMG-(Halb)Bilds auf einer VGA-Karte
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
**  Interrupt-Nummer des Video-BIOS des IBM-PC
*/
static const int videoBios = 0x10;

/*
**  Zeiger auf die von vga_retten() angelegte "save area"
**  der Bereich wird von vga_ruecksetzen() benutzt un freigegeben
*/
static byte* saveArea = NULL;

/********************************************************************/

/*
**  Fehlermeldung, wenn beim Zugriff auf eine VGA-Funktion
**  kein VGA-BIOS angesprochen wurde
*/
static void LOCAL noVga ( void )
  {
  error( "kein VGA-BIOS vorhanden.\n" );
  }

/*
**  Die Größe der "save area" bestimmen um alle Einstellungen
**  der VGA-Karte/BIOS zu sichern
**  return : die Größe der "save area" in Bytes
*/
static unsigned LOCAL sizeSaveArea ( void )
  {
  union REGS regs;
  regs.h.ah = 0x1c;                 /* Funktion 0x1c      */
  regs.h.al = 0x00;                 /* Unterfunktion 0x00 */
  regs.x.cx = 0x0007;               /* alle Komponenten   */
  int86( videoBios, &regs, &regs );
  if( regs.h.al != 0x1c ) noVga();
  return regs.x.bx * 64;
  }

/*
**  Sichern/Restaurieren alle Komponenten des Status der VGA-Karte
**  save     : Soll der Status gespeichert werden ?
**             sonst wird er geladen
**  saveArea : Buffer der Größe sizeSaveArea() für den Status der Karte
*/
static void LOCAL saveLoad ( int save, byte* saveArea )
  {
  const int code = save ? 0x01 : 0x02;
  union REGS regs;
  struct SREGS sreg;
  regs.h.ah = 0x1c;                 /* Funktion 0x1c                */
  regs.h.al = (byte) code;          /* Unterfunktion 0x01 oder 0x02 */
  regs.x.cx = 0x0007;               /* alle Komponenten             */
  regs.x.bx = FP_OFF( saveArea );
  sreg.es = FP_SEG( saveArea );
  int86x( videoBios, &regs, &regs, &sreg );
  if( regs.h.al != 0x1c ) noVga();
  }

/*
*/
static void LOCAL setEgaPalette ( void )
  {
  int i;
  inp( 0x3da );
  for( i = 0; i < 16; ++i )
    {
    outp( 0x3c0, i );    /* Index = i */
    outp( 0x3c0, i );    /* Wert  = i */
    }
  outp( 0x3c0, 0x20 );
  }

/*
**  eine Tabelle von 16 Graustufen in die DAC-Farbpalette ab
**  Register 15 laden
*/
static void LOCAL grauTabelle ( void )
  {
  union REGS regs;
  struct SREGS sreg;
  byte tabelle [16 * 3];
  int index = 0;             /* läuft über 16 DAC-Register */
  int grau = 0;              /* läuft von 0 bis 60 (6Bit)  */
      /* bei "byte grau" meldet ztc2 einen internen Fehler */
  while( index < 16*3 )
    {
    tabelle[index] = tabelle[index+1] = tabelle[index+2] = (byte) grau;
    index += 3;
    grau += 4;
    }
  regs.h.ah = 0x10;                  /* Funktion 0x10      */
  regs.h.al = 0x12;                  /* Unterfunktion 0x12 */
  regs.x.bx = 0;                     /* ab Register 0      */
  regs.x.cx = 16;                    /* 16 Register laden  */
  regs.x.dx = FP_OFF( tabelle );
  sreg.es = FP_SEG( tabelle );
  int86x( videoBios, &regs, &regs, &sreg );
  }

/********************************************************************/

/*
**  Testen ob eine VGA-Karte aktiv ist
**  return : Ist eine VGA-Karte aktiv ?
*/
int vga_testen ( void )
  {
  union REGS regs;
  regs.h.ah = 0x1a;                 /* Funktion 0x1a      */
  regs.h.al = 0x00;                 /* Unterfunktion 0x00 */
  int86( videoBios, &regs, &regs );
  return ( regs.h.al == 0x1a )                     /* VGA vorhanden         */
         && ( regs.h.bl == 7 || regs.h.bl == 8 );  /* Color oder sw-Monitor */
  }

/*
**  Zustand der VGA-Karte retten
*/
void vga_retten ( void )
  {
  ega_retten();
  if( saveArea == NULL )
    {
    saveArea = mallocOk( sizeSaveArea() );
    saveLoad( 1, saveArea );
    }
  }

/*
**  Den mit vga_retten() geretteten Zustand wiederherstellen.
**  Da nicht alle wichtigen Daten von der BIOS-Funktion gesichert
**  werden, wird zuerst der alte Video-Mode eingeschaltet.
*/
void vga_ruecksetzen ( void )
  {
  ega_ruecksetzen();
  if( saveArea != NULL )
    {
    saveLoad( 0, saveArea );
    free( saveArea );
    saveArea = NULL;
    }
  }

/*
**  die VGA-Karte für Ausgabe des IMG-(Halb)Bilds initialisieren
**  Mode 0x10 : 640 x 350 in 16 Farben
*/
void vga_initialisieren ( void )
  {
  union REGS regs;
  /* Grafikmode setzen */
  regs.h.ah = 0x00;            /* Funktion 0x00      */
  regs.h.al = 0x10;            /* Mode 0x10 mit CLS  */
  int86( videoBios, &regs, &regs );
  /* Farbauswahl über 16 verschiedene Paletten */
  regs.h.ah = 0x10;            /* Funktion 0x10                   */
  regs.h.al = 0x13;            /* Unterfunktion 0x13              */
  regs.h.bl = 0x00;            /* Bit 7 des Mode-Controllreg. mit */
  regs.h.bh = 0x01;            /* Wert 1 laden                    */
  int86( videoBios, &regs, &regs );
  /* Farbauswahl auf 16 Graustufen */
  regs.h.ah = 0x10;            /* Funktion 0x10               */
  regs.h.al = 0x13;            /* Unterfunktion 0x13          */
  regs.h.bl = 0x01;            /* Color-Select-Register laden */
  regs.h.bh = 0x00;            /* mit Wert 0x00               */
  int86( videoBios, &regs, &regs );
  /* Zugriff der CPU auf Speicher der VGA erlauben */
  regs.h.ah = 0x12;            /* Funktion 0x12      */
  regs.h.bl = 0x32;            /* Unterfunktion 0x32 */
  regs.h.al = 0x00;            /* Zugriff erlaubt    */
  int86( videoBios, &regs, &regs );
  setEgaPalette();
  grauTabelle();
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die VGA-Karte
*/
void vga_anzeigen ( const byte* zeilen [hoehe] )
  {
  ega_anzeigen( zeilen );
  }
