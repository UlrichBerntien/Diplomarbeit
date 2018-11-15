/*
**  Zortech C V2.1
**  .04.1994  Ulrich Berntien
**
**  Modul zum Lesen und Interpretieren von PCX-Dateien
**
**  23.04.1994  Beginn, entstanden aus dem Rumpf von BMP2.C
**  07.05.1994  x_manual und x_schalter() ergänzt
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x2img.h"
#include "pcx.h"

/********************************************************************/

/*
**  Informationen zusammengefaßt, die für das Lesen aus dem PCX-File
**  benötigt werden
*/
typedef struct infoTAG
  {
  int zeilen;        /* Anzahl der Zeilen im PCX-Bild (Pixel)       */
  int spalten;       /* Anzahl der Spalten im PCX-Bild (Pixel)      */
  int kopfstand;     /* Steht das PCX-Bild auf dem Kopf ?           */
  int compressed;    /* Ist das PCX-Bild komprimiert (RLE) ?        */
  int bpp;           /* Anzahl der Bits pro Pixel                   */
  int bpl;           /* Anzahl der Byte pro Zeiel (alle Farbebenen) */
  long farbtabelle;  /* Position der Farbtabellem oder 0L           */
  }
  info;

/*
**  feste Offsets innerhalb des PCX-Files
*/
static const long offset_header = 0L;
static const long offset_bitmap = sizeof (pcx_header);

/*
**  Name des Programms
*/
const char *const x_name = "PCX2IMG";

/*
**  Standard Extension für die Files vom PCX-Typ
*/
const char *const x_extension = ".PCX";

/*
**  keine Erweiterung der Programm-Kurzbeschreibung
*/
const char *const x_manual = NULL;

/*
**  Umrechnung ( Index in Spalte % 8 ) zu Bit im Byte der PCX-Daten
*/
static const BYTE index2bit [8] =
  {
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
  };

/*
**  Umsetzungstabelle Pixelwert -> Grauwert
**  (nicht bei True-Color PCX-Files)
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
**  Daten über das PCX-File speichern
*/
static info data;

/********************************************************************/

/*
**  Einlesen und Auswerten des Headers des PCX-Files
**  Eintragen der Daten in der Variablen 'data'
*/
static void leseHeader ( void )
  {
  pcx_header* header = ok_malloc( sizeof (pcx_header) );
  long akku;
  in_readdirect( offset_header, header, sizeof (pcx_header) );
  if( header->typ != pcx_typ && header->typ != scr_typ )
    err_message( "Weder PCX noch SCR Kennung im Fileheader" );
  if( header->compressed != 0 && header->compressed != 1 )
    err_message( "Unbekannte Komprimierungsart eingetragen, versuche RLE" );
  data.compressed = (int) header->compressed;
  if( header->bitPerPixel != 1 && header->bitPerPixel != 8 )
    err_abort( "%d Bits pro Pixel pro Farbebene werden nicht unterstützt",
               header->bitPerPixel );
  akku = (long) header->xmax - (long) header->xmin;
  if( akku < 0L )
    {
    err_message( "Bild kann seitenverkehrt sein" );
    akku = - akku;
    }
  ++akku;
  if( akku > (long) INT_MAX )
    {
    err_message( "Mehr als %d Spalten, kürze auf diese Bildbreite", INT_MAX );
    akku = (long) INT_MAX;
    }
  data.spalten = (int) akku;
  akku = (long) header->ymax - (long) header->ymin;
  data.kopfstand = akku < 0L;
  if( data.kopfstand )
    {
    err_message( "Bild kann auf dem Kopf stehen" );
    akku = - akku;
    }
  ++akku;
  if( akku > (long) INT_MAX )
    if( data.kopfstand )
      err_abort( "Bild hat mehr als %d Zeilen", INT_MAX );
    else
      {
      err_message( "Mehr als %d Zeilen, kürze auf diese Bildhöhe", INT_MAX );
      akku = (long) INT_MAX;
      }
  data.zeilen = (int) akku;
  switch( header->planes )
    {
    case 1  : data.bpp = header->bitPerPixel == 1 ? 1 : 8;
              break;
    case 3  : if( header->bitPerPixel != 8 )
                err_abort( "3 Bits/Pixel werden nicht unterstützt" );
              else
                data.bpp = 24;
              break;
    case 4  : if( header->bitPerPixel != 1 )
                err_abort( "32 Bits/Pixel werden nicht unterstützt" );
              else
                data.bpp = 4;
              break;
    default : err_abort( "%d Farbebenen werden nicht unterstützt", header->planes );
    }
  akku = (long) header->bytesPerLine * header->planes;
  if( akku > (long) INT_MAX )
    err_abort( "Mehr als %d Bytes/Bildzeile können nicht verarbeitet werden", INT_MAX );
  data.bpl = (int) akku;
  switch( header->ver )
    {
    case pcx_ver25  :
    case pcx_ver3   :
    case pcx_ver28o : data.farbtabelle = 0L;
                      break;
    case pcx_ver28m : if( data.bpp == 2 || data.bpp == 4 )
                        data.farbtabelle = 16L;
                      else if( data.bpp == 8 )
                        data.farbtabelle = in_filelength() - 0x300L;
                      else
                        data.farbtabelle = 0L;
                      break;
    default : err_message( "Unbekannte Version, benutze V2.8 ohne Farbtabelle" );
              data.farbtabelle = 0L;
    }
  if( header->colorForm != 1 && header->colorForm != 2 )
    err_message( "Nicht unterstützte Interpretation der Farbtabelle" );
  if( header->colorForm == 2 && data.farbtabelle != 0L )
    {
    data.farbtabelle = 0L;
    err_message( "Ignoriere die Graustufentabelle, weil das Format nicht bekannt" );
    }
  free( header );
  }

/*
**  Bestimmt aus 'data.bpp' die Anzahl der Farben in der Farbtabelle
**  return : Anzahl der Farben
*/
static int anzahlfarben ( void )
  {
  static const char fktname[] = "anzahlfarben";
  int count;
  switch( data.bpp )
    {
    case  1 : count = 2;
              break;
    case  4 : count = 16;
              break;
    case  8 : count = 256;
              break;
    case 24 : err_intern( fktname, "bei True-Color gibt es keine Farbatbelle" );
              break;
    default : err_intern( fktname, "Ungültige Anzahl der Bits/Pixel" );
    }
  return count;
  }

/*
**  im PCX-File ist keine Farbtabelle vorhanden, also eine
**  Graustufen-Tabelle selbst erzeugen:
**  Pixelwert 0 = Grauwert 0, höchster Pixelwert = Grauwert 255
*/
static void erstelleFarbtabelle ( void )
  {
  const unsigned farben = anzahlfarben();
  const unsigned farbenM1 = farben - 1;
  unsigned akku = 0;
  int i;
  for( i = 0; i < farben; ++i )
    {
    grauwert[i] = (BYTE) ( akku / farbenM1 );
    akku += 255;
    }
  }

/*
**  Einlesen der Farbtabelle aus dem PCX-File und umrechnen in
**  Graustufen-Tabelle.
**  die Position und die Anzahl der Farben ergeben sich aus 'data'
*/
static void leseFarbtabelle ( void )
  {
  const int farben = anzahlfarben();
  const size_t farbsize = farben * 3;
  long* grau = ok_malloc( farben * sizeof (long) );
  BYTE* tafel = ok_malloc( farbsize );
  long max = 0L;
  int i;
  in_readdirect( data.farbtabelle, tafel, farbsize );
  for( i = 0; i < farben; ++i )
    {
    const BYTE *const rgb = tafel + 3 * i;
    grau[i] = rgb[0] * (long) FAKTOR_RED   +
              rgb[1] * (long) FAKTOR_GREEN +
              rgb[2] * (long) FAKTOR_BLUE;
    if( grau[i] > max ) max = grau[i];
    }
  if( max > 0L )
    {
    const long divisor = ( max + 254 )/ 255;
    for( i = 0; i < farben; ++i )
      grauwert[i] = (BYTE) ( grau[i] / divisor );
    }
  else
    erstelleFarbtabelle();
  free( tafel );
  free( grau );
  }

/*
**  Interpretiere eine Zeile aus dem PCX-File als Schwarz/Weiß-Bild
**  1 Bit/Pixel in einer Farbene
**  zeile : die Zeile (nicht komprimiert)
*/
static void verarbeite1bpp ( const BYTE* zeile )
  {
  BYTE akku;
  int s;
  for( s = 0; s < data.spalten; ++s )
    {
    const int m = s & 0x0007;
    if( m == 0 ) akku = *zeile++,
    img_pixel( ( akku & index2bit[m] ) ? grauwert[1] : grauwert[0] );
    }
  }

/*
**  Interpretiere eine Zeile aus dem PCX-File als 16 Farben-Bild
**  4 Bit/Pixel aufgeteilt in 4 Farbebenen mit je 1 Bit/Pixel/Farbebene
**  zeile : die Zeile (nicht komprimiert)
*/
static void verarbeite4bpp ( const BYTE* zeile )
  {
  const int ebene1 = data.bpl;
  const int ebene2 = ebene1 + data.bpl;
  const int ebene3 = ebene2 + data.bpl;
  BYTE akku0, akku1, akku2, akku3;
  int s;
  for( s = 0; s < data.spalten; ++s )
    {
    const int m = s & 0x0007;
    const BYTE maske = index2bit[m];
    BYTE pixel = 0;
    if( m == 0 )
      {
      ++zeile;
      akku0 = zeile[0];
      akku1 = zeile[ebene1];
      akku2 = zeile[ebene2];
      akku3 = zeile[ebene3];
      }
    if( akku0 & maske ) pixel += 0x01;
    if( akku1 & maske ) pixel += 0x02;
    if( akku2 & maske ) pixel += 0x04;
    if( akku3 & maske ) pixel += 0x08;
    img_pixel( grauwert[pixel] );
    }
  }

/*
**  Interpretiere eine Zeile aus dem PCX-File als 256 Farben-Bild
**  8 Bit/Pixel in einer Farbebene mit 8 Bit/Pixel/Farbebene
**  zeile : die Zeile (nicht komprimiert)
*/
static void verarbeite8bpp ( const BYTE *const zeile )
  {
  int s;
  for( s = 0; s < data.spalten; ++s )
    img_pixel( grauwert[ zeile[s] ] );
  }

/*
**  Interpretiere eine Zeile aus dem PCX-File als True-Color-Bild
**  24 Bit/Pixel aufgeteilt in 3 Farbebenen (Rot/Grün/Blau) mit
**  je 1 Byte/Pixel/Farbebene
**  zeile : die Zeile (nicht komprimiert)
*/
static void verarbeite24bpp ( const BYTE *const zeile )
  {
  const BYTE *const red   = zeile;
  const BYTE *const green = red + data.bpl;
  const BYTE *const blue  = green + data.bpl;
  int s;
  for( s = 0; s < data.spalten; ++s )
    {
    long farbe = red[s] * (int) FAKTOR_RED;
    farbe += green[s] * (int) FAKTOR_GREEN;
    farbe += blue[s] * (int) FAKTOR_BLUE;
    img_pixel( (BYTE) ( farbe / (FAKTOR_RED+FAKTOR_GREEN+FAKTOR_BLUE) ) );
    }
  }

/*
**  Eine Zeile aus dem PCX-Bild verarbeiten und ins IMG-Bild leiten
**  zeile : Die Zeile aus dem PCX-Bild (nicht komprimiert)
*/
static void verarbeite ( const BYTE *const zeile )
  {
  switch( data.bpp )
    {
    case  1 : verarbeite1bpp( zeile );
              break;
    case  4 : verarbeite4bpp( zeile );
              break;
    case  8 : verarbeite8bpp( zeile );
              break;
    case 24 : verarbeite24bpp( zeile );
              break;
    default : err_abort( "leseBitmap", "Anzahl Bits/Pixel ungültig" );
    }
  }

/*
**  Einlesen und Interpretieren der Bitmap aus dem PCX-File
**  Die Bitmap ist nicht komprimiert
*/
static void leseBitmap ( void )
  {
  const size_t size = data.bpl * sizeof (BYTE);
  BYTE* buffer = ok_malloc( size );
  int z;
  printf( "Lese PCX-File (%d x %d) mit %d Bits/Pixel (unkomprimiert)\n",
          data.spalten, data.zeilen, data.bpp );
  for( z = 0; z < data.zeilen; ++z )
    {
    in_read( buffer, size );
    verarbeite( buffer );
    }
  free( buffer );
  }

/*
**  Einlesen und Interpretieren der Bitmap aus dem PCX-File
**  Die Bitmap ist Lauflängen komprimiert
*/
static void leseBitmapRLE ( void )
  {
  const size_t size = data.bpl * sizeof (BYTE);
  BYTE* buffer = ok_malloc( size );
  int z;
  printf( "Lese PCX-File (%d x %d) mit %d Bits/Pixel (RLE)\n",
          data.spalten, data.zeilen, data.bpp );
  for( z = 0; z < data.zeilen; ++z )
    {
    int i = 0;
    while( i < data.bpl )
      {
      BYTE akku = in_readbyte();
      if( akku >= 0xC0 )
        {
        const int repeat = akku & 0x3F;
        akku = in_readbyte();
        memset( buffer + i, akku, repeat );
        i += repeat;
        }
      else
        buffer[i++] = akku;
      }
    verarbeite( buffer );
    }
  free( buffer );
  }

/*
**  Lesen des Input-Files über die in_*() Funktionen und schreiben
**  des IMG-Files über die img_*() Funktionen
*/
void x_trafo ( void )
  {
  leseHeader();
  if( data.farbtabelle != 0L )
    leseFarbtabelle();
  else
    erstelleFarbtabelle();
  img_start( data.spalten, data.zeilen, ! data.kopfstand );
  in_newbuffer( offset_bitmap );
  if( data.compressed )
    leseBitmapRLE();
  else
    leseBitmap();
  }

/*
**  Auswerten der Schalter speziell für diese Modul
**  sw     : der Schalter aus der Kommandozeile, ohne das Schaltersymbol
**  return : Konnte der Schalter ausgewertet werden ?
*/
int x_schalter ( const char* sw )
  {
  return 0;
  }
