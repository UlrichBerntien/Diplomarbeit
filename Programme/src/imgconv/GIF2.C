/*
**  Zortech C V2.1
**  .12.1994  Ulrich Berntien
**
**  Modul zum Lesen und Interpretieren von GIF-Dateien
**
**  03.12.1994  Beginn, entstanden aus TIFF2.C
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x2img.h"
#include "gif.h"

/********************************************************************/

/*
**  Name des Programms
*/
const char *const x_name = "GIF2IMG";

/*
**  Standard Extension für die Files vom TIF-Format
*/
const char *const x_extension = ".GIF";

/*
**  keine Erweiterung der Programm-Kurzbeschreibung
*/
const char *const x_manual = NULL;

/********************************************************************/

/*
**  Funktionen und Daten zur Auswertung/Interpretation der Bitmap
**  Von den Funktionen zur Dekomprimierung wird ein Strom von
**  Bytes erzeugt, diese Strom wird hier aufgenommen und in einen
**  Strom von Grauwerten für img_pixel() umgewandelt.
**
\********************************************************************/

/*
**  Die Faktoren der für die RGB-Werte (bezogen auf 256)
*/
#define FAKTOR_RED    77
#define FAKTOR_GREEN 151
#define FAKTOR_BLUE   28

/*
**  Umsetzungstabelle Pixelwert -> Grauwert
**  (nicht bei True-Color Files)
**  wird aus der Farbtabelle berechnet:
**  Grauwert = Rot * 0.30 + Grün * 0.59 + Blau * 0.11
*/
static BYTE interpret_grauwert [ 256 ];

/*
**  Anzahl der noch auszugebenden Pixel
*/
static long interpret_pixel;

/*
**  Über diesen Funktionszeiger erfolgt die Verteilung des
**  eingehenden Datenstroms aus der LZW-Dekomprimierung
**  auf die zugehörige Funktion.
*/
static void (*interpret) ( BYTE data );

/*
**  Anzahl der Datenbits pro LZW-Ausgabecode, wenn
**  ungleich 8 und ungleich Bit pro Pixel
*/
static short interpret_lzwSize;

/*
**  Verteilung der Bytestroms auf die Funktion, die diesen Byte-Strom
**  in einen Pixel-Strom umformt.
*/
static void (*interpret2) ( BYTE data );

/*
**  Initialisieren der Ausgabe eines Bildes
**  spalten : die Breite des Bilds in Pixel
**  zeilen  : die Höhe des Bilds in Pixel
*/
static void interpret_init ( int spalten, int zeilen )
  {
  interpret_pixel = (long) zeilen * spalten;
  }

/*
**  Den Datenstrom aus der LZW-Dekompriemierung einen Byte-Strom formen
**  data : Ausgabe-Datum aus der LZW-Dekomprimierung
*/
static void interpret_byte ( BYTE data )
  {
  static WORD buffer = 0;
  static short bits = 0;
  buffer |= (WORD) data << bits;
  bits += interpret_lzwSize;
  if( bits > 7 )
    {
    (*interpret2)( (BYTE) buffer );
    buffer >>= 8;
    bits -= 8;
    }
  }

/*
**  Verarbeitet ein Farbpaletten-Bild mit 1 Bit pro Pixel
**  benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap
*/
static void interpret_1bit ( BYTE data )
  {
  const short count = interpret_pixel < 8L ? (short) interpret_pixel : 8;
  short i;
  for( i = 0; i < count; ++i )
    {
    img_pixel( interpret_grauwert[ (data & 0x80) ? 1 : 0 ] );
    data <<= 1;
    }
  interpret_pixel -= count;
  }

/*
**  Verarbeitet ein Farbpaletten-Bild mit 2 Bits pro Pixel
**  benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap
*/
static void interpret_2bits ( BYTE data )
  {
  const short count = interpret_pixel < 4L ? (short) interpret_pixel : 4;
  BYTE b[4];
  short i;
  for( i = 0; i < count; ++i )
    {
    b[i] = data & 0x03;
    data >>= 2;
    }
  for( i = count-1; i >= 0; --i )
    img_pixel( interpret_grauwert[ b[i] ] );
  interpret_pixel -= count;
  }

/*
**  Verarbeitet ein Farbpaletten-Bild mit 4 Bits pro Pixel
**  benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap
*/
static void interpret_4bits ( BYTE data )
  {
  if( interpret_pixel > 0L )
    {
    img_pixel( interpret_grauwert[ data >> 4 ] );
    --interpret_pixel;
    if( interpret_pixel > 0L )
      {
      img_pixel( interpret_grauwert[ data & 0x0F ] );
      --interpret_pixel;
      }
    }
  }

/*
**  Verarbeitet ein Farbpaletten-Bild mit 8 Bits pro Pixel
**  oder mit LZW-Datenbits = Bits pro Pixel
**  benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap oder aus LZW-Dekomprimierung
*/
static void interpret_8bits ( BYTE data )
  {
  if( interpret_pixel > 0L )
    {
    img_pixel( interpret_grauwert[data] );
    --interpret_pixel;
    }
  }

/*
**  Setzt die benötigete Auswertfunktion in "interpret"
**  bits : Anzahl der Bits pro Pixel
**  data : Anzahl der Bits aus der LZW-Dekomprimierung
**         max. 8 Bit erlaubt
*/
static void interpret_set ( short bits, short data )
  {
  if( data == bits )
    interpret = interpret_8bits;
  else
    {
    switch( bits )
      {
      case 1  : interpret = interpret_1bit;
                break;
      case 2  : interpret = interpret_2bits;
                break;
      case 4  : interpret = interpret_4bits;
                break;
      case 8  : interpret = interpret_8bits;
                break;
      default : err_abort( "%d Bits/Pixel werden nicht unterstützt.", bits );
      }
    if( data != 8 )
      {
      interpret2 = interpret;
      interpret = interpret_byte;
      interpret_lzwSize = data;
      }
    }
  }

/*
**  Löschen der Farbtabelle, Graustufenkeil eintragen
*/
static void clearFarbtabelle ( void )
  {
  short i;
  for( i = 0; i < 256; ++i ) interpret_grauwert[i] = (BYTE) i;
  }

/*
**  Einlesen der Farbtabelle (lokale oder globale)
**  (der Lesenbuffer von x2img muß eingestellt sein)
**  farben : Anzahl der Farbstufen in der Tabelle (max. 256)
*/
static void readFarbtabelle ( short farben )
  {
  short i;
  for( i = 0; i < farben; ++i )
    {
    gif_rgb rgb;
    long akku;
    in_read( &rgb, sizeof (gif_rgb) );
    akku =   (long) rgb.rgbRed   * FAKTOR_RED
           + (long) rgb.rgbGreen * FAKTOR_GREEN
           + (long) rgb.rgbBlue  * FAKTOR_BLUE;
    interpret_grauwert[i] = (BYTE) (akku/(FAKTOR_RED+FAKTOR_GREEN+FAKTOR_BLUE) );
    }
  for( i = farben; i < 256; ++i ) interpret_grauwert[i] = 0;
  }

/*
**  Lesen und Auswerten von Verwaltungsinformationen aus dem
**  GIF-File
**
\********************************************************************/

/*
**  Lesen des Screen Descriptors und ggf. der globalen Farbtabelle
**  (der Lesebuffer von x2img muß eingestellt sein)
**  return : die Anzahl der Bits pro Pixel
*/
static short readHeader ( void )
  {
  gif_header head;
  gif_resolution resolution;
  in_read( &head, sizeof head );
  memcpy( &resolution, &head.flag, 1 );
  if(    memcmp( head.signatur, gif_signatur, sizeof head.signatur )
      && memcmp( head.signatur, gif_signaturAlt, sizeof head.signatur ) )
    err_message( "Das Eingabefile hat keine bekannte GIF-Signatur." );
  if( resolution.colormap )
    readFarbtabelle( 1 << resolution.colors+1 );
  return resolution.pixel + 1;
  }

/*
**  Überlesen eines Erweiterungsblocks
**  Der Lesebuffer von x2img muß eingestellt sein, der Separator ist
**  bereits gelesen.
*/
static void readExtBlock ( void )
  {
  BYTE fktCode;
  BYTE count;
  BYTE* dummy = ok_malloc( 256 );
  fktCode = in_readbyte();
  while( ( count = in_readbyte() ) != 0 ) in_read( dummy, (size_t) count );
  free( dummy );
  }

/*
**  Lesen des Image Descriptors und ggf. der lokalen Farbtabelle
**  Der Lesebuffer von x2img muß eingestellt sein, der Separator ist
**  bereits gelesen.
*/
static void readImageDescriptor ( void )
  {
  gif_image_descriptor head;
  gif_image_flags flags;
  in_read( (void*) &head + 1, sizeof head - 1 );
  memcpy( &flags, &head.flags, 1 );
  if( head.left != 0 || head.top != 0 )
    err_message( "Ignoriere die Position des Teilbilds" );
  if( head.width > (WORD) INT_MAX )
    err_abort( "Das Bild hat zuviele Spalten (%u).", head.width );
  if( head.height > (WORD) INT_MAX )
    err_abort( "Das Bild hat zuviele Zeilen (%u).", head.height );
  if( flags.interlaced )
    err_abort( "Interlaced Bilder werden nicht unterstützt." );
  if( flags.colormap )
    readFarbtabelle( 1 << flags.colors+1 );
  printf( "Lese GIF-File (%u x %u)\n", head.width, head.height );
  img_start( (int) head.width, (int) head.height, 1 );
  interpret_init( (int) head.width, (int) head.height );
  }

/*
**  Anzahl der restlichen Bytes in dem aktuellen Block der Bilddaten
*/
static short imageRest = 0;

/*
**  Lesen eines Bytes aus dem aktuellen Block der Bilddaten
**  return : ein Datenbyte
*/
static BYTE readCodeByte ( void )
  {
  if( imageRest == 0 )
    imageRest = (short) in_readbyte();
  --imageRest;
  if( imageRest < 0 )
    {
    /* restliche Bits aus dem Buffer in lzw_lese() ausspülen */
    if( --imageRest < -4 )
      err_abort( "Unerwartetes Ende der LZW-Daten." );
    return 0;
    }
  return in_readbyte();
  }

/*
**  Überlesen des Rests der Bilddaten
*/
static void readRest ( void )
  {
  if( imageRest > 0 )
    {
    BYTE* dummy = ok_malloc( 256 );
    err_message( "Ignoriere Bytes hinter dem Ende der LZW-Daten." );
    while( imageRest != 0 )
      {
      in_read( dummy, imageRest );
      imageRest = (short) in_readbyte();
      }
    free( dummy );
    }
  }

/*
**  Funktionen und Daten zur Dekomprimierung bei LZW-Kompression
**
\********************************************************************/

/*
**  Die LZW-Codes im GIF-Format sind auf 13 Bits beschränkt, also:
*/
#define lzw_gif_count 4095

/*
**  kein gültiger LZW-Code
*/
#define lzw_nocode 0xFFFF

/*
**  Spezieller LZW-Code von GIF : löschen der LZW-Code-Tabelle
**  = 2 hoch data_size
*/
static WORD lzw_gif_clear;

/*
**  Spezieller LZW-Code von GIF : Ende der Daten
**  = 2 hoch data_size + 1
*/
static  WORD lzw_gif_end;

/*
**  Ab hier werden die neuen LZW-Codes vergeben
**  = 2 hoch data_size + 2
*/
static WORD lzw_gif_neu;

/*
**  Maximale Anzahl der LZW-Codes die verarbeitet werden können
**  (lzw_tiff_count wird von dem TIF-Format gefordert)
*/
#define lzw_limit ( ( lzw_gif_count + 1023 ) & ~1023 )

/*
**  Masken für die 8 .. 14 niederwertigen Bits
*/
static WORD lzw_maske [] =
  {
  0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF
  };

/*
**  Hier wird die Codetabelle für die LZW-Codes aufgebaut.
**  lzw_suffix[] ist dabei wieder ein Index in der Tabelle.
*/
static WORD lzw_suffix [ lzw_limit ];
static BYTE lzw_praefix [ lzw_limit ];

/*
**  Datengröße für die LZW-Kompression
*/
static short lzw_data_size;

/*
**  Anzahl der Bits im nächsten zu lesenden LZW-Code
*/
static short lzw_bits;

/*
**  Der aktuelle LZW-Suffix Code.
**  Ein Index in der lzw_suffix und lzw_praefix Tabelle.
*/
static WORD lzw_muster;

/*
**  naechster Code der zu vergeben ist
**  (Index in den lzw_praefix und lzw_suffix Tabellen)
*/
static WORD lzw_nextcode;

/*
**  Stack in dem der Code nach dem LZW-Verfahren zusammengesetzt wird
*/
static BYTE lzw_stack [ lzw_limit ];

/*
**  Reset der LZW-Dekomprimierung
**  alle globalen lzw_* Variablen werden gesetzt
*/
static void lzw_clear ( void )
  {
  lzw_nextcode = lzw_gif_neu;
  lzw_bits = lzw_data_size + 1;
  lzw_muster = lzw_nocode;
  }

/*
**  Setzt globale Variablen, die während der Laufzeit konstant
**  bleiben und löst eine Clear der LZW-Dekodierung aus
**  data_size : die LZW-Datengröße beim Start
*/
static void lzw_init ( short data_size )
  {
  WORD i;
  lzw_data_size = data_size;
  lzw_gif_clear = 1 << data_size;
  lzw_gif_end = lzw_gif_clear + 1;
  lzw_gif_neu = lzw_gif_clear + 2;
  for( i = 0; i < lzw_gif_neu; ++i ) lzw_praefix[i] = (BYTE) i;
  memset( lzw_suffix, 0xFF, 256 * sizeof (WORD) );
  lzw_clear();
  }

/*
**  Einen LZW-Code auswerten, die dekodierten Bytes an 'interpret' übergeben
**  (globale lzw_* Variablen werden benutzt)
**  code : LZW-Codewort
*/
static void lzw_decode ( WORD code )
  {
  static char firstChar;
  if( code == lzw_gif_clear )
    lzw_clear();
  else if( code > lzw_nextcode )
    err_abort( "Ungültiger LZW-Code (0x%04X).", code );
  else if( code != lzw_gif_end )
    {
    short sp = 0;
    WORD index = code;
    if( index == lzw_nextcode )
      {
      lzw_stack[sp++] = firstChar;
      index = lzw_muster;
      }
    while( index != lzw_nocode )
      {
      lzw_stack[sp++] = lzw_praefix[index];
      index = lzw_suffix[index];
      }
    firstChar = lzw_stack[sp-1];
    while( sp > 0 ) (*interpret)( lzw_stack[--sp] );
    if( lzw_muster != lzw_nocode )
      {
      lzw_praefix[lzw_nextcode] = firstChar;
      lzw_suffix[lzw_nextcode] = lzw_muster;
      ++lzw_nextcode;
      if( lzw_nextcode > lzw_maske[lzw_bits-8] && lzw_nextcode < lzw_gif_count )
        ++lzw_bits;
      if( lzw_nextcode >= lzw_limit )
        err_abort( "Ungültiger LZW-Code, Clear-Code wurde erwartet" );
      }
    lzw_muster = code;
    }
  }

/*
**  Datenstruktur wird in lzw_lese benutzt um die Codes
**  mit variabler Länge aus dem Bytestrom zu lösen
*/
union lzw_akku
  {
  DWORD d;
  BYTE b[4];
  };

/*
**  Einlesen und dekomprimieren mit dem LZW-Verfahren
*/
static void lzw_lese ( void )
  {
  WORD code = lzw_nocode;
  short bitposition = 24;
  union lzw_akku akku;
  while( code != lzw_gif_end )
    {
    while( bitposition > 7 )
      {
      akku.b[0] = akku.b[1];
      akku.b[1] = akku.b[2];
      akku.b[2] = readCodeByte();
      bitposition -= 8;
      }
    code = (WORD) ( akku.d >> bitposition ) & lzw_maske[lzw_bits-8];
    bitposition += lzw_bits;
    lzw_decode( code );
    }
  }

/*
**  allgemeine Steuerfunktionen für die Umwandlung
**
\********************************************************************/

/*
**  Lesen eines Teilbilds
**  Der Lesebuffer von x2img muß eingestellt sein, der Separator ist
**  berseits gelesen.
**  bitsProPixel : Anzahl der Bits pro Pixel
*/
static void readImage ( short bitsProPixel )
  {
  static int count = 0;
  short codeSize;
  if( ++count > 1 )
    err_abort( "Es wurde nur das erste Teilbild aus dem GIF-File gelesen." );
  readImageDescriptor();
  codeSize = (int) in_readbyte();
  if( codeSize > 8 ) err_abort( "LZW-Datenlänge nur bis 8 Bit unterstützt" );
  interpret_set( bitsProPixel, codeSize );
  lzw_init( codeSize );
  lzw_lese();
  if( interpret == interpret_byte )
    {
    /* Restliche Bits aus dem Buffer in interpret_byte() ausspülen */
    short i = 0;
    while( i < 16 )
      {
      (*interpret)( 0 );
      i += codeSize;
      }
    }
  readRest();
  }

/*
**  Lesen des Input-Files über die in_*() Funktionen und schreiben
**  des IMG-Files über die img_*() Funktionen
*/
void x_trafo ( void )
  {
  BYTE separator;
  short bitsProPixel;
  in_newbuffer( 0L );
  clearFarbtabelle();
  bitsProPixel = readHeader();
  while( ( separator = in_readbyte() ) != gif_terminator )
    if( separator == gif_ext_sep )
      readExtBlock();
    else if( separator == gif_image_sep )
      readImage( bitsProPixel );
    else
      err_abort( "Unbekannter Blocktrenner '0x%02X' im GIF-File.", separator );
  }

/*
**  Auswerten der Schalter speziell für dieses Modul
**  sw     : der Schalter von der Kommandozeile, ohne das Schaltersymbol
**  return : Konnte der Schalter ausgewertet werden ?
*/
int x_schalter ( const char* sw )
  {
  return 0;
  }
