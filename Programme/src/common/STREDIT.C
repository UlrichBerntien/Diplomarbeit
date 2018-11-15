/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .07.1993
**
**  Editierfeld für Strings auf dem Bildschirm behandlen
**
**  Literatur zu BIOS-Funktionen/Hardware :
**    Michael Tischer: PC-Intern 3.0, Data Becker GmbH
**
**  25.07.1993  Beginn
**  23.08.1993  Funktionen cls,putsxy und screensaver ergänzt
**  16.09.1993  Funktion cll ergänzt
*/

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"     /* nur für die Prototypen */

#ifndef _far
#  define _far    far
#  define _near   near
#  define _pascal pascal
#endif

#ifndef MK_FP
#  define MK_FP( seg, off ) ((void _far*) ( (unsigned long)(seg) << 16 + (off)))
#endif

#define LOCAL _pascal _near

/********************************************************************/

/*
**  Beschreibung der Funktionstasten
*/
static const char tasten [] =
  "\r"
  "<Zeilenschaltung> Eingabe beenden\n"
  "<Rückschritt>     Zeichen vor Cursorposition löschen\n"
  "CTRL-Y            komplettes Eingabefeld löschen\n"
  "CTRL-X            löschen ab Cursorposition bis Feldende\n"
  "Left              eine Spalte nach links\n"
  "Right             eine Spalte nach rechts\n"
  "Home              zur ersten Spalte\n"
  "CTRL - Left       zum ersten Zeichen im Feld\n"
  "CTRL - Right      zum letzten Zeichen im Feld\n"
  "End                         -\"-\n"
  "Delete            ein Zeichen löschen\n"
  "Insert            Einfügemodus ein/aus-schalten";

/*
** Fast immer unsigned char benutzt, damit einfaches trennen der
** Sonderzeichen ( < 0x20) von den normalen Zeichen
*/
typedef unsigned char uchar;

/*
**  Daten zu einem Editierfeld speichern
*/
typedef struct editdataTAG
  {
  int x;          /* Startspalte (0..79) auf dem Bildschirm */
  int y;          /* Startzeile  (0..24) auf dem Bildschirm */
  int screen;     /* muß der Bildschirm erhalten bleiben ?  */
  int len;        /* Länge des Buffers (mit '\0' am Ende)   */
  uchar* buffer;  /* der Buffer für den String              */
  }
  editdata;

/*
**  Zeichen Attribute im Bildschirmspeicher (monochrom)
*/
#define NORMSTYLE 0x07
#define EDITSTYLE 0x09

/*
**  Tastaturcodes (ASCII)
*/
#define CR    0x0d
#define BS    0x08
#define CTRLX 0x18
#define CTRLY 0x19

/*
**  Tastaturcodes (Scan-Codes, ASCII = 0)
*/
#define F1           0x3b
#define RIGHT        0x4d
#define CTRLRIGHT    0x74
#define END          0x4F
#define LEFT         0x4b
#define CTRLLEFT     0x73
#define INSERT       0x52
#define DELETE       0x53
#define HOME         0x47

/**** BIOS/Hardware Zugriff ****************************************/

/*
**  Aktuelle Konfiguration des Bildschirms
**  wird bei einigen BIOS/Hardwarezugriffen benutzt
*/
struct konfiguration
  {
  int ok;                /* wurde Konfiguration ermittelt ? */
  uchar seite;           /* aktuelle Bildschirmseite        */
  int cpl;               /* Zeichen pro Zeile               */
  int lps;               /* Zeilen pro Seite                */
  int mda;               /* Ist MDA aktiv ? (sonst CGA)     */
  unsigned _far* scr;    /* ads des Bildschirmspeichers     */
  }
  konfig;

/*
**  die aktuelle Cursor Position bestimmen
**  x : dort die Spalte (0..79) speichern
**  y : dort die Zeile  (0..24) speichern
*/
static void LOCAL getcrrspos ( int* x, int* y )
  {
  union REGS regs;
  regs.h.ah = 0x03;
  regs.h.bh = konfig.seite;
  int86( 0x10, &regs, &regs );
  *x = regs.h.dl;
  *y = regs.h.dh;
  }

/*
**  die Cursor Position setzen
**  x : die Spalte (0..79)
**  y : die Zeile  (0..24)
*/
void setcrrspos ( int x, int y )
  {
  union REGS regs;
  regs.h.ah = 0x02;
  regs.h.bh = konfig.seite;
  regs.h.dh = (uchar) y;
  regs.h.dl = (uchar) x;
  int86( 0x10, &regs, &regs );
  }

/*
**  Cursorgröße setzen
**  block : == 0 Cursor in Unterstreichform (Standartform)
**          != 0 Cursor in Blockform
*/
static void LOCAL setcrrsblock ( int block )
  {
  static const unsigned groesse  [4] = { 0x60c, 0x0b0c, 0x0307, 0x0607 };
  union REGS regs;
  regs.h.ah = 0x01;
  regs.x.cx = groesse[ ( konfig.mda ? 0 : 2 ) + ( block ? 0 : 1 ) ];
  int86( 0x10, &regs, &regs );
  }

/*
**  Bestimmt Anzahl der Zeilen auf dem Bildschirm
**  return : Anzahl der Zeilen
*/
static int LOCAL getlines ( void )
  {
  int count = 0;
  int x,y;
  setcrrspos( 0, 0 );
  do{
    putchar( '\n' );
    getcrrspos( &x, &y );
    ++count;
    }
    while( count == y );
  return count;
  }

/*
**  Bestimmt die aktuelle Konfiguration
**  die BIOS-Variable *scroffset sollte immer den Anfang des
**  Text-Bildschirmspeichers der aktuellen Seite enthalten, aber
**  bei dem Wisdom AT gibt es damit Probleme.
**  ptr : Zeiger auf die auszufüllende Struktur
*/
static void LOCAL getkonfig ( struct konfiguration* ptr )
  {
  union REGS regs;
  unsigned _far* addr_6845 = MK_FP( 0x0040, 0x0063 );
  unsigned _far* scroffset = MK_FP( 0x0040, 0x004e );
  regs.h.ah = 0x0f;
  int86( 0x10, &regs, &regs );
  ptr->seite = regs.h.bh;
  ptr->cpl = regs.h.ah;
  ptr->lps = getlines();
  ptr->mda = regs.h.al == 0x07;
  ptr->scr = MK_FP( *addr_6845 == 0x03b4 ? 0xb000 : 0xb800,
		    regs.h.bh ? *scroffset : 0x0000 );
  ptr->ok = 1;
  }

/*
**  Zeichen in den Bildschirmspeicher schreiben
**  x,y : Bildschirmposition Spalte, Zeile
**  c   : das Zeichen
**  a   : das Attribut/die Farbe des Zeichens
*/
static void LOCAL tovideo ( int x, int y, uchar c, int a )
  {
  unsigned data;
  if( c == 0x20 && a == EDITSTYLE )
    data = ( EDITSTYLE << 8 ) + 0x00fa;
  else
    data = ( a << 8 ) + (unsigned) c;
  konfig.scr[ x + y * konfig.cpl ] = data;
  }

/*
**  Tastatureingabe entgegennehmen
**  key0 : ASCII der Taste oder 0 bei erweitertem Tastaturcode
**  key1 : Scan-Code der Taste
*/
static void LOCAL getkey ( int* key0, int* key1 )
  {
  union REGS regs;
  regs.h.ah = 0x00;
  int86( 0x16, &regs, &regs );
  *key0 = regs.h.al;
  *key1 = regs.h.ah;
  }

/*******************************************************************/

/*
**  Editfeld komplett ausgeben
**  ptr  : Das Editierfeld
**  attr : Bildschirmattribut/Farbe der Zeichen
*/
static void LOCAL print ( const editdata* ptr, int attr )
  {
  const uchar* str = ptr->buffer;
  int i = ptr->len;
  int x = ptr->x;
  int y = ptr->y;
  while( --i > 0 ) tovideo( x++, y, *str++, attr );
  }

/*
**  Das Editierfeld auf dem Bildschirm löschen
**  ptr : Zeiger auf die Struktur mit den editdaten
*/
static void LOCAL clearprint ( const editdata* ptr )
  {
  int i = ptr->len;
  int x = ptr->x;
  int y = ptr->y;
  while( --i > 0 ) tovideo( x++, y, 0x20, NORMSTYLE );
  }

/*
**  Ein Zeichen in den String einfügen, den Rest nach rechts scheiben
**  ptr    : Zeiger auf die Struktur mit den editdaten
**  wo     : dort soll eingefügt werden
**  was    : das Zeichen soll eingefügt werden
**  return : neue rel. Cursorposition
*/
static int LOCAL einfuegen ( const editdata* ptr, int wo, int was )
  {
  const int end = ptr->len - 2;
  uchar* str = ptr->buffer;
  int next;
  if( wo < end )
    {
    int i;
    for( i = end; i > wo; --i )
      {
      str[i] = str[i-1];
      tovideo( ptr->x + i, ptr->y, str[i], EDITSTYLE );
      }
    next = wo + 1;
    setcrrspos( ptr->x + next, ptr->y );
    }
  else
    next = wo;
  str[wo] = (uchar) was;
  tovideo( ptr->x + wo, ptr->y, (uchar) was, EDITSTYLE );
  return next;
  }

/*
**  Ein neues Zeichen soll in den String geschrieben werden, das alte
**  Zeichen wird überschrieben, Cursor wird ggf. nach rechts bewegt
**  ptr    : Zeiger auf die Struktur mit den editdaten
**  wo     : die aktuelle rel. Cursorposition
**  was    : das Zeichen soll geschrieben werden
**  return : neue rel. Cursorposition
*/
static int LOCAL schreib ( const editdata* ptr, int wo, int was )
  {
  const int end = ptr->len - 2;
  uchar* str = ptr->buffer;
  str[wo] = (uchar) was;
  tovideo( ptr->x + wo, ptr->y, (uchar) was, EDITSTYLE );
  if( wo < end )
    {
    ++wo;
    setcrrspos( ptr->x + wo, ptr->y );
    }
  return wo;
  }

/*
**  Ein Zeichen aus dem String wird gelöscht, der Rest wird herangezogen
**  ptr : Zeiger auf die Struktur mit den editdaten
**  wo  : dort soll das Zeichen gelöscht werden
*/
static void LOCAL delete ( const editdata* ptr, int wo )
  {
  uchar* str = ptr->buffer;
  const int end = ptr->len - 1;
  int i;
  str[end] = 0x20;
  for( i = wo; i < end; ++i )
    {
    str[i] = str[i+1];
    tovideo( ptr->x + i, ptr->y, str[i], EDITSTYLE );
    }
  }

/*
**  Den String komplett löschen, Cursor neu positionieren
**  ptr    : Zeiger auf die Struktur mit den editdaten
**  return : neue rel. Cursorposition == 0
*/
static int LOCAL deleteall ( const editdata* ptr )
  {
  int i;
  const int end = ptr->len - 1;
  uchar* str = ptr->buffer;
  for( i = 0; i < end; ++i )
    {
    str[i] = 0x20;
    tovideo( ptr->x + i, ptr->y, 0x20, EDITSTYLE );
    }
  setcrrspos( ptr->x, ptr->y );
  return 0;
  }

/*
**  Löscht den String ab Cursorposition
**  ptr : Zeiger auf die Struktur mit den editdaten
**  pos : rel. Position des Cursors
*/
static void LOCAL deleterest ( const editdata* ptr, int pos )
  {
  const int end = ptr->len - 1;
  uchar* str = ptr->buffer;
  while( pos < end )
    {
    str[pos] = 0x20;
    tovideo( ptr->x + pos, ptr->y, 0x20, EDITSTYLE );
    ++pos;
    }
  }

/*
**  Das erste Zeichen im String wird gesucht, dorthin wird der Cursor gesetzt
**  ptr    : Zeiger auf die Struktur mit den editdaten
**  return : rel. Cursorposition auf das erste Zeichen
*/
static int LOCAL getfirstpos ( const editdata* ptr )
  {
  uchar* str = ptr->buffer;
  int i = 0;
  str[ptr->len-1] = 0x21;
  while( str[i] <= 0x20 ) ++i;
  if( i >= ptr->len - 1 ) i = 0;
  setcrrspos( ptr->x + i, ptr->y );
  return i;
  }

/*
**  Das letzte Zeichen im String wird gesucht, dorthin wird der Cursor
**  gesetzt
**  ptr    : Zeiger auf die Struktur mit den editdaten
**  return : rel. Cursorposition auf das letzte Zeichen
*/
static int LOCAL getlastpos ( const editdata* ptr )
  {
  const uchar* str = ptr->buffer;
  int i = ptr->len - 2;
  while( i > 0 && str[i] <= 0x20 ) --i;
  setcrrspos( ptr->x + i, ptr->y );
  return i;
  }

/*
**  Tastenbeschreibung ausgeben
**  ptr : Zeiger auf Sturktur mit den editdaten, die Startposition des
**        Felds wird verändert
*/
static void LOCAL hilfe ( editdata* ptr )
  {
  if( ! ptr->screen )
    {
    clearprint( ptr );
    puts( tasten );
    getcrrspos( &ptr->x, &ptr->y );
    print( ptr, EDITSTYLE );
    }
  }

/*
**  Editieren
**  data : Struktur mit allen benötigten Daten
*/
static void LOCAL edit ( editdata* data )
  {
  int weiter = 1;
  int pos = 0;
  int insert = 0;
  print( data, EDITSTYLE );
  setcrrsblock( 0 );
  setcrrspos( data->x, data->y );
  do{
    int key0, key1;
    getkey( &key0, &key1 );
    if( key0 )
      switch( key0 )
        {
        case CR    : weiter = 0;
                     break;
        case BS    : if( pos > 0 ) setcrrspos( data->x + --pos, data->y );
                     delete( data, pos );
                     break;
        case CTRLX : deleterest( data, pos );
                     break;
        case CTRLY : pos = deleteall( data );
                     break;
        default    : if( key0 >= 0x20 )
                       if( insert )
                         pos = einfuegen( data, pos, key0 );
                       else
                         pos = schreib( data, pos, key0 );
        }
    else
      switch( key1 )
        {
        case F1        : hilfe( data );
                         break;
        case RIGHT     : if( pos+2 < data->len ) setcrrspos( data->x + ++pos, data->y );
                         break;
        case END       :
        case CTRLRIGHT : pos = getlastpos( data );
                         break;
        case LEFT      : if( pos > 0 ) setcrrspos( data->x + --pos, data->y );
                         break;
        case HOME      : pos = 0;
                         setcrrspos( data->x, data->y );
                         break;
        case CTRLLEFT  : pos = getfirstpos( data );
                         break;
        case INSERT    : insert = ! insert;
                         setcrrsblock( insert );
                         break;
        case DELETE    : delete( data, pos );
                         break;
        }
    }
    while( weiter );
  print( data, NORMSTYLE );
  setcrrsblock( 0 );
  }

/*
**  ASCIZ String in editdata Struktur eintragen
**  str : der ASCIZ String
**  len : die Länge des Buffers
**  ptr : Zeiger auf die auszufüllende Struktur
*/
static void LOCAL toeditdata ( uchar* str, int len, editdata* ptr )
  {
  int i;
  ptr->buffer = str;
  ptr->len = len;
  getcrrspos( &ptr->x, &ptr->y );
  for( i = 0; i < len && str[i] != '\0'; ++i )
    if( str[i] < 0x20 ) str[i] = 0x20;
  while( i < len ) str[i++] = 0x20;
  }

/*
**  ASCIZ String aus editdata Struktur herauslösen
**  ptr    : Zeiger auf die Struktur
**  return : Zeiger auf den Buffer mit dem ASCIZ String
*/
static char* LOCAL fromeditdata ( const editdata* ptr )
  {
  int i = ptr->len - 1;
  uchar* str = ptr->buffer;
  while( i > 0 && str[i-1] <= 0x20 ) --i;
  str[i] = '\0';
  return (char*) str;
  }

/*
**  Editieren eines Strings auf dem Bildschirm
**  str : der String
**  len : Länge des Stringbuffers (mit \0 am Ende)
*/
void stredit ( char* str, int len )
  {
  editdata data;
  if( ! konfig.ok ) getkonfig( &konfig );
  toeditdata( (uchar*) str, len, &data );
  data.screen = 0;
  edit( &data );
  fromeditdata( &data );
  putchar( '\n' );
  }

/*
**  wie stredit, aber das Editfeld beginnt ab der angebenen Position
**  str : der String, zum Editieren
**  len : die Länge des Buffers
**  x   : Spalte des Editfelds (0..79)
**  y   : Zeile des Editfelds (0..24)
*/
void streditxy ( char* str, int len, int x, int y )
  {
  editdata data;
  if( ! konfig.ok ) getkonfig( &konfig );
  toeditdata( (uchar*) str, len, &data );
  data.x = x;
  data.y = y;
  data.screen = 1;
  edit( &data );
  fromeditdata( &data );
  }

/*
**  Bildschirm löschen
*/
void cls ( void )
  {
  int i;
  unsigned _far* ptr;
  if( ! konfig.ok ) getkonfig( &konfig );
  i = konfig.cpl * konfig.lps;
  ptr = konfig.scr;
  while( --i >= 0 ) ptr[i] = ' ' + ( NORMSTYLE << 8 );
  }

/*
**  eine Bildschirmzeile löschen
**  line : Nummer der Zeile 0..
*/
void cll ( int line )
  {
  int i;
  unsigned _far* ptr = konfig.scr + line * konfig.cpl;
  if( ! konfig.ok ) getkonfig( &konfig );
  for( i = 0; i < konfig.cpl; ++i )
    ptr[i] = ' ' + ( NORMSTYLE << 8 );
  }

/*
**  Einen String auf den Bildschirm ausgeben
**  x,y : die Startposition
**  str : der String
*/
void putsxy ( int x, int y, const char* str )
  {
  unsigned _far* ptr;
  if( ! konfig.ok ) getkonfig( &konfig );
  ptr = konfig.scr + x + y * konfig.cpl;
  while( *str != '\0' ) *ptr++ = (uchar) *str++ + ( NORMSTYLE << 8 );
  }

/*
**  Bildschirminhalt und Cursorposition abspeichern
**  ptr    : dort Buffer mit Bildschirminhalt zurückgeben, (wird angelegt)
**           NULL, falls ein Problem beim Abspeichern
**  x,y    : dort Cursor Position zurück
**  return : alles ok ?
*/
static int LOCAL savescreen ( unsigned** ptr, int* x, int* y )
  {
  unsigned* save;
  int count;
  if( ! konfig.ok ) getkonfig( &konfig );
  count = konfig.cpl * konfig.lps;
  *ptr = save = malloc( count * sizeof (unsigned) );
  if( save != NULL )
    {
    const unsigned _far* screen = konfig.scr;
    getcrrspos( x, y );
    setcrrspos( konfig.cpl, konfig.lps );
    while( --count >= 0 ) save[count] = screen[count];
    }
  return save != NULL;
  }

/*
**  Bildschirminhalt wiederherstellen und Cursor posititionieren
**  save : der gespeicherte Bildschirminhalt
**         der Speicher wird wieder freigegeben
**  x,y  : die Curosposition
*/
static void LOCAL restorescreen ( const unsigned* save, int x, int y )
  {
  int i = konfig.cpl * konfig.lps;
  unsigned _far* screen = konfig.scr;
  setcrrspos( x, y );
  while( --i >= 0 ) screen[i] = save[i];
  }

/*
**  Bildschirmschoner, wird deaktiviert durch einen Tastendruck
*/
void screensaver ( void )
  {
  int crrsx, crrsy;
  unsigned* screen;
  if( savescreen( &screen, &crrsx, &crrsy ) )
    {
    int x = 40;
    int y = 0;
    while( ! kbhit() )
      {
      tovideo( x, y, ' ', NORMSTYLE );
      if( ++y >= konfig.lps ) { y = 0; x = rand() % konfig.cpl; }
      tovideo( x, y, 'U', NORMSTYLE );
      }
    restorescreen( screen, crrsx, crrsy );
    }
  }
