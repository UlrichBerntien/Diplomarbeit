/*
**  Zortech C V2.1
**  .03.1994  Ulrich Berntien
**
**  Enthält:
**  - Einlesen aus IMG-File
**  - Schreiben in ein GIF-File
**  - Transformation der Bitmap von IMG-Format in GIF-Format
**    mit LZW-Kompresssion
**
**  03.03.1994  Beginn, Entstanden aus BMP.C
**  07.04.1994  Default Extension für Output-Filename eingeführt
**  03.12.1994  Definition des Aufbaus des GIF-Files in GIF.H isoliert
**              Laufzeit der Funktion bit_write() reduziert
*/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "img2x.h"
#include "gif.h"

/********************************************************************/

/*
**  Programmname GIF
*/
const char *const progname = "IMG2GIF";

/*
**  Programmfunktion GIF
*/
const char *const progfunktion =
  "Das Output-File wird im GIF-Format (Version 89a) geschrieben.\n";

/*
**  Ergänzung zu der Programm-Kurzbeschreibung
*/
const char* const manual2 = NULL;

/*
**  Default Extension des Output-Filenamens
*/
const char *const extension = ".GIF";

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
**  Ein Pixel lesen. Die Pixel werden von Links nach Rechts, die Zeilen
**  von Oben nach Unten geliefert, wie sie vom GIF-Format benötigt werden
*/
static BYTE img_get ( void )
  {
  if( img_readpointer >= img_bufferused )
    {
    img_zeile -= img_bufferused / img_width;
    img_fillbuffer();
    }
  return img_buffer[ img_readpointer++ ];
  }

/** GIF-File schreiben **********************************************/

/*
**  Offsets im GIF-File
*/
static const long gif_off_header
  = 0L;
static const long gif_off_rgb
  = sizeof (gif_header);
static const long gif_off_imagedescriptor
  = sizeof (gif_header) + 0x100 * sizeof (gif_rgb);
static const long gif_off_image
  = sizeof (gif_header) + 0x100 * sizeof (gif_rgb) + sizeof (gif_image_descriptor);

/*
**  Schreibe des gif_header in das gif-File
**  benutzt Konstanten aus dem img-Modul
*/
static void gif_write_header ( void )
  {
  gif_header* head;
  short verhaeltnis;
  gif_resolution rflag;
  out_seek( gif_off_header );
  head = out_getbuffer( sizeof (gif_header) );
  memcpy( head->signatur, gif_signatur, sizeof head->signatur );
  head->width = img_width;
  head->height = img_height;
  rflag.pixel = 7;
  rflag.sort = 0;
  rflag.colors = 7;
  rflag.colormap = 1;
  memcpy( &head->flag, &rflag, 1 );
  head->background = 0;
  verhaeltnis = ( (long) img_width_mm * img_height  * 64 )
                / ( img_width * (long) img_height_mm );
  head->pixel_aspect_ratio = verhaeltnis - 15;
  /* hier folgt die globale Farbtabelle */
  }

/*
**  Schreiben des Kopf eines Teilbilds = gesamte IMG-Bild
**  benutzt Konstanten aus dem img-Modul
*/
static void gif_write_imagedescriptor ( void )
  {
  gif_image_descriptor* head;
  gif_image_flags rflags;
  out_seek( gif_off_imagedescriptor );
  head = out_getbuffer( sizeof (gif_image_descriptor) );
  head->separator = gif_image_sep;
  head->left = 0;
  head->top = 0;
  head->width = img_width;
  head->height = img_height;
  rflags.colors = 7;
  rflags.reserved = 0;
  rflags.sort = 0;
  rflags.interlaced = 0;
  rflags.colormap = 0;
  memcpy( &head->flags, &rflags, 1 );
  /* hier würde die lokale Farbtabelle geschrieben */
  }

/*
**  Schreiben der gif_rgbtabelle Tabelle in das GIF-File
**  rgb : Die Intensitätstabelle ( blau = grün = rot = rgb(i) )
*/
static void gif_write_rgbtabelle ( BYTE rgb [0x100] )
  {
  int i;
  gif_rgb* tab;
  out_seek( gif_off_rgb );
  tab = out_getbuffer( 0x0100 * sizeof (gif_rgb) );
  for( i = 0; i < 0x100; ++i )
    tab[i].rgbBlue = tab[i].rgbGreen = tab[i].rgbRed = rgb[i];
  }

/** Bits in gif_data schreiben **************************************/

/*
**  Buffers
*/
static gif_data bit_data;

static union bit_akkuTAG
  {
  DWORD d;
  BYTE b[4];
  }
  bit_akku;

/*
**  Index des jetzt zu beschreibenden Bits
*/
static int bit_bit_index;

/*
**  Bit-Buffer zurücksetzen
*/
static void bit_clear ( void )
  {
  bit_bit_index = 0;
  bit_data.len = 0;
  bit_akku.d = 0;
  }

/*
**  Schreiben der in bit_data gebufferten Daten
*/
static void bit_data_write ( void )
  {
  out_write( (size_t) bit_data.len + 1, &bit_data );
  bit_data.len = 0;
  }

/*
**  Bit-Buffer in das GIF-File schreiben (über gif_buffer)
*/
static void bit_flush ( void )
  {
  while( bit_bit_index > 0 )
    {
    bit_bit_index -= 8;
    bit_data.x[bit_data.len++] = bit_akku.b[0];
    bit_akku.b[0] = bit_akku.b[1];
    bit_akku.b[1] = bit_akku.b[2];
    if( bit_data.len == sizeof bit_data.x ) bit_data_write();
    }
  if( bit_data.len > 0 ) bit_data_write();
  bit_clear();
  }

/*
**  Bits in das GIF-File schreiben (über Buffer)
**  databits : Anzahl der zu schreibenden Bits, maximal 16
**  data     : die zu schreibenden Bits
*/
static void bit_write ( short databits, WORD data )
  {
  bit_akku.d |= (DWORD) data << bit_bit_index;
  bit_bit_index += databits;
  while( bit_bit_index > 7 )
    {
    bit_bit_index -= 8;
    bit_data.x[bit_data.len++] = bit_akku.b[0];
    bit_akku.b[0] = bit_akku.b[1];
    bit_akku.b[1] = bit_akku.b[2];
    bit_akku.b[2] = 0;
    if( bit_data.len == sizeof bit_data.x ) bit_data_write();
    }
  }

/** LZW Kompression *************************************************/

/*
**  Tabelle mit Werten "2 hoch (index+9)"
*/
static const WORD lzw_bits [8] =
  {
  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
  };

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
static const WORD lzw_gif_clear = 256;

/*
**  Spezieller LZW-Code von GIF : Ende der Daten
**  = 2 hoch data_size + 1
*/
static const WORD lzw_gif_end = 257;

/*
**  Ab hier werden die neuen LZW-Codes vergeben
**  = 2 hoch data_size + 2
*/
#define lzw_gif_neu 258

/*
**  Struktur der internen LZW-Tabelle :
**  1) Array indiziert mit Präfixen (WORDs) 'lzw_praefix', also Hash-Tabelle
**     In einem Element steht lzw_nocode -> noch kein LZW-Code zu geordnet
**                                  oder -> Index für das Array 'lzw_suffix'
**  2) Arrays indiziert mit den LZW-Codes, organisiert als Binärbaum,
**     Die Wurzeln werden über das Feld 'lzw_praefix' gefunden
**     'lzw_suffix' in den Elementen stehen die Suffixe zu dem LZW-Code
**     'lzw_great' lzw_nocode -> noch kein LZW-Code zugeordnet, sonst
**                 LZW-Code mit einem Suffix > lzw_suffix[i]
**     'lzw_less'  lzw_nocode -> noch kein LZW-Code zugeordnet, sonst LZW-Code
**                 (Index im lzw_suffix) mit einem Suffix < lzw_suffix[i]
*/

static WORD lzw_praefix [ lzw_gif_count ];

static BYTE lzw_suffix [ lzw_gif_count ];
static WORD lzw_great [ lzw_gif_count ];
static WORD lzw_less [ lzw_gif_count ];

/*
**  nächster zu vergebender LZW-Code
*/
static WORD lzw_nextcode = lzw_gif_neu;

/*
**  aktuelle Länge der LZW-Codes in Bits
*/
static int lzw_codebits = 9;

/*
**  aktuelles Muster ( als LZW-Code gespeichert )
*/
static WORD lzw_muster = lzw_nocode;

/*
**  Initialisieren der LZW-Komprimierung, Tabelle setzen
*/
static void lzw_clear ( void )
  {
  bit_write( lzw_codebits, lzw_gif_clear );
  lzw_codebits = 9;
  lzw_nextcode = lzw_gif_neu;
  lzw_muster = lzw_nocode;
  memset( lzw_praefix, 0xFF, sizeof lzw_praefix );
  }

/*
**  Neuer LZW-Code, oder ggf. Reset der LZW-Code-Tabelle
**  data   : Suffix für das es noch keinen LZW-Code gibt
**  return : der neue Code
*/
static WORD lzw_neuercode ( BYTE data )
  {
  WORD result;
  bit_write( lzw_codebits, lzw_muster );
  if( lzw_nextcode < lzw_gif_count )
    {
    result = lzw_nextcode++;
    if( result >= lzw_bits[lzw_codebits-8] ) ++lzw_codebits;
    lzw_suffix[result] = data;
    lzw_less[result] = lzw_great[result] = lzw_nocode;
    }
  else
    {
    lzw_clear();
    result = lzw_nocode;
    }
  lzw_muster = data;
  return result;
  }

/*
**  8 Bits LZW-komprimiert in das GIF-File schreiben (über Buffer)
**  data : diese 8 Bits schreiben
*/
static void lzw_write ( BYTE data )
  {
  if( lzw_muster == lzw_nocode )
    lzw_muster = data;
  else
    {
    WORD code = lzw_praefix[lzw_muster];                      /* Hashing */
    WORD _near* pcode = lzw_praefix + lzw_muster;
    while( code != lzw_nocode && lzw_suffix[code] != data )   /* Binär Baum */
      if( lzw_suffix[code] > data )
        {
        pcode = lzw_less + code;
        code = lzw_less[code];
        }
      else
        {
        pcode = lzw_great + code;
        code = lzw_great[code];
        }
    if( code == lzw_nocode )
      {
      code = lzw_neuercode( data );
      *pcode = code;
      }
    else
      lzw_muster = code;
    }
  }

/*
**  Beenden des Schreibens von LZW-Code in das GIF-File
*/
static void lzw_termit ( void )
  {
  bit_write( lzw_codebits, lzw_muster );
  lzw_clear(); /* nur zur Sicherheit, Dekoder sauber zurücklassen */
  bit_write( lzw_codebits, lzw_gif_end );
  }

/** Transformation IMG => GIF ***************************************/

/*
**  Schreiben des Bilds ( = Teilbild ) in das GIF-File
*/
static void gif_write_bild ( void )
  {
  const long count = img_width * (long) img_height;
  const BYTE codesize = 8;
  long i;
  out_seek( gif_off_image );
  out_write( sizeof codesize, &codesize );
  bit_clear();
  lzw_clear();
  for( i = 0; i < count; ++i ) lzw_write( img_get() );
  lzw_termit();
  bit_flush();
  out_write( sizeof gif_image_end, &gif_image_end );
  }

/*
**  auswerten eines Schalters von der Kommandozeile
**  sw     : der Schalter (ohne das Schaltersysmbol)
**  return : Ist der Schalter gültig ?
*/
int schalter2 ( const char* sw )
  {
  return 0;
  }

/*
**  Durchführen der Transformation   IMG => GIF
**  skala      : Zuordnungsart Grauwert => RGB-Wert
**  compressed : Soll das GIF-File komprimiert werden ?
*/
void trafo ( img_skalierung skala, int compressed )
  {
  BYTE* tabelle;
  int prozent;
  if( ! compressed )
    puts( "GIF-Files sind immer LZW komprimiert, \"-1\" wird ignoriert." );
  gif_write_header();
  gif_write_imagedescriptor();
  gif_write_bild();
  out_write( sizeof gif_terminator, &gif_terminator );
  tabelle = img_getgrayscale( skala );
  gif_write_rgbtabelle( tabelle );
  free( tabelle );
  prozent = ( 100L * out_filelength() )/ filelength( img_file );
  printf( "GIF-Dateilänge %d%% von der IMG-Dateilänge.\n", prozent );
  }
