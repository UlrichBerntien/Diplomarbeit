/*
**  Zortech C V2.1
**  .02.1994  Ulrich Berntien
**
**  Enthält:
**  - Einlesen aus IMG-File
**  - Definition des Aufbaus eines PCX-Files
**  - Schreiben in ein PCX-File (Version 2.8 mit Farbtabelle)
**  - Transformation der Bitmap von IMG-Format in PCX-Format
**    mit/ohne Kompresssion
**
**  20.02.1994  Beginn
**  07.04.1994  Default Extension für Output-File eingeführt
**  23.04.1994  Definition des PCX-Headers in 'PCX.H' gelöst, Eintrag in
**              pcx_header.colorForm korrigiert, Abspeichern der Farbtabelle
**              korrigiert, Version in 2.8 mit Farbtabelle geändert
**  28.04.1994  Schalter "-f" eingeführt: speichern im Format 2.8
*/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "img2x.h"
#include "pcx.h"

/********************************************************************/

/*
**  Programmname PCX
*/
const char *const progname = "IMG2PCX";

/*
**  Programmfunktion PCX
*/
const char *const progfunktion =
  "Das Output-File wird im PCX-Format geschrieben.\n";

/*
**  Ergänzung der Schalterbeschreibung
*/
const char *const manual2 =
  "  -f  Datei im Format Version 2.8 mit Farbtabelle schreiben.\n"
  "      Default: Format Version 3.0 ohne Farbtabelle\n"
  "";

/*
**  Default Extension des Output-Filenamens
*/
const char *const extension = ".PCX";

/*
**  Soll in Format 2.8 mit Farbtabelle geschrieben werden ?
*/
static int farbtabelle = 0;

/********************************************************************/

/*
**  Buffer 'img_buffer' wieder auffüllen
**  'img_readpointer' und 'img_bufferused' setzen
*/
void img_fillbuffer ( void )
  {
  const int readlines = img_bufferlines > img_zeile ? img_zeile : img_bufferlines;
  img_bufferused = img_width * readlines;
  errno = 0;
  if( read( img_file, img_buffer, img_bufferused ) != img_bufferused )
    img_error( "Lesen" );
  img_readpointer = 0;
  img_registriere();
  }

/*
**  Eine Zeile lesen. Die Zeilen werden von Oben nach Unten
**  geliefert, wie sie im PCX-Format benötigt werden.
**  buffer : Inhalt der Zeile dorthin schreiben
*/
static void img_getline ( BYTE buffer [img_width] )
  {
  if( img_readpointer >= img_bufferused ) img_fillbuffer();
  memcpy( buffer, img_buffer + img_readpointer, img_width );
  img_readpointer += img_width;
  --img_zeile;
  }

/** PCX-File schreiben **********************************************/

/*
**  Schreibe des pcx_header in das PCX-File
**  benutzt Konstanten aus dem img-Modul
**  comressed : Wird die Bitmap komprimiert gespeichert ?
*/
static void pcx_write_header ( int compressed )
  {
  pcx_header *const head = out_getbuffer( sizeof (pcx_header) );
  memset( head, 0, sizeof (pcx_header) );
  head->typ = pcx_typ;
  head->ver = farbtabelle ? pcx_ver28m : pcx_ver28o;
  head->compressed = compressed ? 1 : 0;
  head->bitPerPixel = 8;
  head->xmin = 0;
  head->ymin = 0;
  head->xmax = img_width - 1;
  head->ymax = img_height - 1;
  head->dpiHor = ( img_width * (long) 25 ) /(long) img_width_mm;
  head->dpiVer = ( img_height * (long) 25 ) / (long) img_height_mm;
  head->planes = 1;
  head->bytesPerLine = img_width;
  head->colorForm = farbtabelle ? 1 : 2;
  }

/*
**  Schreiben der pcx_rgbtabelle Tabelle in das PCX-File
**  rgb : Die Intensitätstabelle ( blau = grün = rot = rgb(i) )
*/
static void pcx_write_rgbtabelle ( BYTE rgb [0x100] )
  {
  int i;
  pcx_rgbtabelle *const tab = out_getbuffer( 0x0301 );
  tab->kennung = pcx_farb;
  for( i = 0; i < 0x100; ++i )
    {
    const int offset = i * 3;
    tab->rgb[offset+0] =           /* Rot  */
    tab->rgb[offset+1] =           /* Grün */
    tab->rgb[offset+2] = rgb[i];   /* Blau */
    }
  }

/** Transformation IMG => PCX ***************************************/

/*
**  Übersetzen der Zeilen unkomprimiert
*/
static void trafo_1zu1 ( void )
  {
  int i;
  for( i = 0; i < img_height; ++i )
    {
    BYTE* buffer = out_getbuffer( img_width );
    img_getline( buffer );
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
**  Schreiben im RLE-Code vom PCX-Format.
**  Ein Byte kommt mehrmals in Folge vor.
**  input  : das zu wiederholende Bytes
**  output : dort dfen Code schreiben
**  count  : Anzahl der Wiederholungen von 'input'
**  return : Anzahl der geschriebenen Bytes in 'output'
*/
static int wiederholt ( BYTE input, BYTE output[], int count )
  {
  int dest = 0;
  if( count < 0 ) internerror( "wiederholt", "count < 0" );
  while( count > 0 )
    {
    const int repeat = count > 0x3F ? 0x3F : count;
    output[dest++] = 0xC0 + repeat;
    output[dest++] = input;
    count -= repeat;
    if( count == 1 && input < 0xC0 )
      {
      output[dest++] = input;
      --count;
      }
    }
  return dest;
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
  img_getline( input );
  while( source < img_width )
    {
    const int count = countequal( input, source );
    if( count > 1 || input[source] >= 0xC0 )
      dest += wiederholt( input[source], buffer + dest, count );
    else
      buffer[dest++] = input[source];
    source += count;
    }
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
    int i;
    const int buffersize = 3 * img_width;
    buffer = out_getbuffer( buffersize );
    i = zeile_rle8( buffer );
    if( i < buffersize )
      out_ungetbuffer( buffersize - i );
    else if( i > buffersize )
      internerror( "trafo_rle8", "Buffer übergelaufen" );
    }
  }

/*
**  Auswerten des Schalters von der Kommanodzeile
**  sw     : der Schalter (ohne das Schaltersysmbol)
**  return : Ist der Schalter gültig ?
*/
int schalter2 ( const char* sw )
  {
  const int ok = sw[0] == 'f' && sw[1] == '\0';
  if( ok )
    farbtabelle = 1;
  return ok;
  }

/*
**  Durchführen der Transformation   IMG => PCX
**  skala      : Zuordnungsart Grauwert => RGB-Wert
**  compressed : Soll das PCX-File komprimiert werden ?
*/
void trafo ( img_skalierung skala, int compressed )
  {
  int prozent;
  pcx_write_header( compressed );
  if( compressed )
    trafo_rle8();
  else
    trafo_1zu1();
  if( farbtabelle )
    {
    BYTE* tabelle = img_getgrayscale( skala );
    pcx_write_rgbtabelle( tabelle );
    free( tabelle );
    }
  prozent = ( 100L * out_filelength() )/ filelength( img_file );
  printf( "PCX-Dateilänge %d%% von der IMG-Dateilänge.\n", prozent );
  }
