/*
**  C ( Microsoft C V5.10 / Zortech C V2.1 / Turbo C V2.0 )
**
**  Ulrich Berntien .02.1998
**
**  Umschalten zwischen verschiedenen Drucker für die Ausgabe von
**  Text und Grafik
**
**  03.02.1998  Beginn
*/

#include <string.h>
#include <stdio.h>

#include "printer.h"

/********************************************************************/

#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#define LOCAL _near _pascal

/*
**  ASCII Codes
*/
#define ESC "\x1B"
#define FS  "\x1C"

/********************************************************************/

/*
**  der aktuelle Druckertyp
*/
static enum printer_t printer = _hpLaserjet;

/*
**  intern genutzte Codes für die Druckeransteuerung
*/
enum printer_ic
  {
  _i_stripeStart,     /* Anfang eines Grafikstreifens   */
  _i_stripeEnd,       /* Ende des Grafikstreifen        */
  _i_codesCount       /* Anzahl der Steuercodes         */
  };

/*
**  Codes für Laängeninformation in den Grafikstreifen
*/
enum printer_stripeLen
  {
  _s_fprintf,              /* über fprintf ausgeben        */
  _s_binLHAppend           /* Binär anhängen Low,High Byte */
  };

/*
**  Informationen über einen Druckertyp in dieser
**  Datenstruktur zusammenfassen
*/
struct printerdata
  {
  const char* name;                   /* Names des Druckers               */
  const char* kurz;                   /* Kurzbezeichnung                  */
  const char* codes [_codesCount];    /* Strings mit Steuercodes          */
  enum printer_stripeLen stripeLen;   /* Code für Streifenlängenformat    */
  enum printer_g graphicMode;         /* Code für das Grafikformat        */
  const char* icodes [_i_codesCount]; /* Strings mit internen Steuercodes */
  };

/*
**  Informationen zum HP-Laserjet
*/
static struct printerdata hplaserjet =
  {
  "HP-Laserjet",
  "HP",
  { ESC "E",                   /* reset                     */
    ESC "(s3B",                /* bold on                   */
    ESC "(s0B",                /* bold off                  */
    ESC "(10U",                /* IBM-PC Zeichensatz        */
    "\f",                      /* Anfang neue Seite         */
    "\r\n",                    /* Anfang neue Zeile         */   
    ESC "*t300R" ESC "*r1A",   /* Grafikmode an (300dpi)    */
    ESC "*rB" "\r\n"           /* Grafikmode aus            */
    },
  _s_fprintf,                  /* Streifenlänge über fprintf             */
  _bitZeileL,                  /* 1 Pixel hoher Streifen, Bit 0x80 links */
  {
    ESC "*b%dW",               /* Anfang eines Grafikstreifens */
    ""                         /* Ende eines Grafikstreifen    */
    }
  };

/*
**  Informationen zum NEC Pinwriter Series 2200
*/
static struct printerdata necpinwriter2200 =
  {
  "NEC Prinwriter Series 2200",
  "NEC",
  { ESC "@",                   /* reset                              */
    ESC "G",                   /* bold on                            */
    ESC "H",                   /* bold off                           */
    ESC "t\x01",               /* IBM-PC Zeichensatz                 */
    "\f",                      /* Anfang neue Seite                  */
    "\r\n",                    /* Anfang neue Zeile                  */   
    ESC "3\x18",               /* Grafikmode an (24/180inch spacing) */
    ESC "2" ESC "J\x0c"        /* Grafikmode aus (6 lines/inch)      */
                               /*         und 12/180 inch freilassen */
    },
  _s_binLHAppend,              /* Streifenlänge binär anhängen          */
  _24bitZeileO,                /* 24 Pixel Streifenhöhe, Bit 0x80 oben  */
  {
    ESC "*\x27",               /* Anfang eines Grafikstreifens (180dpi) */
    "\r\n"                     /* Ende eines Grafikstreifen             */
    }
  };

/*
**  Array mit allen bekannten Druckern
*/
static const struct printerdata *const printers [_printerCount] =
  {
  &hplaserjet,
  &necpinwriter2200
  };

/********************************************************************/

/*
**  Fehlermeldung ausgegeben
**  msg : String mit der Fehlermeldung
*/
static void LOCAL error ( const char* msg )
   {
   fprintf( stderr, "PRINTER: %s.\n", msg );
   }

/*
**  Setzten des Druckertyps für die weitere Ausgabe
**  prn     : diesen Drucker benutzten
**  return  : die alte Einstellung
*/
enum printer_t printer_set ( const enum printer_t prn )
  {
  const enum printer_t result = printer;
  if( prn <= _noPrinter || prn >= _printerCount )
    error( "Ungültiger Printercode" );
  else
    printer = prn;
  return result;
  }

/*
**  Erfragen des aktuellen Druckertyps
**  return : aktuelle Einstellung
*/
enum printer_t printer_get ( void )
  {
  return printer;
  }

/*
**  Erfragen des Grafikformats des aktuellen Druckers
**  return : Das Grafikformat
*/
enum printer_g printer_graphic ( void )
  {
  return printers[printer]->graphicMode;
  }

/*
**  Suchen des Printertys
**  kurz   : Kurzbezeichnung des Typs
**  return : der Printertyp oder _none
*/
enum printer_t printer_search ( const char* kurz )
  {
  int i = _printerCount - 1;
  while( i > _noPrinter && strcmp( kurz, printers[i]->kurz ) != 0 ) --i;
  return i;
  }

/*
**  Ausgeben des Zeichencodes für den Drucker 
**  code  : gewünschter Steuercode
**  ofile : in diese File schreiben
*/
void printer_code ( const enum pinter_c code, FILE *const ofile )
  {
  if( code < 0 || code >= _codesCount )
    error( "Ungültiger Steurcode ausgegeben" );
  else
    fputs( printers[printer]->codes[code], ofile );
  }

/*
**  Einen Grafikstreifen ausgeben
**  buffer : die Daten
**  len    : Anzahl der Bytes in den Daten
**  ofile  : in dieses File schreiben
**  return : != 0, wenn alles ok
*/
int printer_stripeout ( const char* buffer, const int len, FILE *const ofile )
  {
  int ok = 1;
  int x;
  switch( printers[printer]->stripeLen )
    {
    case _s_fprintf     : fprintf( ofile, printers[printer]->icodes[_i_stripeStart], len );
                          break;
    case _s_binLHAppend : fputs( printers[printer]->icodes[_i_stripeStart], ofile );
                          x = len / 3; /* Anzahl der Grafik Spalten */
                          fputc( (char) (x % 256 ), ofile );
                          fputc( (char) (x / 256 ), ofile );
                          break;
    default             : ok = 0;
    }
  if( ok ) ok = fwrite( buffer, len, 1, ofile ) == 1;
  if( ok ) ok = fputs( printers[printer]->icodes[_i_stripeEnd], ofile ) != -1;
  return ok;
  }
