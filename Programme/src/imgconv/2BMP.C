/*
**  Zortech C V2.1
**  .02.1994  Ulrich Berntien
**
**  Enthält:
**  - Einlesen aus IMG-File
**  - Definition des Aufbaus eines BMP-Files
**  - Schreiben in ein BMP-File
**  - Transformation der Bitmap von IMG-Format in BMP-Format
**    mit/ohne Kompresssion
**
**  20.02.1994  gelöst aus dem Hauptprogramm
**  07.04.1994  Default Extension für Output-Filenamen aufgenommen
**  16.04.1994  Definitionen des BMP-Files in bmp.h heraus gelöst
**  20.04.1994  Einträge in bmp_fileheader.size und bmp_infoheader.compressed
**              korrigiert
**  28.04.1994  Schalterauswertung aufgenommen
*/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "img2x.h"
#include "bmp.h"

/********************************************************************/

/*
**  Programmname BMP
*/
const char *const progname = "IMG2BMP";

/*
**  Programmfunktion BMP
*/
const char *const progfunktion =
  "Das Output-File wird im BMP-Format geschrieben.\n";

/*
**  keine Ergänzung in der Programmbeschreibung
*/
const char *const manual2 = NULL;

/*
**  Default Extension des Output-Filenamens
*/
const char *const extension = ".BMP";

/********************************************************************/

/*
**  Buffer 'img_buffer' wieder auffüllen
**  'img_readpointer' und 'img_bufferused' setzen
*/
void img_fillbuffer ( void )
  {
  const int readlines = img_bufferlines > img_zeile ? img_zeile : img_bufferlines;
  const int startline = img_zeile - readlines;
  img_bufferused = img_width * readlines;
  img_seek( (long) startline * (long) img_width );
  errno = 0;
  if( read( img_file, img_buffer, img_bufferused ) != img_bufferused )
    img_error( "Lesen" );
  img_readpointer = img_width * ( readlines - 1 );
  img_registriere();
  }

/*
**  Eine Zeile lesen. Die Zeilen werden von Unten nach Oben
**  geliefert, wie sie im BMP-Format benötigt werden.
**  buffer : Inhalt der Zeile dorthin schreiben
*/
static void img_getline ( BYTE buffer [img_width] )
  {
  memcpy( buffer, img_buffer + img_readpointer, img_width );
  if( img_readpointer == 0 ) img_fillbuffer();
  img_readpointer -= img_width;
  --img_zeile;
  }

/** BMP-File schreiben **********************************************/

/*
**  Köpfe und die Farbtabelle haben hier immer eine konstante Länge
**  also können die Offset zum Fileanfang direkt angegeben werden
*/
static const long bmp_off_fileheader = 0L;
static const long bmp_off_infoheader = sizeof (bmp_fileheader);
static const long bmp_off_rgbquad    = sizeof (bmp_fileheader)
                                       + sizeof (bmp_infoheader);
static const long bmp_off_bitmap     = sizeof (bmp_fileheader)
                                       + sizeof (bmp_infoheader)
                                       + 256 * sizeof (bmp_rgbquad);

/*
**  Schreibe des bmp_fileheader in das BMP-File
**  Diese Header kann erst geschrieben werden, nachdem die gesamte Bitmap
**  geschrieben wurde, weil die Filegröße bekannt sein muß.
*/
static void bmp_write_fileheader ( void )
  {
  bmp_fileheader* header;
  const long filelength = out_filelength();
  out_seek( bmp_off_fileheader );
  header = out_getbuffer( sizeof (bmp_fileheader) );
  header->typ = bmp_typ;
  header->size = filelength;
  header->reserved = 0L;
  header->offsetBitmap = bmp_off_bitmap;
  }

/*
**  Schreibt den bmp_infoheader in das BMP-File
**  Dieser Header kann erste geschrieben werden, nachdem das gesamte Bitmap
**  geschrieben wurde, weil die Größe der Bitmap feststehen muß.
**  compress : Wurde die Bitmap RLE8 komprimiert ?
**             es werden auch Konstanten aus dem img-Modul benutzt
*/
static void bmp_write_infoheader ( int compress )
  {
  bmp_infoheader* header;
  const long filelength = out_filelength();
  out_seek( bmp_off_infoheader );
  header = out_getbuffer( sizeof (bmp_infoheader) );
  header->size = sizeof (bmp_infoheader);
  header->width = img_width;
  header->height = img_height;
  header->planes = 1;
  header->bitCount = 8;
  header->compression = compress ? bmp_rle8 : bmp_rgb;
  header->sizeImage = filelength - bmp_off_bitmap;
  header->xPelsPerMeter = ( img_width * 1000L ) / img_width_mm;
  header->yPelsPerMeter = ( img_height * 1000L ) / img_height_mm;
  header->clrUsed = 0x100;
  header->clrImportant = 0x100;
  }

/*
**  Schreiben der bmp_rgbquad Tabelle in das BMP-File
**  rgb : Die Intensitätstabelle ( blau = grün = rot = rgb(i) )
*/
static void bmp_write_rgbquad ( BYTE rgb [0x100] )
  {
  bmp_rgbquad* quad;
  int i;
  out_seek( bmp_off_rgbquad );
  quad = out_getbuffer( 0x100 * sizeof (bmp_rgbquad) );
  for( i = 0; i < 0x100; ++i )
    {
    quad[i].rgbBlue = quad[i].rgbGreen = quad[i].rgbRed = rgb[i];
    quad[i].reserved = 0;
    }
  }

/** Transformation IMG => BMP ***************************************/

/*
**  Übersetzen der Zeilen unkomprimiert
*/
static void trafo_1zu1 ( void )
  {
  const int fillbytes = img_height & 3;
  int i;
  for( i = 0; i < img_height; ++i )
    {
    BYTE* buffer = out_getbuffer( img_width );
    img_getline( buffer );
    if( fillbytes > 0 )
      {
      buffer = out_getbuffer( fillbytes );
      memset( buffer, 0, fillbytes );
      }
    }
  }

/*
**  Zählen, wieviele gleiche Bytes jetzt im Buffer folgen
**  buffer : in diesem Buffer suchen
**  index  : ab dieser Position suchen (bis zur Position img_width)
*/
static int countequal ( BYTE buffer [img_width], int index )
  {
  const BYTE akku = buffer[index];
  int i = index;
  do ++i; while( i < img_width && buffer[i] == akku );
  return i - index;
  }

/*
**  Schreiben im RLE8-Format vom BMP-File:
**  Ein Byte 'input' folgt 'count' mal in der Bitmap
**  input  : das wiederholt vorkommende Bytes
**  output : dorthin schreiben
**  count  : Anzahl der Wiederholungen des Bytes
**  return : Anzahl der geschriebenen Bytes
*/
static int wiederholt ( BYTE input, BYTE output[], int count )
  {
  int i;
  if( count < 1 ) internerror( "wiederholt", "count ungültig" );
  for( i = 0; count > 0; i += 2 )
    {
    output[i] = count > 255 ? 255 : count;
    output[i+1] = input;
    count -= output[i];
    }
  return i;
  }

/*
**  Schreiben im RLE8-Format vom BMP-File:
**  Ein Feld von 'zaehler' Bytes in die Bitmap
**  input   : Zeiger hinter das letzte zu schreibende Byte
**  output  : dorthin schreiben
**  zaehler : Anzahl der Bytes (0..255 wird verarbeitet)
**  return  : Anzahl der geschriebenen Bytes
*/
static int einzeln ( BYTE* input, BYTE* output, int zaehler )
  {
  int count = 0;
  if( zaehler > 255 || zaehler < 0 ) internerror( "einzeln", "zaehler ungültig" );
  if( zaehler > 3 )
    {
    output[0] = 0;
    output[1] = zaehler;
    memcpy( output+2, input-zaehler, zaehler );
    count = zaehler + 2;
    if( zaehler & 1 )
      {
      ++count;
      output[zaehler+2] = 0;
      }
    }
  else
    while( zaehler > 0 )
      {
      count += wiederholt( input[-zaehler], output + count, 1 );
      --zaehler;
      }
  return count;
  }

/*
**  Übersetzen einer Zeile ins komprimierte RLE8-Format
**  buffer : Buffer für die Zeile im RLE8-Format
**  return : Anzahl geschriebener Bytes in den Buffer
*/
static int zeile_rle8 ( BYTE buffer [] )
  {
  BYTE input [img_width];
  int dest = 0;
  int source = 0;
  int zaehler = 0;
  img_getline( input );
  while( source < img_width )
    {
    const int count = countequal( input, source );
    if( count < 3 )
      {
      ++source;
      ++zaehler;
      if( zaehler >= 255 )
        {
        dest += einzeln( input + source, buffer + dest, zaehler );
        zaehler = 0;
        }
      }
    else
      {
      dest += einzeln( input + source, buffer + dest, zaehler );
      zaehler = 0;
      dest += wiederholt( input[source], buffer + dest, count );
      source += count;
      }
    }
  dest += einzeln( input + source, buffer + dest, zaehler );
  return dest;
  }

/*
**  Übersetzen der Bitmap komprimiert
*/
static void trafo_rle8 ( void )
  {
  BYTE* buffer;
  int zeile;
  for( zeile = 0; zeile < img_height; ++zeile )
    {
    size_t i;
    const size_t buffersize = 3 * img_width;
    buffer = out_getbuffer( buffersize );
    i = zeile_rle8( buffer );
    buffer[i++] = 0;
    buffer[i++] = 0;  /* Code: Ende der Zeile */
    if( i < buffersize )
      out_ungetbuffer( buffersize - i );
    else if( i > buffersize )
      internerror( "trafo_rle8", "Buffer übergelaufen" );
    }
  out_writebyte( 0 );
  out_writebyte( 1 ); /* Code : Ende der Bitmap */
  }

/*
**  Auswertung von Schalter aus der Kommandozeile
**  sw     : der Schalter (ohne das Schaltersymbol '-')
**  return : Ist der Schalter gültig ?
*/
int schalter2 ( const char* sw )
  {
  return 0;
  }

/*
**  Durchführen der Transformation   IMG => BMP
**  skala      : Zuordnungsart Grauwert => RGB-Wert
**  compressed : Soll das BMP-File komprimiert werden ?
*/
void trafo ( img_skalierung skala, int compressed )
  {
  BYTE* tabelle;
  int prozent;
  out_seek( bmp_off_bitmap );
  if( compressed )
    trafo_rle8();
  else
    trafo_1zu1();
  bmp_write_fileheader();
  bmp_write_infoheader( compressed );
  tabelle = img_getgrayscale( skala );
  bmp_write_rgbquad( tabelle );
  free( tabelle );
  prozent = ( 100L * out_filelength() )/ filelength( img_file );
  printf( "BMP-Dateilänge %d%% von der IMG-Dateilänge.\n", prozent );
  }
