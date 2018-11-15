/*
**  Zortech C 2.1 / Mircosoft C 5.1
**  .08.1994 Ulrich Berntien + Jens Raacke
**
**  Ausgabe eines IMG-(Halb)Bilds auf einer SVGA-Karte
**  über das VESA-BIOS
**
**  15.08.1994  Beginn
**  07.09.1994  erweitert um VESA-True-Color Ausgabe
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
**  Funktionsnummer aller vesaBios-Funktionen innerhalb des
**  Video-Interrupts
*/
static const byte vesaBios = 0x4f;

/*
**  Datenstruktur der Information, die von der VESA-Funktion vesaBios/0x01
**  geliefert wird
**  (Ausrichtung der Komponenten auf Word-Grenzen.)
*/
typedef struct modeInfoTAG
  {
  unsigned modus;             /* Modus-Flag                              */
  unsigned flags;             /* High/Low-Byte = Flags für 1./2. Fenster */
  unsigned granularitaet;     /* Granularität in KByte mit der die       */
                              /* Zugriffsfenster verschoben werden       */
  unsigned size;              /* Größe der Fenster in KByte              */
  unsigned segment1;          /* Segment-Adresse des 1. Fensters         */
  unsigned segment2;          /* Segment-Adresse des 2. Fensters         */
  void* setFunktion;          /* Pointer auf Funktion zum Setzen der     */
                              /* Fensterposition im Video-Speicher       */
  unsigned perLine;           /* Anzahl der Bytes in einer Video-Zeile   */
  unsigned optional [300];    /* optionale Daten                         */
  }
  modeInfo;

/*
**  Zeiger auf die von svga_retten() angelegte "save area"
**  der Bereich wird von svga_ruecksetzen() benutzt und freigegeben
*/
static byte* saveArea = NULL;

/*
**  Dieser Funktionscode wird zur Darstellung des IMG-Bilds benutzt
**       0x0112 : 640 x 480 TrueColor
**  oder 0x0115 : 800 x 600 TrueColor
**  oder 0x0100 : 640 x 400 bei 256 Farben
**  oder 0x0101 : 640 x 480 bei 256 Farben
**  oder 0x0103 : 800 x 600 bei 256 Farben
**  wird von unterstuetzt() gesetzt
*/
static unsigned functionCode = 0;

/*
**  Zeiger auf einen Block mit Informationen für den eingestellten
**  Video-Mode zur Bildausgabe (functionCode)
**  Wird von svga_initialisieren() angelegt und von svga_ruecksetzen()
**  gelöscht.
*/
static modeInfo* info = NULL;

/********************************************************************/

/*
**  Fehlermeldung und Programmende, wenn bei dem Zugriff auf eine
**  VESA-Funktion keine VESA-BIOS gefunden wurde
*/
static void LOCAL noVesa ( void )
  {
  error( "kein VESA - BIOS vorhanden.\n" );
  }

/*
**  Bestimmt die Größe der "save area" für das sichern alle Komponenten
**  des Status der VESA-Karte
**  return : die Größe der "save area" in Bytes
*/
static unsigned LOCAL sizeSaveArea ( void )
  {
  union REGS regs;
  regs.h.ah = vesaBios;                /* Funktion vesaBios       */
  regs.h.al = 0x04;                    /* Unterfunktion 0x04/0x00 */
  regs.h.dl = 0x00;
  regs.x.cx = 0x000f;                  /* alle Komponenten        */
  int86( videoBios, &regs, &regs );
  if( regs.h.al != vesaBios ) noVesa();
  return regs.x.bx * 64;
  }

/*
**  Speichern/Restaurieren des Zustands der VESA-Karte
**  save     : soll der Status gespeichert werden ?, sonst sichern
**  saveArea : Speicherbereich der Größe sizeSaveArea()
*/
static void LOCAL saveLoad ( int save, byte* saveArea )
  {
  const int code = save ? 0x01 : 0x02;
  union REGS regs;
  struct SREGS sreg;
  regs.h.ah = vesaBios;           /* Funktion vesaBios       */
  regs.h.al = 0x04;               /* Unterfunktion 0x04/code */
  regs.h.dl = (byte) code;
  regs.x.cx = 0x000f;             /* alle Komponenten        */
  regs.x.bx = FP_OFF( saveArea );
  sreg.es = FP_SEG( saveArea );
  int86x( videoBios, &regs, &regs, &sreg );
  if( regs.h.al != vesaBios ) noVesa();
  }

/*
**  Informationen über den benutzten vesaBios-Mode (functionCode) lesen
**  return : Zeiger auf einen Buffer für die Informationen
**           ( free nicht vergessen )
*/
static modeInfo* LOCAL getModeInfo ( void )
  {
  modeInfo *ptr = mallocOk( sizeof (modeInfo) );
  union REGS regs;
  struct SREGS sreg;
  regs.h.ah = vesaBios;                  /* Funtion vesaBios        */
  regs.h.al = 0x01;                      /* Unterfunktion 0x01      */
  regs.x.cx = functionCode;              /* Info über diesen Mode   */
  regs.x.di = FP_OFF( ptr );             /* dort das Info schreiben */
  sreg.es = FP_SEG( ptr );
  int86x( videoBios, &regs, &regs, &sreg );
  if( regs.h.al != vesaBios ) noVesa();
  return ptr;
  }

/*
**  Setzen des ersten Zugriffsfensters auf die gegebene Segment-Startadresse
**  segment : Segment-Startadresse innerhalb des Video-Speichers
**            (in Einheiten von info->granularitaet)
*/
static void LOCAL setWindow ( unsigned segment )
  {
  union REGS regs;
  regs.h.ah = vesaBios;              /* Funktion vesaBios        */
  regs.h.al = 0x05;                  /* Unterfunktion 0x05/0x00  */
  regs.h.bh = 0x00;
  regs.h.bl = 0x00;                  /* das erste Fenster        */
  regs.x.dx = segment;
  int86( videoBios, &regs, &regs );
  if( regs.h.al != vesaBios ) noVesa();
  }

/*
**  Erzeugen einer Farbpalette mit Graustufen schwarz bis weiß
**  und übergeben der Farbpalette in die DAC-Farbregister
*/
void setGrautabelle ( void ) 
  {
  union REGS regs;
  struct SREGS sreg;
  byte* tabelle = mallocOk( 256 * 3 );
  int stufe = 0;
  int index = 0;
  while( stufe < 256 )
    {
    tabelle[index] = tabelle[index+1] = tabelle[index+2] = (byte) (stufe/4);
    ++stufe;
    index += 3;
    }
  regs.h.ah = 0x10;                    /* Funktion 0x10          */
  regs.h.al = 0x12;                    /* Subfunktion 0x12       */
  regs.x.bx = 0;                       /* ab Register 0          */
  regs.x.cx = 256;                     /* 256 Register laden     */
  regs.x.dx = FP_OFF( tabelle );       /* mit dieser Farbtabelle */
  sreg.es = FP_SEG( tabelle );
  int86x( videoBios, &regs, &regs, &sreg );
  free( tabelle );
  }

/*
**  Durchsuchen der Liste der Unterstützen Funktionscodes nach
**  den gewünschten Funktionscodes durchsuchen
**  return : wurde ein brauchbarer Code gefunden ?
**           (und in functionCode gespeichert)
*/
static int LOCAL unterstuetzt ( const unsigned* liste )
  {
  static const unsigned test [6] = { 0x103, 0x101, 0x100, 0x115, 0x112, 0xffff };
  int found = -1;
  int i;
  for( i = 0; liste[i] != 0xffff; ++i )
    {
    int j = 0;
    while( liste[i] != test[j] && test[j] != 0xffff ) ++j;
    if( test[j] != 0xffff && j > found ) found = j;
    }
  functionCode = found >= 0 ? test[found] : 0;
  return found >= 0;
  }

/********************************************************************/

/*
**  Testen ob eine VESA-Karte aktiv ist
**  return : Ist eine VESA-Karte aktiv ?
*/
int svga_testen ( void )
  {
  int ok;
  byte* buffer = mallocOk( 1024 );
  union REGS regs;
  struct SREGS sreg;
  regs.h.ah = vesaBios;              /* Funktion vesaBios          */
  regs.h.al = 0x00;                  /* Unterfunktion 0x00         */
  regs.x.di = FP_OFF( buffer );      /* Zeiger auf den Info-Puffer */
  sreg.es = FP_SEG( buffer );
  int86x( videoBios, &regs, &regs, &sreg );
  ok = regs.h.al == vesaBios && regs.h.ah == 0x00;
  if( ok )
    {
    unsigned** ptr = (unsigned**) ( buffer + 0x000e );
    const unsigned* liste = *ptr;
    ok = unterstuetzt( liste );
    }
  free( buffer );
  return ok;
  }

/*
**  Zustand der VESA-Karte retten
*/
void svga_retten ( void )
  {
  ega_retten();
  if( saveArea == NULL )
    {
    saveArea = mallocOk( sizeSaveArea() );
    saveLoad( 1, saveArea );
    }
  }

/*
**  Den mit svga_retten() geretteten Zustand wiederherstellen.
**  Da nicht alle wichtigen Daten von der BIOS-Funktion gerettet
**  werden, wird zuerst in den alten Videomodus zurückgeschaltet.
*/
void svga_ruecksetzen ( void )
  {
  ega_ruecksetzen();
  if( saveArea != NULL )
    {
    saveLoad( 0, saveArea );
    free( saveArea );
    saveArea = NULL;
    }
  if( info != NULL )
    {
    free( info );
    info = NULL;
    }
  }

/*
**  die VESA-Karte für Ausgabe des IMG-(Halb)Bilds initialisieren
*/
void svga_initialisieren ( void )
  {
  union REGS regs;
  if( functionCode == 0 && svga_testen() == 0 ) noVesa();
  regs.h.ah = vesaBios;             /* Funktion vesaBios          */
  regs.h.al = 0x02;                 /* Unterfunktion 0x02         */
  regs.x.bx = functionCode;         /* der ausgewählte Videomodus */ 
  int86( videoBios, &regs, &regs );
  if( regs.h.al != vesaBios ) noVesa();
  info = getModeInfo();
  setGrautabelle(); 
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die VESA-Karte bei 256 Farb-Modus
*/
static void LOCAL anzeigen256 ( const byte* zeilen [hoehe] )
  {
  int zeile;
  const unsigned long laengeFenster = info->size * 1024L;
  unsigned segmentFenster = 0;
  unsigned long endeFenster = laengeFenster;
  byte* video = MK_FP( info->segment1, 0 );
  unsigned long position = 0L;
  unsigned schiebung = info->granularitaet;
  while( schiebung + info->granularitaet <= info->size )
    schiebung += info->granularitaet;
  setWindow( segmentFenster );
  for( zeile = 0; zeile < hoehe; ++zeile )
    if( position + breite <= endeFenster )
      {
      memcpy( video, zeilen[zeile], breite );
      video += info->perLine;
      position += info->perLine;
      }
    else
      {
      int teil1 = (unsigned) ( endeFenster - position );
      /* Anfang der Zeile ins 'alte Fenster' kopieren */
      if( teil1 > 0 )
        {
        memcpy( video, zeilen[zeile], teil1 );
        video += teil1;
        }
      else
        teil1 = 0;
      /* Verschieben des Fensters */
      segmentFenster += schiebung / info->granularitaet;
      endeFenster += (long) schiebung * 1024;
      video -= schiebung * 1024;
      setWindow( segmentFenster );
      /* Ende der Zeile ins 'neue Fenster' kopieren */
      memcpy( video, zeilen[zeile] + teil1, breite - teil1 );
      video += info->perLine - teil1;
      position += info->perLine;
      }
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die VESA-Karte bei TrueColor-Modus
*/
static void LOCAL anzeigenTrue( const byte* zeilen [hoehe] )
  {
  int zeile;
  const unsigned long laengeFenster = info->size * 1024L;
  unsigned segmentFenster = 0;
  unsigned long endeFenster = laengeFenster;
  byte* video = MK_FP( info->segment1, 0 );
  unsigned long position = 0L;
  unsigned schiebung = info->granularitaet;
  while( schiebung + info->granularitaet <= info->size )
    schiebung += info->granularitaet;
  setWindow( segmentFenster );
  for( zeile = 0; zeile < hoehe; ++zeile )
    if( position + breite*3 <= endeFenster )
      {
      multiCopy( video, zeilen[zeile], breite );
      video += info->perLine;
      position += info->perLine;
      }
    else
      {
      div_t teil1;
      int teil1bytes = (unsigned) ( endeFenster - position );
      /* Anfang der Zeile ins 'alte Fenster' kopieren */
      if( teil1bytes > 0 )
        {
        int i;
        teil1 = div( teil1bytes, 3 );
        multiCopy( video, zeilen[zeile], teil1.quot );
        video += teil1.quot;
        for( i = 0; i < teil1.rem; ++i ) *video++ = zeilen[zeile][teil1.quot];
        }
      else
        {
        teil1bytes = 0;
        teil1.quot = teil1.rem = 0;
        }
      /* Verschieben des Fensters */
      segmentFenster += schiebung / info->granularitaet;
      endeFenster += (long) schiebung * 1024;
      video -= schiebung * 1024;
      setWindow( segmentFenster );
      /* Ende der Zeile ins 'neue Fenster' kopieren */
      if( teil1.rem > 0 )
        {
        const int len = 3 - teil1.rem;
        int i;
        for( i = 0; i < len; ++i ) *video++ = zeilen[zeile][teil1.quot];
        ++teil1.quot;
        teil1bytes += 3;
        }
      multiCopy( video, zeilen[zeile] + teil1.quot, breite - teil1.quot );
      video += info->perLine - teil1bytes;
      position += info->perLine;
      }
  }

/*
**  Ausgeben des IMG-(Halb)Bilds auf die VESA-Karte
*/
void svga_anzeigen ( const byte* zeilen [hoehe] )
  {
  if( functionCode > 0x103 )
    anzeigenTrue( zeilen );
  else
    anzeigen256( zeilen );
  }
