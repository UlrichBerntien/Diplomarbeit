/*
**  C    ( Zortech C V2.1 / Micrsoft C 5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .09.1992
**
**  Vorsicht : Stack wird stark mit Daten belastet, bis zu 2 KByte
**             in einer Funktion
**
**  27.09.1992  Beginn
**  01.10.1992  Umbenennung des Headerfiles 'iris01.h' => 'iris.h'
**  02.10.1992  Aufteilung in 'iris01.c' und 'iris02.c'
**  03.10.1992  Aufteilung in 'iris01.c' und 'iris00.c'
**  19.11.1992  Funktion kopfstand() ergänzt
**  06.01.1993  min/max bestimmen in expandoben(), expandunten()
**              funktioniert jetzt auch wenn Name '_' enthält
**  14.09.1993  Funktion bildspiegeln() ergänzt
**  29.07.1994  Funktion permanent() ergänzt
**  02.09.1994  Funktion minmax() zur Laufzeitoptimierung ergänzt
**  21.12.1994  Funktion permanent() öffentlich gemacht
*/

/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iris.h"
#include "isdefs.h"

/********************************************************************/

#define ipl 256                                 /* Integers pro LUT */

/********************************************************************/

/*
**  Auf Wunsch String in das Bild schreiben
**  frb : frame buffer number
**  str : diesen String schreiben
*/
static void LOCAL printstr ( int frb, char* str )
  {
  int wohin;
  printf( "Bild mit Text '%s' beschriften ?\n", str );
  puts( "links oben  -  4               3  -  rechts oben\n"
        "                    0 - nicht\n"
        "links unten -  1               2  -  rechts unten" );
  wohin = getzahl( 0, 4 );
  if( wohin )
    {
    int zeile = ( wohin == 4 || wohin == 3 ) ? 4 : 490;
    int rbuendig = wohin == 3 || wohin == 2;
    is_draw_str( frb, zeile, rbuendig, str );
    }
  }

/********************************************************************/

/*
**  Vertauscht zwei Variable vom Typ char*
**  v1, v2 : zeigen auf die zu tauschenden Variablen
*/
static void LOCAL swap ( char** v1, char** v2 )
  {
  char* tmp = *v1;
  *v1 = *v2;
  *v2 = tmp;
  }

/*
**  Ein Bild in die beiden Halbbilder zerlegen,
**  die Halbblider untereinander auf dem Bildschirm zeigen
**  frb : frame buffer number
*/
static void LOCAL split ( int frb )
  {
  char alt [zpb];
  char buffera [bpz];
  char bufferb [bpz];
  int zeile;
  puts( "Splitten des Bildes in die beiden Halbbilder" );
  for( zeile = 0; zeile < zpb/2; ++zeile ) alt[zeile] = 1;
  for( zeile = 0; zeile < zpb/2; ++zeile )
    if( alt[zeile] )
      {
      char* buffer = buffera;
      int wer = zeile;
      int nach = wer & 1 ? (wer >> 1) + zpb/2 : wer >> 1;
      char* buffer2 = bufferb;
      isb_get_pixel( frb, zeile, wpz, buffer );
      while( nach != zeile )
        {
        isb_get_pixel( frb, nach, wpz, buffer2 );
        isb_put_pixel( frb, nach, wpz, buffer );
        alt[nach] = 0;
        swap( &buffer, &buffer2 );
        wer = nach;
        nach = wer & 1 ? (wer >> 1) + zpb/2 : wer >> 1;
        }
      isb_put_pixel( frb, nach, wpz, buffer );
      alt[nach] = 0;
      }
  }

/********************************************************************/

/*
**  Index des ersten und letzten Elements != 0 aus werte suchen
**  werte : zu durchsuchendes Feld
**  min   : dort den minimalen Index speichern
**  max   : dort den maximalen Index speichern
*/
static void LOCAL minmax ( const char werte [256], int* min, int* max )
  {
  int i;
  i = 0;
  while( i < 256 && werte[i] == 0 ) ++i;
  *min = i == 256 ? 0 : i;
  i = 255;
  while( i > 0 && werte[i] == 0 ) --i;
  *max = i;
  }

/*
**  Das obere Halbbild (Zeilen 0..zpb/2-1) zum Gesamtbild expandieren
**  frb  : frame buffer number
**  pmax : dort den größten Grauwert im Bild speichern
**  pmin : dort den kleinsten Grauwert im Bild speichern
*/
static void LOCAL expandoben ( int frb, int* pmax, int* pmin )
  {
  char buffer [ bpz ];
  char werte [256];
  int source = zpb/2 -1;
  int destination = zpb - 1;
  memset( werte, 0, sizeof werte );
  while( source >= 0 )
    {
    isb_get_pixel( frb, source--, wpz, buffer );
    if( source > 8 && source < 242 )
      {
      int i;
      for( i = bpz-1; i >= 0; --i )
        werte[ (uchar) buffer[i] ] = 1;
      }
    isb_put_pixel( frb, destination--, wpz, buffer );
    isb_put_pixel( frb, destination--, wpz, buffer );
    }
  minmax( werte, pmin, pmax );
  }

/*
**  Das untere Halbbild (Zeilen zpb/2..zpb-1) zum Gesamtbild expandieren
**  frb  : frame buffer number
**  pmax : dort den größten Grauwert im Bild speichern
**  pmin : dort den kleinsten Grauwert im Bild speichern
*/
static void LOCAL expandunten ( int frb, int* pmax, int* pmin )
  {
  char buffer [ bpz ];
  char werte [256];
  int source = zpb/2;
  int destination = 0;
  memset( werte, 0, sizeof werte );
  while( source < zpb )
    {
    isb_get_pixel( frb, source++, wpz, buffer );
    if( source > 266 && source < 499 )
      {
      int i;
      for( i = bpz-1; i >= 0; --i )
        werte[ (uchar) buffer[i] ] = 1;
      }
    isb_put_pixel( frb, destination++, wpz, buffer );
    isb_put_pixel( frb, destination++, wpz, buffer );
    }
    minmax( werte, pmin, pmax );
  }

/*
**  Benutzerdialog, nachfragen was von Halbbild
**  zu Gesamtbild expandiert werden soll
**  frb    : frame buffer number
**  max    : dort den größten Grauwert im Bild speichern
**  min    : dort den kleinsten Grauwert im Bild speichern
**  return : wurde ein Halbbild expandiert ? (nur dann *mas und *min gültig)
*/
static int LOCAL expanddialog ( int frb, int* max, int* min )
  {
  char was;
  puts( "Ein Halbbild zum Gesamtbild expandieren ?\n"
        "\t- - nein\n"
        "\to - oberes\n"
        "\tu - unteres"    );
  was = getcset( "-ou" );
  if( was == 'o' )
    expandoben( frb, max, min );
  else if( was == 'u' )
    expandunten( frb, max, min );
  return was != '-';
  }

/********************************************************************/

/*
**  Besetzt eine Lock-up-table mit Werten zur maximaler Kontrastausnutzung
**  zwischen 'min' und 'max' linearer Anstieg von 0 auf 0xff
**  LUT : Nummer der LUT
**  max : größter Grauwert im Bild
**  min : kleinster Grauwert im Bild
*/
static void LOCAL maxminlut( int LUT, int max, int min )
  {
  int lut [ipl];
  const unsigned diff = max - min;
  const unsigned step = diff > 0 ? 0xffff / diff : 0;
  unsigned akku;
  int i;
  for( i = 0; i < min; ++i ) lut[i] = 0;
  akku = 0;
  for( i = min; i < max; ++i )
    {
    lut[i] = akku >> 8;
    akku += step;
    }
  for( i = max; i < ipl; ++i ) lut[i] = 0x00ff;
  is_load_olut( LUT, lut, lut, lut );
  }

/*
**  Erstellt eine Output-LUT, die ein Bild ähnlich Höhenlinien erzeugt
**  LUT : Nummer der LUT
**  max : größter Grauwert im Bild
**  min : kleinster Grauwert im Bild
*/
static void LOCAL zebralut ( int LUT, int max, int min )
  {
  int lut [ ipl ];
  const int stufen = 15;
  const int step = max - min;
  int i;
  int now = 0;
  for( i = 0; i < ipl; i += 2 )
    {
    lut[i+1] = lut[i] = now / stufen;
    now += step;
    if( now > 0x00ff * stufen ) now = 0;
    }
  is_load_olut( LUT, lut, lut, lut );
  }

/*
**  erstellen einer Output-LUT
**  LUT : Nummer der LUT
**  max : größter Grauwert im Bild
**  min : kleinster Grauwert im Bild
*/
static void LOCAL wahrelut ( int LUT, int max, int min )
  {
  int lut [ ipl ];
  int i;
  const double maxWahre = pixel2wahr( max );
  const double minWahre = pixel2wahr( min );
  double faktor;
  if( max != min )
    faktor = 255.0 / ( maxWahre - minWahre );
  else
    faktor = 1.0;
  i = 0;
  while( i <= min ) lut[i++] = 0;
  while( i < max )
    {
    lut[i] = (int) ( ( pixel2wahr( i ) - minWahre ) * faktor );
    ++i;
    }
  while( i < ipl ) lut[i++] = 255;
  is_load_olut( LUT, lut, lut, lut );
  }

/*
**  Bestimmt Index eines Zeichens innerhalb eines Strings
**  str    : in diesem String suchen
**  c      : dieses Zeichen suchen
**  return : Index von c im String, oder falls nicht gefunden Ende des Strings
*/
static int LOCAL strindex ( const char* str, char c )
  {
  int index = 0;
  while ( str[index] != c && str[index] != '\0' ) ++index;
  return index;
  }

/*
**  Bild nach einer LUT ändern.
**  frb    : frame buffer number
**  LUT    : Nummer der LUT nach der das Bild verändert werden soll
**  return : Wurde das Bild verändert ?
*/
int permanent ( int frb, int LUT )
  {
  int ok;
  uchar* buffer = malloc( bpz * 16 );
  fputs( "Die Daten des Bilds werden verändert, ok", stdout );
  ok = janein();
  if( buffer == NULL )
    {
    errmalloc();
    ok = 0;
    }
  if( ok )
    {
    int zeile = 0;
    int lut [ ipl ];
    int dummy [ ipl ];
    is_read_olut( LUT, lut, dummy, dummy );
    is_select_olut( 0 );
    while( zeile < zpb )
      {
      int j;
      isb_get_pixel( frb, zeile, wpz * 16, (char*) buffer );
      for( j = 0; j < bpz * 16; ++j ) buffer[j] = (uchar) lut[ buffer[j] ];
      isb_put_pixel( frb, zeile, wpz * 16, (char*) buffer );
      zeile += 16;
      }
    }
  free( buffer );
  return ok;
  }

/*
**  Dialog zur Kontrastverstärkung
**  frb : frame buffer number
**  max : größter Grauwert im Bild
**  min : kleinster Grauwert im Bild
*/
static void LOCAL kontrast ( int frb, int max, int min )
  {
  int* luts = malloc( ipl * 9 * sizeof (int) );
  TEXT set[] = "nvzspw";
  static const int table [6] = { 0,1,2,3,0,0 };
  printf( "Kontrast : maximaler Pixelwert = %d, minimaler = %d\n", max, min );
  if( luts != NULL )
    {
    char antwort;
    int index = 0;
    is_read_olut( 1, luts+0*ipl, luts+1*ipl, luts+2*ipl );
    is_read_olut( 2, luts+3*ipl, luts+4*ipl, luts+5*ipl );
    is_read_olut( 3, luts+6*ipl, luts+7*ipl, luts+8*ipl );
    maxminlut( 1, max, min );
    zebralut( 2, max, min );
    wahrelut( 3, max, min );
    do{
      fputs( "n - normal, v - verstärkt, z - zebra, s - wahre Werte verstärkt\n"
             "p - permanent speichern, w - weiter ", stdout );
      antwort = getcset( set );
      if( antwort == 'p' )
        antwort = permanent( frb, index ) ? (char) 'w' : (char) 'n';
      index = table[ strindex( set, antwort ) ];
      is_select_olut( index );
      }
      while( antwort != 'w' );
    is_load_olut( 1, luts+0*ipl, luts+1*ipl, luts+2*ipl );
    is_load_olut( 2, luts+3*ipl, luts+4*ipl, luts+5*ipl );
    is_load_olut( 3, luts+6*ipl, luts+7*ipl, luts+8*ipl );
    free( luts );
    }
  else
    errmalloc();
  }

/*******************************************************************/

/*
**  Beschriften, Splitten, Speichern, Expandieren des Bildes
**  frb : frame buffer number
*/
void fastpart ( int frb )
  {
  static char name [ namelen ];
  int max, min;
  getfname( name, sizeof name );
  if( name[0] )
    printstr( frb, name );
  split( frb );
  if( name[0] )
    savedialog ( frb, name );
  if( expanddialog ( frb, &max, &min ) )
    kontrast( frb, max, min );
  }

/*
**  Oben und Unten in Bild vertauschen
**  frb : frame buffer number
*/
void kopfstand ( int frb )
  {
  char buffer1 [bpz];
  char buffer2 [bpz];
  int i;
  for( i = 0; i < zpb / 2; ++i )
    {
    const int ir = zpb - i;
    isb_get_pixel( frb, i , wpz, buffer1 );
    isb_get_pixel( frb, ir, wpz, buffer2 );
    isb_put_pixel( frb, i , wpz, buffer2 );
    isb_put_pixel( frb, ir, wpz, buffer1 );
    }
  }

/*
**  ein Bild spiegeln (vertauscht Links und Rechts)
**  frb : frame buffer number
*/
void bildspiegeln ( int frb )
  {
  char buffer1 [bpz];
  char buffer2 [bpz];
  int i;
  for( i = 0; i < zpb; ++i )
    {
    int j;
    isb_get_pixel( frb, i, wpz, buffer1 );
    for( j = 0; j < bpz; ++j ) buffer2[bpz-1-j] = buffer1[j];
    isb_put_pixel( frb, i, wpz, buffer2 );
    }
  }
