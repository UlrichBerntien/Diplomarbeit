/*
**  C    ( Zortech C V2.1 / Micrsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .10.1992
**
**  Auswerten der Spektren
**  Berechnungen zur Halbwertesbreite, Elektronen Dichte und Elektronen
**  Temperatur
**
**  13.06.1993  aus Modul iris04.c abgespalten
**  07.07.1994  Funktion spekrtum2wahrewerte() aufgenommen
**  26.07.1994  Funktion pixel2wahr() isoliert
**  29.07.1994  Funktion um 'wwtabellieren' ergänzt
*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "iris.h"
#include "iris04.h"

/*
**  Tabelle Pixelwert -> Wahrer Wert
**  Umwandlung Grauwert in wahre (relative) Intensität
*/
static double* pixel2wahrARRAY;

/*
**  Umrechnen Grauwert in wahren Wert
**  grauwert :
**  return   : wahrer Wert
*/
double pixel2wahr ( int grauwert )
  {
  double result;
  if( grauwert < 40 )
    result = 0.175349 * pow( 10.0, (double) grauwert / 40 );
  else
    result = pow( 10.0, (double) grauwert / 164 );
  return result;
  }

/*
**  Erzeugen der Tabelle pixel2wahrARRAY für die
**  Umwandlung Grauwert in wahre (relative) Intensität
**  return : alles ok ?
*/
int init_pixel2wahr ( void )
  {
  pixel2wahrARRAY = malloc( 256 * sizeof (double) );
  if( pixel2wahrARRAY == NULL )
    errmalloc();
  else
    {
    int pixel;
    for( pixel = 0; pixel < 256; ++pixel )
      pixel2wahrARRAY[pixel] = pixel2wahr( pixel );
    }
  return pixel2wahrARRAY != NULL;
  }

/*
**  Filehandle für das Schreiben der Tabelle mit den wahren Werten
**  wird in den ww....() Funktionen benutzt
*/
static FILE* wwFile = NULL;

/*
**  Schrittweite in der die Werte in die Tabelle geschrieben werden sollen
**  wird in den ww...() Funktionen benutzt
*/
static int wwSchrittweite;

/*
**  Initialisieren des Schreibens der wahren Werte in das Logfile
*/
static void LOCAL wwInit ( void )
  {
  fputs( "Tabellieren der wahren Werte in das Logfile.\n"
	 "Mit Schrittweite (0=keine Tabelle) ", stdout );
  wwSchrittweite = getzahl( 0, bpz );
  if( wwSchrittweite > 0 )
    {
    wwFile = fopen( logfile, "at" );
    if( wwFile == NULL )
      printf( "Das Logfile \"%s\" kann nicht geöffnet werden.\n", logfile );
    else
      {
      char buffer [70];
      puts( "Kommentare für das Logfile:\n"
	    "(Eine leere Zeile beendet die Eingabe.)" );
      do{
	buffer[0] = '\0';
	stredit( buffer, sizeof buffer );
	if( buffer[0] != '\0' ) fprintf( wwFile, "%s\n", buffer );
	}
	while( buffer[0] != '\0' );
      if( wwtabellieren == 1 )
	fputs( "Index | (rel.Wellenlänge/nm) | Grauwwert | wahrer Wert\n"
	       "------+----------------------+-----------+--------------\n"
	       ,wwFile );
      }
    }
  }

/*
**  Terminieren des Schreibens der wahren Werte in das Logfile
*/
static void LOCAL wwTermit ( void )
  {
  if( wwFile != NULL )
    {
    fputc( '\n', wwFile );
    fclose( wwFile );
    wwFile = NULL;
    }
  }

/*
**  wahre Werte in das wwFile schreiben
*/
static void LOCAL wwZeile ( int index, int grauwert, double wwert )
  {
  if( wwFile != NULL && index % wwSchrittweite == 0 )
    {
    if( wwtabellieren == 1 )
      fprintf( wwFile, "%5d |%21.4Lf |%10d |%12.2Lf\n",
	       index, (double) index * nm_pixel, grauwert, wwert );
    else
      fprintf( wwFile, "%5.2f\n", wwert );
    }
  }

/*
**  Umrechnen eines Spektrums in "wahre Werte" für die Anzeige
**  ggf. Ausgabe einer Tabelle mit den wahren Werten ins Logfile
**  wert : die Grauwerte in diesem Feld werden durch die "wahren Werte"
**         ersetzt, wobei auf Grauwert 255 = "wahrer Wert" 255 skaliert
**         wurde.
*/
void spektrum2wahrwerte ( int wert [bpz] )
  {
  int i;
  double faktor;
  if( pixel2wahrARRAY == NULL ) init_pixel2wahr();
  fputs( "Normieren auf Maximum = 255 Einheiten", stdout );
  if( janein() )
    {
    int max = 0;
    for( i = 0; i < bpz; ++i )
      if( max < wert[i] ) max = wert[i];
    if( max < 1 ) max = 255;
    faktor = 255.0 / pixel2wahrARRAY[ max ];
    }
  else
    faktor = 255.0 / pixel2wahrARRAY[255];
  if( wwtabellieren ) wwInit();
  for( i = 0; i < bpz; ++i )
    {
    const double akku = faktor * pixel2wahrARRAY[ wert[i] ];
    if( wwtabellieren ) wwZeile( i, wert[i], akku );
    wert[i] = (int) akku;
    }
  if( wwtabellieren ) wwTermit();
  }

/*
**  Umwandlung von wahrer (relativer) Intensität in Grauwert
**  wahr   : relative Intensität
**  return : Grauwert
*/
static int LOCAL wahr2pixel ( double wahr )
  {
  int pixel;
  if( wahr < 1.75349 )
    pixel = (int) ( 40 * log10( wahr * 5.70292 ) );
  else
    pixel = (int) ( 164 * log10( wahr ) );
  return pixel;
  }

/*
**  Umwandlung Temperaturwert Kelvin in eV
**  k      : Temperatur in Kelvin
**  return : Temperatur in eV
*/
static double LOCAL k2ev ( double k )
  {
  return k / 1.16045e4;
  }

/*
**  Hilfsfunktion für die Rationale Interpolation
*/
static double LOCAL interpolrat ( int count, double* a, double* b, double* dx  )
  {
  double* tmp;
  int limit;
  for( limit = count - 1; limit > 0; --limit )
    {
    int i;
    int k = count - limit;
    for( i = 0; i < limit; ++i, ++k )
      {
      const double tnm = b[i+1];
      const double diff2 = tnm - a[i];
      if( fabs(diff2) < eps )
        a[i] = tnm;
      else
        {
        const double diff1 = tnm - b[i];
        const double akku = ( dx[i] / dx[k] ) * ( 1.0 - diff1 / diff2 ) - 1.0;
        if( fabs( akku ) < eps )
          {
          putchar( '\b' );
          a[i] = 1.0 / eps;
          }
        else
           a[i] = tnm + diff1 / akku;
        }
      }
    tmp = a;
    a = b + 1;
    b = tmp;
    }
  return b[0];
  }

/*
**  Rationale Interpolation
**  Bestimmt y = y(x) aus einer Tabelle yi,xi mit 'count'
**  Zwischen den Stützstellen wird rational interpoliert
**  Algorithmus aus  Stoer : Einf. in die Numerische Mathematik I
**  x     : An dieser Stelle den Funktioswert interpolieren
**  count : Anzahl der Stützstellen in der Tabelle
**  yi    : Tabelle mit den Funnktswerte yi[k] = y( xi[k] )
**  xi    : Tabelle mit den Stellen an denen die Funktionswerte angegeben sind
*/
static double LOCAL interpol ( double x, int count, const double* yi, const double* xi )
  {
  double result = yi[0];
  double* dx = malloc( count * 3 * sizeof (double) );
  double* a = dx + count;
  double* b = a + count;
  if( dx != NULL )
    {
    int i, hit;
    memset( a, 0 , count * sizeof (double) );
    memcpy( b, yi, count * sizeof (double) );
    hit = -1;
    for( i = 0; i < count; ++i )
      if( fabs( dx[i] = x - xi[i] ) < eps ) hit = i;
    if( hit < 0 )
      result = interpolrat( count, a, b, dx );
    else
      result = yi[hit];
    free( dx );
    }
  else
    errmalloc();
  return result;
  }

/*
**  Im Infoblock müssen die Werte bezüglich des Kontinuums bereits
**  eingetragen sein !
**  info : in dieser Struktur die Werte eintragen,
**         die Lage (Grauwert) des Kontinuums muß schon eingetragen sein
*/
void liniesummieren( infoblock* info )
  {
  const int* wert = info->sp->wert;
  const int grenze = info->kontinuum;
  int pos;
  int count = 0;
  double akku = 0.0;
  info->wkontinuum = pixel2wahrARRAY[ info->kontinuum ];
  info->kontinuumi = info->wkontinuum * 10;
  if( toupper(halb_methode[0]) == 'I' )
    {                                       /* Suche vom Linienzentrum aus */
    pos = info->linie;
    while( pos < bpz && wert[pos] > grenze ) ++pos;
    info->liniebis = pos - 1;
    pos = info->linie - 1;
    while( pos >= 0 && wert[pos] > grenze ) --pos;
    info->linievon = pos + 1;
    }
  else
    {                                       /* Suche vom Bildrand aus */
    const int zentrum = info->linie;
    pos = 10;
    while( pos < zentrum && wert[pos] < grenze ) ++pos;
    info->linievon = pos;
    pos = bpz - 10;
    while( pos > zentrum && wert[pos] < grenze ) --pos;
    info->liniebis = pos;
    }
  if( toupper(limit_methode[0]) == 'M' ) erfragegrenzen( info );
  for( pos = info->linievon; pos <= info->liniebis; ++pos )
    akku += pixel2wahrARRAY[ wert[pos] ];
  count = info->liniebis - info->linievon + 1;
  info->liniealles = akku * nm_pixel;
  info->linieintegral = info->liniealles
           - (double) count * info->wkontinuum * nm_pixel;
  }

/*
**  Bestimmt Halbwertsbreite der Linie
**  info : dort werden die Werte zur Linienhalbwertsbreite eingetragen,
**         die Lage der Spitze und der Linienbereich müssen schon eingetragen sein
*/
void halbwertsbreite ( infoblock* info )
  {
  int wert = info->spitze;
  const int *const werte = info->sp->wert;
  int i;
  info->wspitze = pixel2wahrARRAY[ wert ] - info->wkontinuum;
  info->whalbespitze = info->wspitze / 2;
  info->halbespitze = wert = wahr2pixel( info->whalbespitze + info->wkontinuum );
  i = info->linievon;
  while( i < info->liniebis && werte[i] < wert ) ++i;
  info->halbvon = i;
  i = info->liniebis;
  while( i > info->linievon && werte[i] < wert ) --i;
  info->halbbis = i;
  info->fwhm = (double) ( info->halbbis - info->halbvon + 1 ) * nm_pixel;
  info->fwhmA = 10 * info->fwhm;
  }

/*
**  Berechnet Elektronen-Dichte aus der Starkverbreiterung
**  info : dort werden die Werte zu der Dichte eingetragen,
**         vorher müssen alle Informationen zu Linie angeben sein
*/
void dichte ( infoblock* info )
  {
  int i;
  double cn [stlen];
  double c;
  double dichte;
  double dichtealt;
  const double breite = pow( info->fwhmA, 1.5 );
  const starkdaten* stark = starktabellen + (int) ( info->art - 'a' );
  const double temp = fixtemperatur > 0.0 ? fixtemperatur : info->temperaturk;
  for( i = 0; i < stark->cdichten; ++i )
    cn[i] = interpol( temp, stark->ctemperaturen,
                          stark->c + i*stlen, stark->temperaturen );
  c = cn[0];
  dichte = c * breite;
  do{
    dichtealt = dichte;
    c = interpol( dichtealt, stark->cdichten, cn, stark->dichten );
    dichte = c * breite;
    }
    while ( fabs( dichte - dichtealt ) > fabs( dichte / 1e4 ) );
  info->cnt = c;
  info->ne = dichte;
  }

/*
**  Bestimmt die Elektronen-Temperatur aus dem Verhältnis
**  Linienintensität zu Kontinuumsintensität
**  info : dort werden die Werte zu Temperatur eingetragen
**         voher müssen alle Informationen über die Linie angegeben sein
*/
void temperatur ( infoblock* info )
  {
  info->verhaeltnis = info->linieintegral / info->kontinuumi;
  if( info->art != 'k' )
    {
    const double* x;
    switch( info->art )
      {
      case 'a' : x = ptratio->alpha;
                 break;
      case 'b' : x = ptratio->beta;
                 break;
      case 'c' : x = ptratio->gamma;
      }
    info->temperaturk = interpol( info->verhaeltnis, ptratio->count, ptratio->t, x );
    }
  else
    info->temperaturk = 0.0;
  info->temperaturev = k2ev( info->temperaturk );
  }

