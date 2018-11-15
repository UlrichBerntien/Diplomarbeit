/*
**  Zortech C V2.1
**  .03.1994  Ulrich Berntien
**
**  Enthält:
**  - Einlesen aus IMG-File
**  - Definition des Aufbaus eines TIFF-Files
**  - Schreiben in ein TIFF-File
**  - Transformation der Bitmap von IMG-Format in TIF-Format
**    mit LZW-Kompresssion oder unkomprimiert
**
**  Die LZW-Kompression und das Schreiben der Bits in das TIFF-File
**  sind sehr ähnlich den Funktionen für das GIF-File, daher waren
**  in diesen Teilen nur kleine Änderungen notwendig.
**
**  Speicherung des Bilds als Graustufenbild (Klasse G) würde
**  weniger Platz benötigen.
**
**  Aufbau der Datei:
**    tiff_header
**    tiff_ifd
**    Farbtabelle
**    Bilddaten (ggf. LZW komprimiert)
**
**  05.03.1994  Beginn, entstanden aus GIF.C
**  07.04.1994  Default Extension für Output-Filenamen eingeführt
**  28.04.1994  Schalter '-d' eingeführt
**  03.05.1994  Konstanten und Typen des Tiff-Formats in TIFF.H gelöst
**  27.05.1994  Aufteilung des Bilds in 2,4 oder 8 Steifen je nach Schalter
**  09.06.1994  Standardeinstellung 8 Streifen pro Bild
*/

#include <io.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "img2x.h"
#include "tiff.h"

/********************************************************************/

/*
**  Programmname TIFF
*/
const char *const progname = "IMG2TIFF";

/*
**  Programmfunktion TIFF
*/
const char *const progfunktion =
  "Das Output-File wird im TIF-Format (Version 5.0/6.0) geschrieben.\n";

/*
**  Beschreibung der Schalterfunktionen für diese Modul
*/
const char *const manual2 =
  "  -2  Das Bild wird in 2 Steifen aufgeteilt.\n"
  "  -4  Das Bild wird in 4 Steifen aufgeteilt.\n"
  "  -8  Das Bild wird in 8 Steifen aufgeteilt.\n"
  "      Dieses ist die Standardeinstellung.\n"
  "  -noclear  Am Ende jedes Steifens wird kein Clear-Code angehängt,\n"
  "      wie bei anderen Programmen üblich. Ohne diesen Schalter wird\n"
  "      zur Sicherheit ein Clear angehängt.\n"
  "  -d  Benutzen des Differencing Predictor Verfahren zu besseren\n"
  "      Kompression. Wird ab TIFF-Version 6.0 unterstützt.\n"
  "";

/*
**  Default Extension des Output-Filenamens
*/
const char *const extension = ".TIF";

/*
**  Soll das Differencing Predictor Verfahren benutzt werden ?
*/
static int predictor = 0;

/*
**  In wieviele Steifen soll das Bild aufgeteilt werden ?
**  erlaubt ist 2,4 oder 8
*/
static int countStrips = 8;

/*
**  Soll am Ende jedes Steifens ein Clear-Code angehängt werden ?
*/
static int addclear = 1;

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
**  von Oben nach Unten geliefert, wie sie vom TIF-Format benötigt werden
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

/** TIFF-File schreiben *********************************************/

/*
**  Daten für das TIFF IFD
*/
static const char artist []       = "Institut fuer Experimentalphysik";
static const char copyright []    = "(c) Universitaet Duesseldorf";
static const char hostComputer [] = "MS-DOS";
static const char description []  = "converted IMG-File";
static const char software []     = "IMG2TIFF Version " __DATE__;

/*
**  Länge des Strings mit dem Datum und der Uhrzeit
*/
#define sizeof_dateTime 20

/*
**  Aufbau des Image File Directories (ifd)
**  Enthält Informationen über ein Bild im File
**  Die Namen der Felder vom Typ tiff_entry sind die Namen der
**  Konstanten vom enum tiff_tagTAG ohne den Präfix 'TAG'.
*/
typedef struct tiff_ifdTAG
  {
  WORD anzahl;
  tiff_entry class;             /* Tiff-Klasse, Art des Bildes (3:Palette)   */
  tiff_entry subFileTyp;        /* Art des Bilds im TIFF-File                */
  tiff_entry imageWidth;        /* Breite des Bilds in Pixel                 */
  tiff_entry imageLength;       /* Höhe des Bilds in Pixel                   */
  tiff_entry bitsPerSample;     /* Bits pro Pixel (4 oder 8 möglich)         */
  tiff_entry compression;       /* Kompressionsart (1:keine,2:CCITT,5:LZW)   */
  tiff_entry photometric;       /* Farbformat (3:Palette)                    */
  tiff_entry stripOffset;       /* Fileoffset zu jedem Streifen              */
  tiff_entry rowsPerStrip;      /* Anzahl der Bildzeilen pro Streifen        */
  tiff_entry stripByteCount;    /* Anzahl der Bytes in jedem Streifen        */
  tiff_entry xResolution;       /* Anzahl der Spalten pro Auflösungseinheit  */
  tiff_entry yResolution;       /* Anzahl der Zeilen pro Auflösungseinheit   */
  tiff_entry resolutionUnit;    /* Auflösungseinheit                         */
  tiff_entry colorMap;          /* Farbtabelle Anzahl = 2^BitsPerSample * 3  */
                                /*    Data[i] = Windows-RGB-Wert * 256       */
  tiff_entry artist;            /* Person, die das Bild erzeugt hat          */
  tiff_entry dateTime;          /* Datum und Uhrzeit der Bilderzeugung       */
  tiff_entry copyright;         /* Copyright-Vermerk                         */
  tiff_entry hostComputer;      /* Computer oder Betriebssystem              */
  tiff_entry description;       /* Bildbeschreibung                          */
  tiff_entry software;          /* Software mit der das Bild erzeugt wurde   */
  tiff_entry predictor;         /* Differencing Predictor benutzt ?          */
  LONG next_ifd;
  /* weitere Daten (>32Bit) und für die TIFF-Felder */
  DWORD  _stripOffset[ 8 ];                     /* maximal 8 Steifen möglich */
  DWORD  _stripByteCount[ 8 ];
  DWORD _xResolution[ 2 ];
  DWORD _yResolution[ 2 ];
  char  _artist[ (sizeof artist +1 ) & ~1 ];     /* char-Anzahl immer gerade */
  char  _dateTime[ (sizeof_dateTime +1 ) & ~1 ];
  char  _copyright[ (sizeof copyright +1 ) & ~1 ];
  char  _hostComputer[ (sizeof hostComputer +1 ) & ~1 ];
  char  _description[ (sizeof description +1 ) & ~1 ];
  char  _software[ (sizeof software +1 ) & ~1 ];
  }
  tiff_ifd;

/*
**  Offsets im TIFF-File
**  der Offset des zweiten Streifens wird erst zur Laufzeit bestimmt,
**  da dieser unmittelbar hinter dem ersten Streifen beginnt
*/
static const long tiff_off_header
  = 0L;
static const long tiff_off_ifd
  = sizeof (tiff_header);
static const long tiff_off_rgb
  = sizeof (tiff_header) + sizeof (tiff_ifd);
static const long tiff_off_strip0
  = sizeof (tiff_header) + sizeof (tiff_ifd) + 0x0600;

/*
**  Offsets der einzelnen Steifen, werden erst beim Schreiben
**  des TIFF-Files ermittelt
*/
static long tiff_off_strips[ 8 ];

/*
**  Größen der Streifen im TIFF-File, werden erst beim Schreiben
**  des TIFF-Files ermittelt
*/
static long tiff_size_strips[ 8 ];

/*
**  Setzen eines tiff_entry's auf einen gegeben Wert vom Typ WORD
**  ptr : auf einen tiff_entry
**  x   : Sollwert
*/
static void tiff_setword ( tiff_entry *const ptr, WORD x )
  {
  ptr->typ   = _WORD;
  ptr->count = 1;
  ptr->data  = (DWORD) x;
  }

/*
**  Setzen eines tiff_entry's auf einen gegeben Wert von Typ DWORD
**  ptr : auf einen tiff_entry
**  x   : Sollwert
*/
static void tiff_setdword ( tiff_entry *const ptr, DWORD x )
  {
  ptr->typ   = _DWORD;
  ptr->count = 1;
  ptr->data  = x;
  }

/*
**  Setzen eines tiff_entry's auf einen gegebene Wert vom Typ RATIONAL
**  ptr    : auf einen tiff_entry
**  offset : Offset des Werts bezüglich der tiff_ifd Struktur
*/
static void tiff_setratio ( tiff_entry *const ptr, size_t offset )
  {
  ptr->typ   = _RATIONAL;
  ptr->count = 1;
  ptr->data  = (DWORD) offset + (DWORD) tiff_off_ifd;
  }

/*
**  Setzen eines tiff_entry's auf eine Stringsadresse und Stringlänge
**  ptr    : auf einen tiff_entry
**  len    : Länge des Strings (mit abschließendem '\0')
**  offset : Offset des Strings bezüglich der tiff_ifd Struktur
*/
static void tiff_setstring ( tiff_entry *const ptr, size_t len, size_t offset )
  {
  ptr->typ   = _ASCII;
  ptr->count = (DWORD) len;
  ptr->data  = (DWORD) offset + (DWORD) tiff_off_ifd;
  }

/*
**  Schreiben des tiff_header in das TIFF-File
**  benutzt Konstanten aus dem img-Modul
*/
static void tiff_write_header ( void )
  {
  tiff_header* head;
  out_seek( tiff_off_header );
  head = out_getbuffer( sizeof (tiff_header) );
  head->endian = tiff_little;
  head->id = tiff_id;
  head->first_ifd = tiff_off_ifd;
  }

/*
**  Setzen aller TAG-Werte gemäß des TIF-Formats
**  ifd : auf den tiff_ifd
*/
static void tiff_settags ( tiff_ifd *const ifd )
  {
#define SETTAG( feld ) ifd->feld.tag = _TAG##feld
  SETTAG( class );
  SETTAG( subFileTyp );
  SETTAG( imageWidth );
  SETTAG( imageLength );
  SETTAG( bitsPerSample );
  SETTAG( compression );
  SETTAG( predictor );
  SETTAG( photometric );
  SETTAG( stripOffset );
  SETTAG( rowsPerStrip );
  SETTAG( stripByteCount );
  SETTAG( xResolution );
  SETTAG( yResolution );
  SETTAG( resolutionUnit );
  SETTAG( colorMap );
  SETTAG( artist );
  SETTAG( dateTime );
  SETTAG( copyright );
  SETTAG( hostComputer );
  SETTAG( description );
  SETTAG( software );
#undef SETTAG
  }

/*
**  Schreibt Datum und Uhrzeit in den String 'str' im TIF-Format
**  str : Buffer der Länge sizeof_dateTime
*/
static void tiff_setDateTime ( char *const str )
  {
  struct tm* currenttime;
  time_t timedata;
  time( &timedata );
  currenttime = localtime( &timedata );
  strftime( str, sizeof_dateTime, "%Y:%m:%d %H:%M:%S", currenttime );
  }

/*
**  Setzen eines tiff_entry's auf einen String
**  Dabei gibt es eine globale Variable 'element' mit dem Sollstring,
**  in der tiff_ifd gibt es einen tiff_entry mit Namen 'element' und
**  einen ausreichend großen String mit Namen '_element'
**  ifd_pointer : auf die tiff_ifd Struktur mit dem 'element'
**  element     : Name des Elemnts (s.o)
*/
#define COPY_STRING( ifd_pointer, element )            \
  tiff_setstring( &ifd_pointer->element,               \
                  sizeof ifd_pointer->_##element,      \
                  offsetof(tiff_ifd,_##element[0]) );  \
  strcpy( (char*) ifd_pointer->_##element, element )

/*
**  Setzen eines tiff_entrys.data auf den Offset der zugehörigen
**  Daten mit dem Namen "_element"
**  ifd_pointer : auf die tiff_ifd Struktur mit dem 'element'
**  element     : Name des Elements (s.o.)
*/
#define SET_OFFSET( ifd_pointer, element )             \
  ifd_pointer->element.data =                          \
     tiff_off_ifd + offsetof( tiff_ifd, _##element[0] )

/*
**  Schreiben des tiff_ifd in das TIFF-File
**  Erst Aufrufen, nachdem das Bild ins File geschrieben wurde, denn
**  die Filelänge wird benötigt.
**  compressed : Bilddaten komprimiert ?
*/
static void tiff_write_ifd ( int compressed )
  {
  int i;
  tiff_ifd* ifd;
  out_seek( tiff_off_ifd );
  ifd = out_getbuffer( sizeof (tiff_ifd) );
  tiff_settags( ifd );
  ifd->anzahl = 21;
  ifd->next_ifd = 0L;
  tiff_setword( &ifd->class, _dataPalette );
  tiff_setdword( &ifd->subFileTyp, 1 );   /* Bild mit voller Auflösung */
  tiff_setword( &ifd->imageWidth, img_width );
  tiff_setword( &ifd->imageLength, img_height );
  tiff_setword( &ifd->bitsPerSample, 8 );
  tiff_setword( &ifd->compression, compressed ? _dataLZW : _dataUncompressed );
  tiff_setword( &ifd->predictor, ( compressed && predictor ) ? 1 : 0 );
  tiff_setword( &ifd->photometric, _dataPalette );
  tiff_setword( &ifd->rowsPerStrip, img_height / countStrips );
  SET_OFFSET( ifd, stripOffset );
  ifd->stripOffset.typ   = _DWORD;
  ifd->stripOffset.count = countStrips;
  for( i = 0; i < countStrips; ++i )
    ifd->_stripOffset[i] = tiff_off_strips[i];
  SET_OFFSET( ifd, stripByteCount );
  ifd->stripByteCount.typ   = _DWORD;
  ifd->stripByteCount.count = countStrips;
  for( i = 0; i < countStrips; ++i )
    ifd->_stripByteCount[i] = tiff_size_strips[i];
  tiff_setratio( &ifd->xResolution, offsetof(tiff_ifd,_xResolution[0]) );
  ifd->_xResolution[0] = (DWORD) 254 * (DWORD) img_width;
  ifd->_xResolution[1] = (DWORD) 10  * (DWORD) img_width_mm;
  tiff_setratio( &ifd->yResolution, offsetof(tiff_ifd,_yResolution[0]) );
  ifd->_yResolution[0] = (DWORD) 254 * (DWORD) img_height;
  ifd->_yResolution[1] = (DWORD) 10  * (DWORD) img_height_mm;
  tiff_setword( &ifd->resolutionUnit, _dataInch );
  ifd->colorMap.typ   = _WORD;
  ifd->colorMap.count = 3 * 0x100;
  ifd->colorMap.data  = tiff_off_rgb;
  tiff_setstring( &ifd->dateTime, sizeof ifd->_dateTime,
                                  offsetof(tiff_ifd,_dateTime[0]) );
  tiff_setDateTime( (char*) ifd->_dateTime );
  COPY_STRING( ifd, artist );
  COPY_STRING( ifd, copyright );
  COPY_STRING( ifd, hostComputer );
  COPY_STRING( ifd, description );
  COPY_STRING( ifd, software );
  }

/*
**  Schreiben der tiff_rgbtabelle Tabelle in das TIFF-File
**  rgb : Die Intensitätstabelle ( blau = grün = rot = 256 * rgb[i] )
*/
static void tiff_write_rgbtabelle ( BYTE rgb [0x100] )
  {
  int i;
  tiff_rgb* tab;
  out_seek( tiff_off_rgb );
  tab = out_getbuffer( sizeof (tiff_rgb) );
  for( i = 0; i < 0x100; ++i )
    tab->blue[i] = tab->green[i] = tab->red[i] = (WORD) rgb[i] * 256;
  }

/** Bits in tiff_data schreiben **************************************/

/*
**  Nummer des Bits 0..15 umrechnen in Position im WORD
*/
static const WORD bit_bits [16] =
  {
  0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000
  };

/*
**  Buffer für die aktuell zu schreibenden Bytes
**  benutzt werden die 3 Bytes b[0], b[1], b[2]
*/
static union bit_akkuTAG
  {
  DWORD w;
  BYTE b [4];
  }
  bit_akku;

/*
**  Index des jetzt zu beschreibenden Bits
*/
static short bit_bit_index;

/*
**  Bit-Buffer löschen
*/
static void bit_clear ( void )
  {
  bit_akku.w = 0L;
  bit_bit_index = 24;
  }

/*
**  Bit-Akku in das TIFF-File schreiben (über tiff_akku)
*/
static void bit_flush ( void )
  {
  while( bit_bit_index < 24 )
    {
    out_writebyte( bit_akku.b[2] );
    bit_akku.b[2] = bit_akku.b[1];
    bit_akku.b[1] = bit_akku.b[0];
    bit_akku.b[0] = 0;
    bit_bit_index += 8;
    }
  bit_akku.w = 0L;
  bit_bit_index = 24;
  }

/*
**  Bits in das TIFF-File schreiben (über Buffer)
**  databits : Anzahl der zu schreibenden Bits
**  data     : die zu schreibenden Bits (maximal 15 Bits möglich)
*/
static void bit_write ( short databits, WORD data )
  {
  const DWORD akku = (DWORD) data << ( bit_bit_index - databits );
  bit_bit_index -= databits;
  bit_akku.w |= akku;
  while( bit_bit_index < 16 )
    {
    out_writebyte( bit_akku.b[2] );
    bit_akku.b[2] = bit_akku.b[1];
    bit_akku.b[1] = bit_akku.b[0];
    bit_akku.b[0] = 0;
    bit_bit_index += 8;
    }
  }

/** LZW Kompression *************************************************/

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
static WORD lzw_praefix [ lzw_tiff_count ];

static BYTE lzw_suffix [ lzw_tiff_count ];
static WORD lzw_great [ lzw_tiff_count ];
static WORD lzw_less [ lzw_tiff_count ];

/*
**  nächster zu vergebender LZW-Code
*/
static WORD lzw_nextcode = lzw_tiff_neu;

/*
**  aktuelle Länge der LZW-Codes in Bits
*/
static int lzw_codebits = 9;

/*
**  aktuelles Muster ( als LZW-Code gespeichert )
*/
static WORD lzw_muster = lzw_nocode;

/*
**  LZW-Codetabelle löschen
*/
static void lzw_clear ( void )
  {
  bit_write( lzw_codebits, lzw_tiff_clear );
  lzw_codebits = 9;
  lzw_nextcode = lzw_tiff_neu;
  lzw_muster = lzw_nocode;
  memset( lzw_praefix, 0xFF, sizeof lzw_praefix );
  }

/*
**  LZW Komprimierung initialisieren
*/
static void lzw_init ( void )
  {
  lzw_codebits = 9;
  lzw_clear();
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
  if( lzw_nextcode < lzw_tiff_count )
    {
    result = lzw_nextcode++;
    if( lzw_nextcode >= bit_bits[lzw_codebits] ) ++lzw_codebits;
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
**  8 Bits LZW-komprimiert in das TIFF-File schreiben (über Buffer)
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
**  Beenden des Schreibens von LZW-Code in das TIFF-File
*/
static void lzw_termit ( void )
  {
  bit_write( lzw_codebits, lzw_muster );
  if( addclear ) lzw_clear();
  bit_write( lzw_codebits, lzw_tiff_end );
  lzw_codebits = 9;
  }

/** Transformation IMG => TIFF ***************************************/

/*
**  Schreiben von Zeilen aus dem IMG-File in das TIFF-File
**  mit LZW-Kompression und ggf. Differencing Predictor
**  zeilen : Anzahl der zu schreibenden Zeilen
*/
static void tiff_write_zeilen ( const int zeilen )
  {
  short zeile, spalte;
  bit_clear();
  lzw_init();
  if( predictor )
    for( zeile = 0; zeile < zeilen; ++zeile )
      {
      BYTE akku = 0;
      for( spalte = 0; spalte < img_width; ++spalte )
        {
        const BYTE tmp = img_get();
        lzw_write( tmp - akku );
        akku = tmp;
        }
      }
  else
    for( zeile = 0; zeile < zeilen; ++zeile )
      for( spalte = 0; spalte < img_width; ++spalte )
        lzw_write( img_get() );
  lzw_termit();
  bit_flush();
  }

/*
**  Das Bild aus dem IMG-File mit LZW-Kompression im
**  TIFF-File speichern
*/
static void compression ( void )
  {
  const int zeilen = img_height / countStrips;
  long position = tiff_off_strip0;
  int i;
  out_seek( position );
  for( i = 0; i < countStrips; ++i )
    {
    tiff_off_strips[i] = position;
    tiff_write_zeilen( zeilen );
    position = out_filelength();
    tiff_size_strips[i] = position - tiff_off_strips[i];
    if( position & 1L )
      {
      out_writebyte( 0 );
      ++position;
      }
    }
  }

/*
**  Daten aus dem IMG-File in das TIFF-File schaufeln
**  (nicht komprimiert)
*/
static void schaufel ( void )
  {
  const long bytesPerStrip = (long) img_width * (long) (img_height/countStrips);
  int i;
  for( i = 0; i < countStrips; ++i ) tiff_size_strips[i] = bytesPerStrip;
  tiff_off_strips[0] = tiff_off_strip0;
  for( i = 1; i < countStrips; ++i )
    tiff_off_strips[i] = tiff_off_strips[i-1] + bytesPerStrip;
  out_seek( tiff_off_strip0 );
  while( img_bufferused != 0 )
    {
    out_write( img_bufferused, img_buffer );
    img_zeile -= img_bufferused / img_width;
    img_fillbuffer();
    }
  }

/*
**  Schalter aus der Komnmanozeile auswerten
**  sw     : der Schalter (ohne das Schaltersymbol)
**  return : Ist der Schalter gültig ?
*/
int schalter2 ( const char* sw )
  {
  int ok = 1;
  if( sw[1] == '\0' )
    switch( sw[0] )
      {
      case 'd' : predictor = 1;
                 break;
      case '2' :
      case '4' :
      case '8' : countStrips = sw[0] - '0';
                 break;
      default  : ok = 0;
      }
  else if( strcmp( sw, "noclear" ) == 0 )
    addclear = 0;
  else
    ok = 0;
  return ok;
  }

/*
**  Durchführen der Transformation   IMG => TIFF
**  skala      : Zuordnungsart Grauwert => RGB-Wert
**  compressed : Soll das TIFF-File komprimiert werden ?
*/
void trafo ( img_skalierung skala, int compressed )
  {
  BYTE* tabelle;
  int prozent;
  if( compressed )
    compression();
  else
    schaufel();
  tiff_write_header();
  tiff_write_ifd( compressed );
  tabelle = img_getgrayscale( skala );
  tiff_write_rgbtabelle( tabelle );
  free( tabelle );
  prozent = ( 100L * out_filelength() )/ filelength( img_file );
  printf( "TIFF-Dateilänge %d%% von der IMG-Dateilänge.\n", prozent );
  }
