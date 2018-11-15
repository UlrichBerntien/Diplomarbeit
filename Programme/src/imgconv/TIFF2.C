/*
**  Zortech C V2.1
**  .05.1994  Ulrich Berntien
**
**  Modul zum Lesen und Interpretieren von TIFF-Dateien
**
**  03.05.1994  Beginn, entstanden aus dem Rumpf von PCX2.C
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x2img.h"
#include "tiff.h"

/********************************************************************/

/*
**  Name des Programms
*/
const char *const x_name = "TIFF2IMG";

/*
**  Standard Extension für die Files vom TIF-Format
*/
const char *const x_extension = ".TIF";

/*
**  Erweiterung der Programm-Kurzbeschreibung
*/
const char *const x_manual =
  "  -clearoff  Nach dem Dekodieren eines Steifens des Bilds wird\n"
  "         normalerweise der LZW-Dekoder zurückgesetzt. Dieser\n"
  "         Schalter verhindert diese Clear-Operation. Kann\n"
  "         versucht werden, wenn bei der LZW-Dekomprimierung\n"
  "         ein Fehler aufgetreten ist.\n"
  "";

/*
**  Funktionen und Daten zum Lesen der benötigten Informationen aus
**  dem TIFF-Fileheader und dem erstem Image File Directory (IFD)
**  des TIFF-Files
**
\********************************************************************/

/*
**  In dieser Struktur wird die Information aus einem tiff_entry
**  abgelgt, wenn der tiff_entry nur einen Wert enthält.
**  Alle Werte in dieser Struktur werdem im Intel-Form gespeichert.
**  (immer in DWORD, weil alle in diesem Programm ausgewerteten
**   Daten in TIFF-File als BYTE,WORD oder DWORD abgelegt sind)
*/
typedef struct einzelInfoTAG
  {
  short tag;            /* Identifikationsnummer laut TIFF   */
  short ok;             /* Wurde die Information gelesen ?   */
  short muss;           /* Ist die Information immer nötig ? */
  DWORD data;           /* Daten aus dem TIFF-File           */
  const char* name;     /* Name des Feldes (für Meldungen)   */
  }
  einzelInfo;

/*
**  Indices zu dem Array 'einzelData'
*/
enum einzelIndex
  {
  _IimageWidth,             /* Breite des Bilds in Pixel            */
  _IimageLength,            /* Höhe des Bilds in Pixel              */
  _Icompression,            /* Art der Kompression                  */
  _IimagePredictor,         /* Differencing Predictor benutzt ?     */
  _IimagePhotometric,       /* Art der Bitmap                       */
  _IsamplesPerPixel,        /* Anzahl der Werte pro Bildpunkt       */
  _IrowsPerStrip,           /* Anzahl der Zeilen pro Streifen       */
  _IeinzelCount             /* Anzahl der Einträge, immer am Ende ! */
  };

/*
**  Hier werden die benötigeten Daten aus dem TIFF-File gesammelt, wenn
**  es sich nicht um Arrays handelt
*/
static einzelInfo einzelData [ _IeinzelCount ] =
  {
  _TAGimageWidth,      0, 1, 0L, "ImageWidth",
  _TAGimageLength,     0, 1, 0L, "ImageLength",
  _TAGcompression,     0, 1, 0L, "Compression",
  _TAGpredictor,       0, 0, 0L, "Predictor",
  _TAGphotometric,     0, 1, 0L, "Photometric",
  _TAGsamplesPerPixel, 0, 0, 0L, "SamplesPerPixel",
  _TAGrowsPerStrip,    0, 1, 0L, "RowsPerStrip",
  };

/*
**  In dieser Struktur werden die Daten aus einem tiff_entry
**  abgelegt, wenn der tiff_entry ein Array von Werte enthält.
**  Alle Werte in dieser Struktur werdem in Intel-Format gespeichert.
*/
typedef struct arrayInfoTAG
  {
  short tag;            /* Identifiaktionsnummer laut TIFF     */
  int count;            /* Anzahl der Elemente im Array        */
  short muss;           /* Ist die Information immer nötig ?   */
  DWORD* pdata;         /* Daten aus dem TIFF-File, falls      */
                        /* NULL wurden die Daten nicht gelesen */
  const char* name;     /* Name des Feldes (für Meldungen)     */
  }
  arrayInfo;

/*
**  Indices zu dem Array 'arrayData'
*/
enum arrayIndex
  {
  _IbitsPerSample,          /* Bits pro Sample                      */
  _IstripOffset,            /* Positionen der Streifen im File      */
  _IstripByteCount,         /* Größe der Steifen in Bytes           */
  _IcolorMap,               /* Die Farbtabelle                      */
  _IarrayCount              /* Anzahl der Einträge, immer am Ende ! */
  };

/*
**  Hier werden die benötigten Daten aus dem TIFF-File gesammelt, wenn
**  es sich um Arrays handelt
*/
static arrayInfo arrayData [ _IarrayCount ] =
  {
  _TAGbitsPerSample,    0, 1, NULL, "BitsPerSample",
  _TAGstripOffset,      0, 1, NULL, "StripOffset",
  _TAGstripByteCount,   0, 1, NULL, "StripByteCount",
  _TAGcolorMap,         0, 0, NULL, "ColorMap",
  };

/*
**  Sind die Zahlen im TIFF-File im Intel-Format gespeichert ?
**  Intel: Byte an höherer Adresse hat den höheren Wert (little endian)
**  sonst: Byte an höherer Adresse hat den kleineren Wert (big endian)
*/
static short intelFormat;

/*
**  Lesen von Daten aus dem TIFF-File in einen dafür anzulegenden
**  Buffer auf dem Heap
**  position : Adresse rel. zum Fileanfang
**  size     : Anzahl der zu lesenden Bytes
**  return   : Die gelesenen Daten (free nicht vergessen)
*/
static void* loadData ( const DWORD position, const size_t size )
  {
  void *const buffer = ok_malloc( size );
  if( position & (DWORD) 1 )
    err_message( "Daten sind nicht auf WORD-Grenze ausgerichtet." );
  if( position > LONG_MAX )
    err_abort( "Zugriff auf Adresse im File > 2 GByte." );
  if( position > in_filelength() - size )
    err_abort( "Es wird über das Fileende gelessen." );
  in_readdirect( position, buffer, size );
  return buffer;
  }

/*
**  Ein short aus dem TIFF-File wird in ein Short im Intel-Format
**  umgewandelt (gemäß Variable intelFormat)
**  ptr    : auf Puffer mit dem short aus dem TIFF-Format
**  result : short im Intel-Format
*/
static short short2short ( const void *const ptr )
  {
  if( intelFormat )
    return * (const short*) ptr;
  else
    {
    const BYTE *const bptr = ptr;
    BYTE result [2];
    result[0] = bptr[1];
    result[1] = bptr[0];
    return (short) result;
    }
  }

/*
**  Ein Word aus dem TIFF-File wird in ein Word im Intel-Format
**  umgewandelt (gemäß Variable intelFormat)
**  ptr    : auf Puffer mit dem Word aus dem TIFF-File
**  return : Word im Intel-Format
*/
static WORD word2word ( const void *const ptr )
  {
  if( intelFormat )
    return * (const WORD *const) ptr;
  else
    {
    const BYTE *const bptr = ptr;
    BYTE result [2];
    result[0] = bptr[1];
    result[1] = bptr[0];
    return (WORD) result;
    }
  }

/*
**  Ein DWord aus dem TIFF-File wird in ein Word im Intel-Format
**  umgewandelt (gemäß Variable intelFormat)
**  ptr    : auf Puffer mit dem DWord aus dem TIFF-File
**  return : DWord im Intel-Format
*/
static DWORD dword2dword ( const void *const ptr )
  {
  if( intelFormat )
    return * (const DWORD *const) ptr;
  else
    {
    const BYTE *const bptr = ptr;
    BYTE result [4];
    result[3] = bptr[0];
    result[2] = bptr[1];
    result[1] = bptr[2];
    result[0] = bptr[3];
    return (DWORD) result;
    }
  }

/*
**  Ein Word aus dem TIFF-File wird in ein DWord im Intel-Format
**  umgewandelt (gemäß Varaiable intelFormat)
**  ptr    : auf Puffer mit dem Word aus dem TIFF-File
**  return : DWord im Intel-Format
*/
static DWORD word2dword ( const void *const ptr )
  {
  if( intelFormat )
    return (DWORD) * (const WORD *const) ptr;
  else
    {
    const BYTE *const bptr = ptr;
    BYTE result [4];
    result[3] = 0;
    result[2] = 0;
    result[1] = bptr[0];
    result[0] = bptr[1];
    return (DWORD) result;
    }
  }

/*
**  Ein Byte aus dem TIFF-File wird in ein DWord im Intel-Format
**  umgewandelt (ist unabhängig von Varaibale intelFormat)
**  ptr    : auf den Puffer mit dem Byte
**  return : DWord im Intel-Format
*/
static DWORD byte2dword ( const void *const ptr )
  {
  return (DWORD) * (const BYTE *const) ptr;
  }

/*
**  Untersucht ob der gelesene tiff_entry in dem 'einzelData'
**  benötig wird, ggf. werden die Daten dorthin übetragen
**  entry  : auf den gelesen tiff_entry
**  return : Wurde der tiff_entry ausgelesen ?
*/
static short sucheEinzelData ( const tiff_entry *const entry )
  {
  const short tag = short2short( &entry->tag );
  short lesen;
  short i = 0;
  while( i < _IeinzelCount && einzelData[i].tag != tag ) ++i;
  lesen = i < _IeinzelCount;
  if( lesen )
    {
    einzelInfo *const data = einzelData + i;
    short ok = 1;
    if( data->ok )
      err_message( "Das Feld \"%s\" existiert mehrfach.", data->name );
    if( dword2dword( &entry->count ) != (DWORD) 1 )
      err_message( "Der Anzahl-Eintrag im Feld \"%s\" ist != 1.", data->name );
    switch( short2short( &entry->typ ) )
      {
      case _BYTE  : data->data = byte2dword( &entry->data );
                    break;
      case _WORD  : data->data = word2dword( &entry->data );
                    break;
      case _DWORD : data->data = dword2dword( &entry->data );
                    break;
      default     : ok = 0;
                    err_message( "Die Typ-Angabe im Feld \"%s\" ist ungültig.",
                                 data->name );
      }
    data->ok = ok;
    }
  return lesen;
  }

/*
**  Lesen der Daten eines tiff_entry's der ein Feld von Bytes enthält
**  count  : Anzahl der Bytes
**  data   : das 'data' Feld des tiff_entry's
**  return : die gelesen und umgewandelten Werte (free nicht vergessen)
*/
static DWORD* leseBYTEs ( short count, DWORD data )
  {
  DWORD *const dest = ok_malloc( count * sizeof (DWORD) );
  const size_t sourceSize = count * sizeof (BYTE);
  BYTE* source;
  short i;
  if( sourceSize > sizeof data )
    source = loadData( data, sourceSize );
  else
    source = (BYTE*) &data;
  for( i = 0; i < count; ++i )
    dest[i] = byte2dword( source + i );
  if( sourceSize > sizeof data ) free( source );
  return dest;
  }

/*
**  Lesen der Daten eines tiff_entry's der ein Feld von Words enthält
**  count  : Anzahl der Words
**  data   : das 'data' Feld des tiff_entry's
**  return : die gelesenen und umgewandelten Werte (free nicht vergessen)
*/
static DWORD* leseWORDs ( short count, DWORD data )
  {
  DWORD *const dest = ok_malloc( count * sizeof (DWORD) );
  const size_t sourceSize = count * sizeof (WORD);
  WORD* source;
  short i;
  if( sourceSize > sizeof data )
    source = loadData( data, sourceSize );
  else
    source = (WORD*) &data;
  for( i = 0; i < count; ++i )
    dest[i] = word2dword( source + i );
  if( sourceSize > sizeof data ) free( source );
  return dest;
  }

/*
**  Lesen der Daten eines tiff_entry's der ein Feld von DWords enthält
**  count  : Anzahl der DWords
**  data   : das 'data' Feld des tiff_entry's
**  return : die gelesenen und übersetzen Werte (free nicht vergessen)
*/
static DWORD* leseDWORDs ( short count, DWORD data )
  {
  DWORD *const dest = ok_malloc( count * sizeof (DWORD) );
  const size_t sourceSize = count * sizeof (DWORD);
  DWORD* source;
  short i;
  if( sourceSize > sizeof data )
    source = loadData( data, sourceSize );
  else
    source = &data;
  for( i = 0; i < count; ++i )
    dest[i] = dword2dword( source + i );
  if( sourceSize > sizeof data ) free( source );
  return dest;
  }

/*
**  Untersucht ob der gelesene tiff_entry in den 'arrayData'
**  benötigt wird, ggf. werden die Daten dorthin übertragen
**  entry  : auf den gelesen tiff_entry
**  return : Wurde der tiff_entry ausgelesen ?
*/
static int sucheArrayData ( const tiff_entry *const entry )
  {
  const short tag = short2short( &entry->tag );
  short lesen;
  short i = 0;
  while( i < _IarrayCount && arrayData[i].tag != tag ) ++i;
  lesen = i < _IarrayCount;
  if( lesen )
    {
    short ok = 1;
    arrayInfo *const data = arrayData + i;
    const DWORD count = dword2dword( &entry->count );
    const short typ = short2short( &entry->typ );
    if( data->pdata != NULL )
      {
      err_message( "Feld \"%s\" ist mehrfach definiert.", data->name );
      free( data->pdata );
      data->pdata = NULL;
      }
    if( count > (DWORD) (INT_MAX / sizeof (DWORD) ) )
      err_message( "Zuviele Einträge in Feld \"%s\".", data->name );
    else
      {
      data->count = (int) count;
      switch( typ )
        {
        case _BYTE  : data->pdata = leseBYTEs( (short) count, entry->data );
                      break;
        case _WORD  : data->pdata = leseWORDs( (short) count, entry->data );
                      break;
        case _DWORD : data->pdata = leseDWORDs( (short) count, entry->data );
                      break;
        default     : err_message( "Typ-Angabe im Feld \"%s\" ungültig", data->name );
        };
      }
    }
  return lesen;
  }

/*
**  Falls in dem gelesenen tiff_entry ein Text enthalten ist
**  wird dieser auf die Konsole ausgegeben
**  entry  : auf den gelesen tiff_entry
**  return : Wurde der tiff_entry ausgelesen ?
*/
static short sucheAscii ( const tiff_entry *const entry )
  {
  const short lese = short2short( &entry->typ ) == _ASCII;
  if( lese )
    {
    char* data;
    const DWORD Count = dword2dword( &entry->count );
    short count;
    short i;
    if( Count > (DWORD) INT_MAX )
      {
      err_message( "Der folgende Text wurde gekürzt." );
      count = INT_MAX;
      }
    else
      count = (short) Count;
    if( count > 4 )
      data = loadData( dword2dword( &entry->data ), count );
    else
      data = (char*) &entry->data;
    fputs( " > ", stdout );
    for( i = 0; i < count && data[i] != '\0'; ++i )
      if( data[i] < 0x20 )
        printf( "\\x%02X", data[i] );
      else
        {
        if( data[i] == '\\' ) putchar( data[i] );
        putchar( data[i] );
        }
    putchar( '\n' );
    if( count > 4 ) free( data );
    }
  return lese;
  }

/*
**  Lesen eines Image File Directories (IFD) aus dem TIFF-File.
**  Benötigte Werte aus dem IFD werden in 'einzelData' und 'arrayData'
**  eingetragen.
**  offset : Position des IFD im File
*/
static void leseIfd ( long offset )
  {
  WORD count;
  WORD i;
  long size;
  tiff_entry* entries;
  DWORD nextifd;
  in_readdirect( offset, &count, sizeof count );
  offset += sizeof count;
  count = word2word( &count );
  size = count * sizeof (tiff_entry);
  if( size > (long) INT_MAX )
    err_abort( "%lu Einträge im IFD können nicht bearbeitet werden.", count );
  entries = ok_malloc( (size_t) size );
  in_readdirect( offset, entries, size );
  offset += size;
  for( i = 0; i < count; ++i )
    {
    short gelesen = sucheEinzelData( entries + i );
    if( ! gelesen ) gelesen = sucheArrayData( entries + i );
    if( ! gelesen ) sucheAscii( entries + i );
    }
  free( entries );
  in_readdirect( offset, &nextifd, sizeof nextifd );
  if( nextifd != 0L )
    err_message( "Mehr als ein Bild im TIFF-File." );
  }

/*
**  Informationen über das Bild aus dem TIFF-File lesen
**  Variable 'intelFormat' wird aus dem TIFF-File bestimmt
*/
static void leseInfo ( void )
  {
  tiff_header header;
  long ifdoffset;
  in_readdirect( 0L, &header, sizeof header );
  if( header.endian != tiff_little && header.endian != tiff_big )
    err_abort( "Ungültiger Code (0x%04X) für die Byteordnung im Fileheader.",
               header.endian );
  intelFormat = header.endian == tiff_little;
  if( word2word( &header.id ) != tiff_id )
    err_message( "Ungültiger ID-Code (0x%04X) im Header.", word2word( &header.id ) );
  ifdoffset = (long) dword2dword( &header.first_ifd );
  if( ifdoffset < 0L || ifdoffset > in_filelength() )
    err_abort( "Positionsangabe des ersten IFDs ist ungültig." );
  leseIfd( ifdoffset );
  }

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
**  Anzahl der noch zu lesenden Zeilen des Bilds in den
**  folgenden Steifen (die Zeilen des aktuellen Streifens
**  sind abgezogen)
*/
static int interpret_zeilen;

/*
**  Anzahl der noch auszugebenden Pixel in dem aktuellen Steifen
*/
static long interpret_pixel;

/*
**  Über diesen Funktionszeiger erfolgt die Verteilung des
**  eingehenden Bytestroms auf die zugehörige Funktion.
*/
static void (*interpret) ( BYTE data );

/*
**  Initialisieren der Ausgabe eines Bildes
*/
static void interpret_init ( void )
  {
  interpret_zeilen = (int) einzelData[_IimageLength].data;
  }

/*
**  Initialisieren der Ausagbe des nächsten Steifens des Bilds
*/
static void interpret_nextStrip ( void )
  {
  int zeilen;
  if( einzelData[_IrowsPerStrip].data > (DWORD) interpret_zeilen )
    zeilen = interpret_zeilen;
  else
    zeilen = (int) einzelData[_IrowsPerStrip].data;
  interpret_pixel = zeilen * einzelData[_IimageWidth].data;
  interpret_zeilen -= zeilen;
  }

/*
**  Verarbeitet den Byte-Stroms aus einer Bitmap, die ein
**  RGB-Bild (3 SamplesPerPixel, 8 BitsPerSample) enthält
**  data : Byte aus dem Strom aus der Bitmap
*/
static void interpret_rgb ( BYTE data )
  {
  static long akku;
  static int i = 0;
  if( interpret_pixel > 0L )
    {
    switch( i )
      {
      case 0 : akku = (int) data * FAKTOR_RED;
               i = 1;
               break;
      case 1 : akku += (int) data * FAKTOR_GREEN;
               i = 2;
               break;
      case 2 : akku += (int) data * FAKTOR_BLUE;
               i = 0;
               --interpret_pixel;
               img_pixel( (BYTE) ( akku / ( FAKTOR_RED+FAKTOR_GREEN+FAKTOR_BLUE) ) );
               break;
      }
    }
  }

/*
**  Verarbeitet ein Graustufen- oder Farbpaletten-Bild mit
**  1 BitsPerSample, benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap
*/
static void interpret_1bit ( BYTE data )
  {
  const int count = interpret_pixel < 8L ? (int) interpret_pixel : 8;
  int i;
  for( i = 0; i < count; ++i )
    {
    img_pixel( interpret_grauwert[ (data & 0x80) ? 1 : 0 ] );
    data <<= 1;
    }
  interpret_pixel -= count;
  }

/*
**  Verarbeitet ein Graustufen- oder Farbpaletten-Bild mit
**  2 BitsPerSample, benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap
*/
static void interpret_2bits ( BYTE data )
  {
  const int count = interpret_pixel < 4L ? (int) interpret_pixel : 4;
  BYTE b[4];
  int i;
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
**  Verarbeitet ein Graustufen- oder Farbpaletten-Bild mit
**  4 BitsPerSample, benutzt Tabelle 'interpret_grauwert'
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
**  Verarbeitet ein Graustufen- oder Farbpaletten-Bild mit
**  8 BitsPerSample, benutzt Tabelle 'interpret_grauwert'
**  data : Byte aus dem Strom aus der Bitmap
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
**  Funktionen und Daten zur Dekomprimierung bei LZW-Kompression
**
\********************************************************************/

/*
**  Maximale Anzahl der LZW-Codes die verarbeitet werden können
**  (lzw_tiff_count wird von dem TIF-Format gefordert)
*/
#define lzw_limit ( ( lzw_tiff_count + 1023 ) & ~1023 )

/*
**  Masken für die 8 .. 14 niederwertigen Bits
*/
static WORD lzw_maske [] =
  {
  0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF
  };

/*
**  Soll nach jedem Streifen ein Clear des LZW-Dekoders ausgeführt
**  werden ?
**  Im Artikel von T.W.Lipp steht ja, aber bei CorelDraw könnte das
**  anders sein.
*/
static int lzw_clearOn = 1;

/*
**  Hier wird die Codetabelle für die LZW-Codes aufgebaut.
**  lzw_suffix[] ist dabei wieder ein Index in der Tabelle.
*/
static WORD lzw_suffix [ lzw_limit ];
static BYTE lzw_praefix [ lzw_limit ];

/*
**  Anzahl der Bits im nächsten zu lesenden LZW-Code
*/
static int lzw_bits;

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
  lzw_nextcode = lzw_tiff_neu;
  lzw_bits = 9;
  lzw_muster = lzw_nocode;
  }

/*
**  Setzt globale Variablen, die während der Laufzeit konstant
**  bleiben und löst eine Clear der LZW-Dekodierung aus
*/
static void lzw_init ( void )
  {
  WORD i;
  for( i = 0; i < lzw_tiff_neu; ++i ) lzw_praefix[i] = (BYTE) i;
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
  if( code == lzw_tiff_clear )
    lzw_clear();
  else if( code > lzw_nextcode )
    err_abort( "Ungültiger LZW-Code (0x%04X).", code );
  else if( code != lzw_tiff_end )
    {
    int sp = 0;
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
      if( lzw_nextcode >= lzw_maske[lzw_bits-8] && lzw_nextcode < lzw_tiff_count )
        ++lzw_bits;
      if( lzw_nextcode >= lzw_limit )
        err_abort( "Ungültiger LZW-Code, Clear-Code wurde erwartet" );
      }
    lzw_muster = code;
    }
  }

/*
**  Datenstruktur wird in leseLZW benutzt um die Codes
**  mit variabler Länge aus dem Bytestrom zu lösen
*/
union lzw_akku
  {
  DWORD d;
  BYTE b[4];
  };

/*
**  Einlesen und dekomprimieren eines Streifens einer Bitmap
**  mit dem LZW-Verfahren. Über in_newbuffer() wurde alles für
**  das Lesen des Streifens vorbereite
**  size : Anzahl des Bytes in dem Streifen
*/
static void leseLZW ( DWORD size )
  {
  WORD code = lzw_nocode;
  int bitposition;
  union lzw_akku akku;
  if( size < (DWORD) 3 )
    err_abort( "Der Streifen enthält keine LZW-Codes" );
  if( lzw_clearOn ) lzw_clear();
  bitposition = 0;
  while( size > (DWORD) 0 && code != lzw_tiff_end )
    {
    while( bitposition < lzw_bits )
      {
      akku.b[2] = akku.b[1];
      akku.b[1] = akku.b[0];
      akku.b[0] = in_readbyte();
      --size;
      bitposition += 8;
      }
    bitposition -= lzw_bits;
    code = (WORD) ( akku.d >> bitposition ) & lzw_maske[lzw_bits-8];
    lzw_decode( code );
    }
  if( code != lzw_tiff_end )
    err_message( "LZW-Daten enden in einem Streifen ohne Ende-Code." );
  }

/*
**  Funktion zur Dekomprimierung bei PackBits-Kompression
**
\********************************************************************/

/*
**  Einlesen und dekomprimieren eines Streifens mit dem
**  PackBits-Verfahen. Über in_newbuffer() wurde alles zum
**  Lesen des Streifens vorbereitet.
**  size : Anzahl der Bytes in dem Streifen
*/
static void lesePackBits ( DWORD size )
  {
  while( size > 1L )
    {
    const signed char akku = (signed char) in_readbyte();
    --size;
    if( akku >= 0 )
      {
      int count = (int) akku + 1;
      if( (long) count > size ) count = (int) size;
      while( count-- > 0 ) (*interpret)( in_readbyte() );
      size -= count;
      }
    else if( akku != -128 )
      {
      int count = 1 - (int) akku;
      BYTE data = in_readbyte();
      --size;
      while( count-- > 0 ) (*interpret)( data );
      }
    }
  }

/*
**  Funktion falls keine Kompresssion
**
\********************************************************************/

/*
**  Einlesen eines Streifens einer nicht komprimierten Bitmap
**  Über in_newbuffer() wurde alles zum Lesen des Streifens
**  vorbereitet
**  size : Anzahl der Bytes in dem Streifen
*/
static void leseUncompressed ( DWORD size )
  {
  while( size-- > (DWORD) 0 ) (*interpret)( in_readbyte() );
  }

/*
**  allgemeine Steuerfunktionen für die Umwandlung
**
\********************************************************************/

static const char fehlt [] = "Das Feld \"%s\" ist nicht angegeben.";

/*
**  Bestimmt aus dem IFD-Eintrag 'bitsPerSample' die Anzahl der
**  Farben/Graustufen und setzt 'interpret' entsprechend
**  return : Anzahl der Farben/Graustufen
*/
static int getColors ( void )
  {
  int colors;
  const arrayInfo *const bitsPerSample = arrayData + _IbitsPerSample;
  if( bitsPerSample->pdata == NULL )
    err_abort( fehlt, bitsPerSample->name );
  if( bitsPerSample->count < 1 )
    err_abort( "Kein Wert in Feld \"%s\" angegeben.", bitsPerSample->name );
  if( bitsPerSample->count > 1 )
    err_message( "Mehr als ein Wert in Feld \"%s\" angegeben.", bitsPerSample->name );
  switch( bitsPerSample->pdata[0] )
    {
    case 1  : colors = 2;
              interpret = interpret_1bit;
              break;
    case 2  : colors = 4;
              interpret = interpret_2bits;
              break;
    case 4  : colors = 16;
              interpret = interpret_4bits;
              break;
    case 8  : colors = 256;
              interpret = interpret_8bits;
              break;
    default : err_abort( "%ld Bits/Sample werden nicht unterstützt.",
                         bitsPerSample->pdata[0] );
    }
  return colors;
  }

/*
**  Auswerten der Informationen aus dem IFD die speziell für ein
**  Schwarz/Weiß-Bild sind.
**  Globale Variablen interpret, interpret_colors,
**  und interpret_grauwert werden gesetzt
**  blackIsZero : Ist Pixel-Wert 0 gleich Grauwert 0 ?
*/
static void auswerteBW ( int blackIsZero )
  {
  const int colors = getColors();
  int i;
  printf( "in %d Graustufen\n", colors );
  for( i = 0; i < colors; ++i )
    interpret_grauwert[i] = ( i * 0xFF )/( colors - 1 );
  if( ! blackIsZero )
    for( i = 0; i < colors; ++i )
      interpret_grauwert[i] = 0xFF - interpret_grauwert[i];
  }

/*
**  Auswerten der Informationen aus dem IFD die speziell für
**  ein RGB-Bild (True-Color-Bild) sind.
**  Globale Variable interpret wird gesetzt
*/
static void auswerteRGB ( void )
  {
  const einzelInfo *const samplesPerPixel = einzelData + _IsamplesPerPixel;
  const arrayInfo *const bitsPerSample = arrayData + _IbitsPerSample;
  puts( "in True-Color (8x8x8 Bit)" );
  if( ! samplesPerPixel->ok )
    err_message( fehlt, samplesPerPixel->name );
  else
    {
    if( samplesPerPixel->data != (DWORD) 3 )
      err_abort( "Anzahl Samples/Pixel != 3 im RGB-Bild." );
    }
  if( bitsPerSample->pdata == NULL )
    err_message( fehlt, bitsPerSample->name );
  else
    {
    int i;
    for( i = 0; i < bitsPerSample->count; ++i )
      if( bitsPerSample->pdata[i] != (DWORD) 8 )
        err_abort( "Nur 8 Bits/Sample bei RGB-Bild unterstützt." );
    }
  interpret = interpret_rgb;
  }

/*
**  auswerten der Informationen aus dem IFD die speziell für ein
**  Farbpaletten-Bild sind.
**  Globale Variabelen interpret, interpret_colors und
**  interpret_grauwert werden gesetzt
*/
static void auswertePalette ( void )
  {
  const int colors = getColors();
  const arrayInfo *const colorMap = arrayData + _IcolorMap;
  printf( "in %d Farben\n", colors );
  if( colorMap->pdata == NULL || colorMap->count < colors * 3 )
    {
    err_message( "Farbpalette fehlt oder ist unvollständig" );
    auswerteBW( 1 );
    }
  else
    {
    int i;
    long* grau = ok_malloc( colors * sizeof (long) );
    long graumax = 0L;
    const DWORD *const red   = colorMap->pdata;
    const DWORD *const green = colorMap->pdata + colors;
    const DWORD *const blue  = colorMap->pdata + 2 * colors;
    DWORD max = colorMap->pdata[0];
    for( i = 1; i < 3 * colors; ++i )
      if( colorMap->pdata[i] > max ) max = colorMap->pdata[i];
    if( max > LONG_MAX/256 )
      {
      const DWORD divisor = ( max + LONG_MAX/256 -1 )/ (LONG_MAX/256);
      for( i = 0; i < 3 * colors; ++i ) colorMap->pdata[i] /= divisor;
      }
    for( i = 0; i < colors; ++i )
      {
      grau[i] = red[i] * FAKTOR_RED + green[i] * FAKTOR_GREEN +
                blue[i] * FAKTOR_BLUE;
      if( grau[i] > graumax ) graumax = grau[i];
      }
    if( graumax > 0L )
      {
      const long divisor = ( graumax + 254 )/ 255;
      for( i = 0; i < colors; ++i )
        interpret_grauwert[i] = (BYTE) ( grau[i] / divisor );
      }
    else
      auswerteBW( 1 );
    free( grau );
    }
  }

/*
**  Auswerten der gelesen Informationen über das Bild aus dem TIFF-File
*/
static void auswerteInfo ( void )
  {
  int i;
  int ok = 1;
  for( i = 0; i < _IeinzelCount; ++i )
    if( einzelData[i].muss && ! einzelData[i].ok )
      {
      err_message( fehlt, einzelData[i].name );
      ok = 0;
      }
  for( i = 0; i < _IarrayCount; ++i )
    if( arrayData[i].muss && arrayData[i].pdata == NULL )
      {
      err_message( fehlt, arrayData[i].name );
      ok = 0;
      }
  if( ! ok ) err_abort( "Ohne diese Angabe(n) kann das Bild nicht gelesen werden" );
  if( einzelData[_IimageWidth].data > (DWORD) INT_MAX )
    err_abort( "Bild breiter als %d Pixel kann nicht gelesen werden.", INT_MAX );
  if( einzelData[_IimageLength].data > (DWORD) INT_MAX )
    err_abort( "Bild höher als %d Pixel kann nicht gelesen werden.", INT_MAX );
  if( arrayData[_IstripOffset].pdata == NULL || arrayData[_IstripOffset].count < 1L )
    err_abort( "Keine Bilddaten vorhanden." );
  if( arrayData[_IstripByteCount].pdata == NULL
      || arrayData[_IstripByteCount].count < arrayData[_IstripOffset].count )
    err_abort( "Angaben über die Anzahl der Bytes der Bilddaten fehlen." );
  printf( "Lese TIFF-File (%lu x %lu) ",
          einzelData[_IimageWidth].data, einzelData[_IimageLength].data );
  switch( einzelData[_IimagePhotometric].data )
    {
    case _dataWhiteIsZero : auswerteBW( 0 );
                            break;
    case _dataBlackIsZero : auswerteBW( 1 );
                            break;
    case _dataRGB         : auswerteRGB();
                            break;
    case _dataPalette     : auswertePalette();
                            break;
    default : err_abort( "Interpretataionsart (0x%lX) wird nicht unterstützt.",
                         einzelData[_IimagePhotometric].data );
    }
  if( arrayData[_IcolorMap].pdata != NULL )
    free( arrayData[_IcolorMap].pdata );
  img_start( (int) einzelData[_IimageWidth].data,
             (int) einzelData[_IimageLength].data, 1 );
  }

/*
**  Nachdem der IFD gelesen und ausgewertet wurde kann nun
**  die Bitmap gelesen und ausgewertet werden
*/
static void leseBitmap ( void )
  {
  const DWORD *const stripOffset = arrayData[_IstripOffset].pdata;
  const DWORD *const stripByteCount = arrayData[_IstripByteCount].pdata;
  const int strips = arrayData[_IstripOffset].count;
  const int compression = (int) einzelData[_Icompression].data;
  int i;
  interpret_init();
  for( i = 0; i < strips; ++i )
    {
    if( stripOffset[i] & (DWORD) 1 )
      err_message( "Streifen Nr. %d beginnt nicht auf WORD-Grenze.", i );
    if( stripOffset[i] + stripByteCount[i] > in_filelength() )
      err_message( "Streifen soll über Dateiende laufen." );
    in_newbuffer( stripOffset[i] );
    interpret_nextStrip();
    switch( compression )
      {
      case _dataUncompressed : leseUncompressed( stripByteCount[i] );
                               break;
      case _dataLZW          : leseLZW( stripByteCount[i] );
                               break;
      case _dataPackBits     : lesePackBits( stripByteCount[i] );
                               break;
      default : err_abort( "Komression-Typ (0x%lX) wird nicht unterstützt.",
                           einzelData[_Icompression].data );
      }
    }
  }

/*
**  Lesen des Input-Files über die in_*() Funktionen und schreiben
**  des IMG-Files über die img_*() Funktionen
*/
void x_trafo ( void )
  {
  lzw_init();
  leseInfo();
  auswerteInfo();
  leseBitmap();
  }

/*
**  Auswerten der Schalter speziell für dieses Modul
**  sw     : der Schalter von der Kommandozeile, ohne das Schaltersymbol
**  return : Konnte der Schalter ausgewertet werden ?
*/
int x_schalter ( const char* sw )
  {
  const int ok = strcmp( sw, "clearoff" ) == 0;
  if( ok ) lzw_clearOn = 0;
  return ok;
  }
