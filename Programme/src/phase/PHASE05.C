/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  Auswertung der Signale des Interferometers
**    mit 2-Phasendetektion
**
**  Literatur:
**   . Engeln-Müllges + Reuter :
**     Formelsammlung zur numerischen Mathematik
**       mit C-Programmen
**     B.I.Wissenschaftsverlag, Mannheim ( 1990 )
**   . Siegmund Brandt :
**     Datenanalyse
**     B.I.Wissenschaftsverlag, Mannheim ( 1975 )
**
**  07.04.1993  abgespalten von PHASE2.C
*/

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "int64.h"
#include "phase.h"

/*
**  Tabelle mit den Sinus-Werte
*/
static int* sintab;

/*
**  Sinus/Cosinus über Tabelle berechenen
*/
#define SIN(x) ( sintab[ (word) (x) & CIRC-1 ] )
#define COS(x) ( sintab[ ( (word) (x) + CIRC/4 ) & CIRC-1 ] )

/*
**  Sinus/Cosinus von Wert + Delta über Tabelle berechnen
*/
#define SIND(x,d) ( sintab[ ( (word) (x) + (d) ) & CIRC-1 ] )
#define COSD(x,d) ( sintab[ ( (word) (x) + (d) + CIRC/4 ) & CIRC-1 ] )

/*
**  Berechen der Sinus-Werte für die Tabelle
*/
void _cdecl calc_init ( void )
  {
  const double pi = 4 * atan( 1.0 );
  const double step = pi / (CIRC/2);
  double x = 0.0;
  int i;
  sintab = malloc( CIRC * sizeof (int) );
  if( sintab == NULL ) error( noheap );
  for( i = 0; i < CIRC; ++i, x += step )
    sintab[i] = (int) ( INT_MAX * sin( x ) );
  }

/*
**  Bestimmt minimalen und maximalen Wert eines Felds von longs's
**    n   : Anzahl der Werte
**    x   : n Werte
**    min : dort den minimalen Wert speichern
**    max : dort den maximalen Wert speichern
*/
static void LOCAL minmax ( int n, const long* x, long* min, long* max )
  {
  long tmin, tmax;
  int i;
  tmin = tmax = x[0];
  for( i = 1; i < n; ++i )
    if( tmin > x[i] )
      tmin = x[i];
    else if( tmax < x[i] )
      tmax = x[i];
  *min = tmin;
  *max = tmax;
  }

/*
**  zu minimierende Funktion für Bestimmung von Phi
**  Summer der Fehlerquadrate
*/
#define F(x) ( gauss32( as - SIN(x) / A, bs - SIND(x,deltaphi) / B ) )

/*
**  Ableitung von F(x) bis auf den Faktor -CIRC/4Pi
*/
#define G(x) ( mul32( as - SIN(x)/A,           COS(x)/A           )     \
             + mul32( bs - SIND(x,deltaphi)/B, COSD(x,deltaphi)/B ) )

/*
**  Bestimmen des Winkels phi.
**  Minimun über gedämpften Newton-Algorithmus bestimmen
**    x  : Berechnet Feld 'winkel.data'
**         die Felder A,A0,B,B0,deltaphi,signalA->data,signalB->data
**         werden zur Berechnung benutzt
*/
static void LOCAL calc_phi ( phasenwerte* x )
  {
  const int A = x->A;
  const int B = x->B;
  const int deltaphi = x->deltaphi;
  const int len = x->signalA.len;
  long phi = 0;
  int i;
  for( i = 0; i < len; ++i )
    {
    const int as = (int) x->signalA.data[i] - x->A0;
    const int bs = (int) x->signalB.data[i] - x->B0;
    int weiter = 1;
    long f = F(phi);
    do{
      const long fs = G(phi);
      long phi1, f1;
      if( fs != 0l )
        {
        weiter = (int) ( ( f * (CIRC/16) )/fs );
        if( weiter < -CIRC/2 + 1 )
          weiter = -CIRC/2 + 1;
        else if( weiter > CIRC/2 - 1 )
          weiter = CIRC/2 + 1;
        }
      else
        weiter = weiter > 0 ? 8 : -8;
      do{
        phi1 = phi + ( weiter /= 2 );
        f1 = F(phi1);
        }
        while( weiter && f < f1 );
      if( weiter && f == f1 )
        {
        do phi1 += weiter; while( f == ( f1 = F(phi1) ) );
        if( f < f1 )
          {
          f1 = f;
          phi1 -= weiter;
          weiter = 0;
          }
        }
      phi = phi1;
      f = f1;
      }
      while( weiter );
    x->winkel.data[i] = phi;
    }
  }

/*
**  Regressiongerade   y = x / A + B  berechnen
**    A      : dort wird 1/Steigung gespeichert
**    B      : dort wird der Achsenabstand gespeichert
**    n      : Anzahl der Meßwerte (2..1024)
**    y      : Meßwerte y im Bereich 0 .. 255
**    x      : Werte x im Bereich -INT_MAX .. + INT_MAX
**    return : != 0, wenn ein Overflow aufgetreten ist
*/
static int LOCAL lineal ( int* A, int* B, int n, const int* x, const uchar* y )
  {
  int error, i;
  long Sx = 0l;
  long Sy = 0l;
  int64 Sxx;
  int64 Sxy;
  int64 a,b,c;
  null64( Sxx );
  null64( Sxy );
  for( i = 0; i < n; ++i )
    {
    Sx += x[i];
    inc64l( Sxx, mul32( x[i],x[i] ) );
    Sy += y[i];
    inc64l( Sxy, mul32( x[i], (int) y[i] ) );
    }
  mul64l( a, Sxx, Sy );
  mul64l( b, Sxy, Sx );
  dec64x( a, b );
  mul64( b, Sxx, n );
  mul64ll( c, Sx, Sx );
  dec64x( b, c );
  div64( c, a,b );
  error = from64( B, c );
  mul64( a, Sxy, n );
  mul64ll( c, Sx, Sy );
  dec64x( a, c );
  div64( c, b,a );
  error = from64( A, c ) || error;
  return error;
  }

/*
**  Bestimmen der Werte A,A0 oder B,B0.
**  Benutzt dabei die lineare Regression.
**  Bei extrem schlechten Startwerten von x,x0 können die Werte aus
**  der linearen Regression < 0 werden, dann werden diese verworfen
**  und Standartwerte eingesetzt.
**    x      : benutzt die Felder signalA.data oder signalB.data
**             und winkel.data
**    return : Betragsdifferenz alte, neue Werte
*/
static long LOCAL calc_AB ( phasenwerte* x )
  {
  const int deltaphi = x->deltaphi;
  const int len = x->signalA.len;
  int* z = malloc( len * sizeof (int) );
  int v = x->A;
  int v0 = x->A0;
  long diff;
  int i;
  if( z == NULL ) error( noheap );
  for( i = 0; i < len; ++i )
    z[i] = SIN( x->winkel.data[i] );
  if( lineal( &x->A, &x->A0, len, z, x->signalA.data ) || x->A < 1 || x->A0 < 0 )
    {
    x->A = INT_MAX / 128;
    x->A0 = 128;
    }
  else
    {                            /* überrelaxieren */
    x->A += ( x->A - v ) / 2;
    x->A0 += ( x->A0 - v0 ) / 2;
    }
  diff = labs( (long) v - x->A ) + labs( (long) v0 - x->A0 );
  v = x->B;
  v0 = x->B0;
  for( i = 0; i < len; ++i )
    z[i] = SIND( x->winkel.data[i], deltaphi );
  if( lineal( &x->B, &x->B0, len, z, x->signalB.data ) || x->B < 1 || x->B0 < 0 )
    {
    x->B = INT_MAX / 128;
    x->B0 = 128;
    }
  else
    {
    x->B += ( x->B - v ) / 2;
    x->B0 += ( x->B0 - v0 ) / 2;
    }
  free( z );
  diff += labs( (long) v - x->B ) + labs( (long) v0 - x->B0 );
  return diff;
  }

/*
**  Berechnet Summe der Fehlerquadrate für deltaPhi
**    deltaphi : der Phasendiff.winkel zwischen den Signalen
**    x        : die Daten zur Berechnung
**    return   : der Funktionswerte
*/
static long LOCAL f_deltaphi ( const int deltaphi, const phasenwerte* x )
  {
  const int B = x->B;
  const int B0 = x->B0;
  const int len = x->signalA.len;
  long akku = 0l;
  int i;
  for( i = 0; i < len; ++i )
    {
    const int a = (int) x->signalB.data[i]
                  - B0 - SIND(x->winkel.data[i],deltaphi) / B;
    akku += mul32( a, a );
    }
  return akku;
  }

/*
**  Berechnet die Ableitung der Summe der Fehlerquadrate für deltaphi
**  bis auf Faktor CIRC/Pi
**    deltaphi : der Phasendiff.winkel zwischen den Signalen
**    x        : die Daten zur Berechnung
**    return   : der Ableitungswerte
*/
static long LOCAL fs_deltaphi ( const int deltaphi, const phasenwerte* x )
  {
  const int B = x->B;
  const int B0 = x->B0;
  const int len = x->signalA.len;
  long akku = 0;
  int i;
  for( i = 0; i < len; ++i )
    {
    const long phi = x->winkel.data[i] + deltaphi;
    akku += mul32( COS(phi) / B, x->signalB.data[i] - B0 - SIN(phi) / B );
    }
  return akku;
  }

/*
**  Bestimmt neuen Näherungswert für deltaPhi
**  minimiert Summe der Fehlerquadrate mit gedämpften Newton-Verfahren
**    data   : das Feld 'deltaphi' wird neu berechnet
**    return : Betrag der Änderung an deltaphi
*/
static long LOCAL calc_deltaphi ( phasenwerte* data )
  {
  int deltaphi = data->deltaphi;
  int weiter = 1;
  long f;
  f = f_deltaphi( deltaphi, data );
  do{
    const long fs = fs_deltaphi( deltaphi, data );
    if( fs != 0l )
      {
      long f2;
      int dphi;
      weiter = (int) ( ( f * CIRC/16 )/ fs );
      if( weiter == 0 ) weiter = fs > 0l ? +4 : -4;
      do{
        f2 = f_deltaphi( dphi = deltaphi + weiter, data );
        if( f <= f2 ) weiter /= 2;
        }
        while( weiter && f <= f2 );
      if( weiter )
        {
        f = f2;
        deltaphi = dphi;
        }
      }
    else
      weiter = 0;
    }
    while( weiter );
  f = labs( (long) data->deltaphi - deltaphi );
  data->deltaphi = deltaphi;
  return f;
  }

/*
**  Bestimmt Startwerte für A,A0, B,B0 und deltaphi
**  dabei wird von normierten Signalen ausgegangen
**    data : die Felder 'signalA' und 'signalB' werden benutzt
*/
static void LOCAL startwerte ( phasenwerte* data )
  {
  data->A  = data->B  = INT_MAX / 128;
  data->A0 = data->B0 = 128;
  data->deltaphi = CIRC/4;
  }

/*
**  Kontrollausgabe
**    data      : an diesen Daten wird zur Zeit gerechnet
**    iter      : Nummer der Iteration
**    grafikaus : falls != 0, Phasendiff.winkel graphisch ausgeben
*/
static void LOCAL printx ( phasenwerte* data, int iter, int grafikaus )
  {
  if( grafikaus )
    grafik_clear( 1 );
  else
    grafik_home( 0, 12 );
  printf( " Iteration : %3d     deltaPhi = %4d\n"
          " A = %3d, A0 = %3d   B = %3d, B0 = %3d\n"
          , iter, data->deltaphi
          , data->A, data->A0, data->B, data->B0 );
  if( grafikaus )
    {
    minmax( data->winkel.len, data->winkel.data,
	    &data->winkel.min, &data->winkel.max );
    grafik_grafl( 1, &data->winkel );
    }
  }

/*
**  Normiert auf Bereich 0 .. 255
**    x : zu normierender Graph
*/
static void LOCAL normiere ( grafuc* x )
  {
  const int len = x->len;
  int i;
  unsigned delta, u;
  uchar* tabelle = malloc( len * sizeof (uchar) );
  if( tabelle == NULL ) error( noheap );
  delta = (unsigned) ( x->max - x->min );
  for( i = 0, u = 0; i <= delta; ++i, u += 255 )
    tabelle[ i + x->min ] = (uchar) ( u / delta );
  for( i = 0; i < len; ++i )
    x->data[i] = tabelle[ x->data[i] ];
  free( tabelle );
  }

/*
**  Auswertung der Signale.
**    data      : die Felder 'signalA' und 'signalB' werden benutzt um
**                die fehlenden Werte zu bestimmen
**    grafikaus : falls != 0, Zwischenwerte graphisch ausgeben
*/
void _cdecl calc_auswertung ( phasenwerte* data, int grafikaus )
  {
  int i;
  long diff;
  normiere( &data->signalA );
  normiere( &data->signalB );
  startwerte( data );
  calc_phi( data );
  if( grafikaus )
    {
    grafikaus = ! grafik_open();
    if( ! grafikaus ) puts( "Grafikausgabe nicht möglich." );
    }
  i = 0;
  printx( data, i, grafikaus );
  do{
    diff = calc_AB( data );
    calc_phi( data );
    diff += calc_deltaphi( data );
    calc_phi( data );
    printx( data, ++i, grafikaus );
    }
    while( i < 70 && diff > 0 && ! stoptaste(0) );
  if( grafikaus ) grafik_close();
  minmax( data->winkel.len, data->winkel.data,
	  &data->winkel.min, &data->winkel.max );
  if( diff > 0 )
    printf( "Iteration durch %s abgebrochen\n", i >= 70 ? "Zähler" : "Benutzer" );
  }

/*
**  Berechnet aus den Winkeln Phi und den Konstanten A,A0,B,B0
**  und deltaphi die 'Signale A oder B' mal Faktor A bzw. B
**    x : die Daten
**    a : 0 -> Signal B wird berechnet, sonst Signal A
**    y : der Graph, data-Feld muß nach Gebrauch mit free gelöscht werden
*/
void _cdecl calc_signal ( int a, const phasenwerte* x, grafl* y )
  {
  const int len = x->signalA.len;
  long* data = malloc( len * sizeof (long) );
  if( data == NULL ) error( noheap );
  y->len = len;
  y->data = data;
  if( a )
    {
    int i;
    const long akku = x->A * x->A0;
    for( i = 0; i < len; ++i )
      {
      const long phi = x->winkel.data[i];
      y->data[i] = akku + SIN(phi);
      }
    }
  else
    {
    int i;
    const int deltaphi = x->deltaphi;
    const long akku = x->B * x->B0;
    for( i = 0; i < len; ++i )
      {
      const long phi = x->winkel.data[i];
      y->data[i] = akku + SIND(phi,deltaphi);
      }
    }
  minmax( len, y->data, &y->min, &y->max );
  }

