/*
**  Zortech C V2.1
**  .04.1994  Ulrich Berntien
**
**  Modul zum Lesen und Interpretieren von Bitmap-Dateien
**  von Windows 3.X und Windows NT 3.X
**
**  16.04.1994  Beginn
**  07.05.1994  x_manual und x_schalter() ergänzt
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "x2img.h"
#include "bmp.h"

/********************************************************************/

/*
**  die verschiedenen Inhalt von BMP-Files
*/
typedef enum typesTAG
  {
  _1bit,          /* 1 Bit/Pixel Schwarz-Weiß-Bild                */
  _4bit,          /* 4 Bit/Pixel Farbbild, unkomprimiert          */
  _8bit,          /* 8 Bit/Pixel Farbbild, unkomprimiert          */
  _24bit,         /* 24 Bit/Pixel True-Color Farbbild             */
  _rle4,          /* 4 Bit/Pixel lauflängenkomprimiertes Farbbild */
  _rle8           /* 8 Bit/Pixel lauflängenkomprimiertes Farbbild */
  }
  types;

/*
**  Informationen zusammengefaßt, die für das Lesen aus der Bitmap
**  benötigt werden
*/
typedef struct infoTAG
  {
  types typ;           /* Typ des BMP-File Inhalts         */
  int   breite;        /* Breite des BMP-Bilds in Pixel    */
  int   hoehe;         /* Höhe des BMP-Files in Pixel      */
  int   farben;        /* Anzahl der Farben in der Tabelle */
  long  farbpos;       /* Position der Farbtabelle         */
  long  bitmappos;     /* Position der Bitmap im BMP-File  */
  }
  info;

/*
**  feste Offsets innerhalb des Bitmap-Files
*/
static const long offset_fileheader = 0L;
static const long offset_infoheader = sizeof (bmp_fileheader);

/*
**  Name des Programms
*/
const char *const x_name = "BMP2IMG";

/*
**  Standard Extension für die Files vom BMP-Typ
*/
const char *const x_extension = ".BMP";

/*
**  keine Ergänzung der Programm-Kurzbeschreibung nötig
*/
const char *const x_manual = NULL;

/*
**  Umsetzungstabelle Pixelwert -> Grauwert
**  (nicht bei True-Color (_24bit) BMP-Files)
**  wird aus der Farbtabelle des BMP-Files berechnet:
**  Grauwert = Rot * 0.30 + Grün * 0.59 + Blau * 0.11
*/
static BYTE grauwert [ 256 ];

/*
**  Die Faktoren der für die RGB-Werte (bezogen auf 256)
*/
#define FAKTOR_RED    77
#define FAKTOR_GREEN 151
#define FAKTOR_BLUE   28

/*
**  Daten über das BMP-File speichern
*/
static info data;

/********************************************************************/

/*
**  Einlesen und Auswerten des Bitmap-Fileheaders
*/
static void leseFileInfo ( void )
  {
  bmp_fileheader* header = ok_malloc( sizeof (bmp_fileheader) );
  in_readdirect( offset_fileheader, header, sizeof (bmp_fileheader) );
  if( header->typ != bmp_typ )
    err_message( "Falsche Typ-Kennung im Dateikopf." );
  if( (long) header->size != in_filelength() )
    err_message( "Falsche Angabe der Dateigröße im Dateikopf." );
  if( header->reserved != 0 )
    err_message( "Unbekannte Informationen im Dateikopf." );
  data.bitmappos = header->offsetBitmap;
  free( header );
  }

/*
**  Einlesen und Auswerten des Bitmap-Infoheaders
*/
static void leseBitmapInfo ( void )
  {
  bmp_infoheader* info = ok_malloc( sizeof (bmp_infoheader) );
  in_readdirect( offset_infoheader, info, sizeof (bmp_infoheader) );
  if( info->size < sizeof (bmp_infoheader) )
    err_message( "Fehlende Informationen im Infoblock." );
  if( info->size > sizeof (bmp_infoheader) )
    err_message( "Unbekannte Informationen im Infoblock." );
  if( info->width < 0L )
    err_abort( "Negative Breite der Bitmap im Infoblock angegeben." );
  if( info->width > (long) INT_MAX )
    err_abort( "Bitmaps breiter als %d Pixel können nicht bearbeitet werden.", INT_MAX );
  if( info->height < 0L )
    err_abort( "Negative Höhe der Bitmap im Infoblock angegeben." );
  if( info->height > (long) INT_MAX )
    err_abort( "Bitmaps höher als %d Pixel können nicht bearbeitet werden.", INT_MAX );
  if( info->planes != 1 )
    err_message( "Anzahl der Farbebenen != 1 im Infoblock angegeben." );
  switch( info->bitCount )
    {
    case  1 : data.typ = _1bit;
              break;
    case  4 : data.typ = _4bit;
              break;
    case  8 : data.typ = _8bit;
              break;
    case 24 : data.typ = _24bit;
              break;
    default : err_abort( "Bitmaps mit %d Bits/Pixel sind unbekannt." );
    }
  if( info->compression != bmp_rgb )
    switch( data.typ )
      {
      case _1bit  : err_abort( "Komprimierte Schwarz/Weiß-Bitmaps unbekannt." );
                    break;
      case _4bit  : data.typ = _rle4;
                    if( info->compression != bmp_rle4 )
                      err_message( "Unbekanntes Kompressionsverfahren, versuche RLE4" );
                    break;
      case _8bit  : data.typ = _rle8;
                    if( info->compression != bmp_rle8 )
                      err_message( "Unbekanntes Kompressionsverfahren, versuche RLE8" );
                    break;
      case _24bit : err_abort( "Komprimierte True-Color-Bitmaps unbekannt." );
                    break;
      }
  if( data.typ != _24bit && info->clrUsed > 256 )
    {
    err_message( "Mehr als 256 Farben in der Farbtabelle." );
    info->clrUsed = 256;
    }
  if( info->clrImportant > info->clrUsed )
    err_message( "Mehr wichtige Farben als benutzte Farben." );
  data.breite = info->width;
  data.hoehe = info->height;
  data.farben = info->clrUsed;
  data.farbpos = offset_infoheader + info->size;
  free( info );
  }

/*
**  Einlesen der Farbtabelle und Umwandeln in eine Grauwert-Tabelle
*/
static void leseFarbtabelle ( void )
  {
  const size_t size = data.farben * sizeof (bmp_rgbquad);
  bmp_rgbquad* rgb = ok_malloc( size  );
  long* grau = ok_malloc( data.farben * sizeof (long) );
  long max = 0L;
  int i;
  in_readdirect( data.farbpos, rgb, size );
  for( i = 0; i < 256; ++i ) grauwert[i] = (BYTE) i;
  for( i = 0; i < data.farben; ++i )
    {
    grau[i] = rgb[i].rgbRed   * (long) FAKTOR_RED   +
              rgb[i].rgbGreen * (long) FAKTOR_GREEN +
              rgb[i].rgbBlue  * (long) FAKTOR_BLUE  ;
    if( grau[i] > max ) max = grau[i];
    }
  if( max > 0L )
    {
    const long divisor = ( max + 254 )/ 255;
    for( i = 0; i < data.farben; ++i )
      grauwert[i] = (BYTE) ( grau[i] / divisor );
    }
  free( grau );
  free( rgb );
  }

/*
**  Lese eine Schwarz/Weiß-Bitmap
**  der Buffer des Input-Files ist bereits positioniert
*/
static void lese1bit ( void )
  {
  const int bpl = ( data.breite + 7 )/ 8;
  const int fuellung = ( 4 - ( bpl & 0x03 ) ) & 0x03;
  DWORD dummy;
  int zeile;
  puts( "2 Farben (1 Bit/Pixel)" );
  for( zeile = 0; zeile < data.hoehe; ++zeile )
    {
    int spalte = 0;
    while( spalte < data.breite )
      {
      BYTE akku = in_readbyte();
      int bits;
      for( bits = 0; bits < 8 && spalte < data.breite; ++bits, ++spalte )
        {
        img_pixel( ( akku & 0x80 ) ? grauwert[1] : grauwert[0] );
        akku <<= 1;
        }
      }
    in_read( &dummy, fuellung );
    }
  }

/*
**  Lese eine 16 Farben Bitmap im nicht komprimierten Format
**  der Buffer des Input-Files ist bereits positioniert
*/
static void lese4bit ( void )
  {
  const int bpl = ( data.breite + 1 )/ 2;
  const int fuellung = ( 4 - ( bpl & 0x03 ) ) & 0x03;
  DWORD dummy;
  int zeile;
  puts( "16 Farben (4 Bit/Pixel)" );
  for( zeile = 0; zeile < data.hoehe; ++zeile )
    {
    int spalte;
    for( spalte = 0; spalte < data.breite; spalte += 2 )
      {
      const BYTE akku = in_readbyte();
      img_pixel( grauwert[ akku >> 4 ] );
      if( spalte+1 < data.breite ) img_pixel( grauwert[ akku & 0x0F ] );
      }
    in_read( &dummy, fuellung );
    }
  }

/*
**  Lese eine 256 Farben Bitmap im nicht komprimierten Format
**  der Buffer des Input-Files ist bereits positioniert
*/
static void lese8bit ( void )
  {
  const int fuellung = ( 4 - ( data.breite & 0x03 ) ) & 0x03;
  DWORD dummy;
  int zeile;
  puts( "256 Farben (8 Bit/Pixel)" );
  for( zeile = 0; zeile < data.hoehe; ++zeile )
    {
    int spalte;
    for( spalte = 0; spalte < data.breite; ++spalte )
      img_pixel( grauwert[in_readbyte()] );
    in_read( &dummy, fuellung );
    }
  }

/*
**  Lese eine True-Color Bitmap
**  der Buffer des Input-Files ist bereits positioniert
*/
static void lese24bit ( void )
  {
  const long bpl = data.breite * 3L;
  const int fuellung = ( 4 - (int) ( bpl & 0x03L ) ) & 0x03;
  DWORD dummy;
  int zeile;
  puts( "True-Color (24 Bit/Pixel)" );
  for( zeile = 0; zeile < data.hoehe; ++zeile )
    {
    int spalte;
    for( spalte = 0; spalte < data.breite; ++spalte )
      {
      long grau = in_readbyte() * (int) FAKTOR_RED;
      grau += in_readbyte() * (int) FAKTOR_GREEN;
      grau += in_readbyte() * (int) FAKTOR_BLUE;
      img_pixel( (BYTE) ( grau / (FAKTOR_RED+FAKTOR_GREEN+FAKTOR_BLUE) ) );
      }
    in_read( &dummy, fuellung );
    }
  }

/*
**  Lese eine 16 Farben-Bitmap im Lauflängen komprimiertem Format
**  der Buffer des Input-Files ist bereits positioniert
*/
static void leseRle4 ( void )
  {
  int spalte = 0;
  int eob = 0;
  puts( "16 Farben (lauflängenkomprimiert)" );
  while( ! eob )
    {
    BYTE akku = in_readbyte();
    BYTE zaehler = akku >> 4;
    BYTE farbwert = akku & 0x0F;
    if( zaehler != 0 )
      {
      BYTE grau = grauwert[ farbwert ];
      do{
        if( spalte >= data.breite ) spalte = 0;
        img_pixel( grau );
        ++spalte;
        }
        while( --zaehler != 0 );
      }
    else
      switch( farbwert )
        {
        case 0 :  while( spalte < data.breite )
                    {
                    img_pixel( grauwert[0] );
                    ++spalte;
                    }
                  spalte = 0;
                  break;
        case 1  : eob = 1;
                  break;
        case 2  : {
                  BYTE rechts = in_readbyte();
                  BYTE unten = in_readbyte();
                  long count = (long) unten * (long) data.breite + (long) rechts;
                  while( --count >= 0L ) img_pixel( grauwert[0] );
                  spalte += rechts;
                  }
                  break;
        default : {
                  int fuellung;
                  akku = farbwert & 0x03;
                  fuellung = akku == 1 || akku == 2;
                  do{
                    akku = in_readbyte();
                    img_pixel( grauwert[ akku >> 4 ] );
                    if( ++spalte >= data.breite ) spalte = 0;
                    if( --farbwert > 0 )
                      {
                      if( spalte >= data.breite ) spalte = 0;
                      img_pixel( grauwert[ akku & 0x0F ] );
                      ++spalte;
                      }
                    }
                    while( --farbwert > 0 );
                  if( fuellung ) in_readbyte();
                  }
        }
    }
  }

/*
**  Lese eine 256 Farben-Bitmap im Lauflängen komprimiertem Format
**  der Buffer des Input-Files ist bereits positioniert
*/
static void leseRle8 ( void )
  {
  int spalte = 0;
  int eob = 0;
  puts( "256 Farben (lauflängenkomprimiert)" );
  while( ! eob )
    {
    BYTE zaehler = in_readbyte();
    BYTE farbwert = in_readbyte();
    if( zaehler != 0 )
      {
      BYTE grau = grauwert[ farbwert ];
      do{
        if( spalte >= data.breite ) spalte = 0;
        img_pixel( grau );
        ++spalte;
        }
        while( --zaehler > 0 );
      }
    else
      switch( farbwert )
        {
        case 0 :  while( spalte < data.breite )
                    {
                    img_pixel( grauwert[0] );
                    ++spalte;
                    }
                  spalte = 0;
                  break;
        case 1  : eob = 1;
                  break;
        case 2  : {
                  BYTE rechts = in_readbyte();
                  BYTE unten = in_readbyte();
                  long count = (long) unten * (long) data.breite + (long) rechts;
                  while( --count >= 0L ) img_pixel( grauwert[0] );
                  spalte += rechts;
                  }
                  break;
        default : {
                  const int fuellung = farbwert & 1;
                  do{
                    if( spalte >= data.breite ) spalte = 0;
                    img_pixel( grauwert[ in_readbyte() ] );
                    ++spalte;
                    }
                    while( --farbwert > 0 );
                  if( fuellung ) in_readbyte();
                  }
        }
    }
  }

/*
**  Einlesen und Interpretieren der Bitmap
*/
static void leseBitmap ( void )
  {
  in_newbuffer( data.bitmappos );
  img_start( data.breite, data.hoehe, 0 );
  printf( "Lese Bitmap (%d x %d) mit ", data.breite, data.hoehe );
  switch( data.typ )
    {
    case _1bit  : lese1bit();
                  break;
    case _4bit  : lese4bit();
                  break;
    case _8bit  : lese8bit();
                  break;
    case _24bit : lese24bit();
                  break;
    case _rle4  : leseRle4();
                  break;
    case _rle8  : leseRle8();
                  break;
    }
  }

/*
**  Lesen des Input-Files über die in_*() Funktionen und schreiben
**  des IMG-Files über die img_*() Funktionen
*/
void x_trafo ( void )
  {
  leseFileInfo();
  leseBitmapInfo();
  if( data.typ != _24bit ) leseFarbtabelle();
  leseBitmap();
  }

/*
**  Auswerten der Schalter speziell für diese Modul
**  sw     : der Schalter von der Kommandozeile, ohne das Schaltersysmbol
**  return : Konnte der Schalter ausgewertet werden ?
*/
int x_schalter ( const char* sw )
  {
  return 0;
  }
