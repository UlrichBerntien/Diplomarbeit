/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  Ausgaben auf Monitor und in Plot-Datei
**
**  07.04.1993  abgespalten von PHASE2.C
**  18.08.1993  Längen der Daten variabel
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "phase.h"

/*
**  Aufnahmezeit als String
**    zeit   : die Aufnahme-Zeit
**    buffer : Buffer für den Text
**    return : != 0, wenn Zeit verfügbar
*/
static int LOCAL aufnahmezeit ( time_t zeit, char* buffer )
  {
  struct tm* local = localtime( &zeit );
  const int ok = local != NULL;
  if( ok )
    sprintf( buffer, "Messung vom %2d.%02d.%4d um %2d:%02d",
             local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
             local->tm_hour, local->tm_min );
  return ok;
  }

/*
**  aus minmimalem und maxlimalem Phasendifferenz-Winkel die
**  optische Wegdifferenz bestimmen für einen roten HeNe-Laser
**    winkel : Phasendifferenzwinkel
**    return : opt. Wegdifferenz in m
*/
static float LOCAL deltam ( const grafl* winkel )
  {
  return (float) ( winkel->max - winkel->min ) * (632.8e-9 / CIRC);
  }

/*
**  Schreiben des Signals- und der Phasendiffernzwerte ins Plotfile
**    x     : die zu schreibenden Werte
**    calc  : != 0, auch rückgerechneten a(j),b(j) zum Vergleich ausgeben
*/
void _cdecl ausgabe_plotfile ( const phasenwerte* x, int calc )
  {
  char buffer[30];
  int delta;
  double prodiv;
  plotfile_ff();
  grafik_headline( "Ausgabe in Plotfile" );
  if( aufnahmezeit( x->aufnahme, buffer ) )
    plotfile_printf( "%s     # %s", buffer, x->komment );
  else
    plotfile_printf( "Messung # %s", x->komment );
  plotfile_printf( "Signal A normiert, A1 = %d, A0 = %d", INT_MAX/x->A, x->A0 );
  plotfile_graf( x->signalA.len, x->signalA.data );
  plotfile_komment( "Kommentar zum Signal :" );
  if( calc )
    {
    grafl y;
    calc_signal( 1, x, &y );
    plotfile_printf( "Fitsignal A, berechnet aus A,A0,Phi" );
    plotfile_grafl( &y );
    free( y.data );
    }
  plotfile_printf( "Signal B normiert, B1 = %d, B0 = %d", INT_MAX/x->B, x->B0 );
  plotfile_graf( x->signalB.len, x->signalB.data );
  plotfile_komment( "Kommentar zum 90° verschobenen Signal :" );
  if( calc )
    {
    grafl y;
    calc_signal( 0, x, &y );
    plotfile_printf( "Fitsignal B, berechnet aus B,B0,Phi,deltaPhi" );
    plotfile_grafl( &y );
    free( y.data );
    }
  plotfile_printf( "Phasendifferenzwinkel, deltaPhi = %9.2e°", x->deltaphi INGRAD );
  delta = plotfile_grafl( &x->winkel );
  prodiv = ( ( x->winkel.max INGRAD - x->winkel.min INGRAD ) * 28 )/ delta;
  plotfile_printf( "VER %9.2e° pro DIV. ( bei HeNe : Ampl. %9.2e m )",
                   prodiv, deltam( &x->winkel ) );
  if( x->timebase > (float) 0 )
    plotfile_printf( "HOR %9.2e sec pro DIV.", x->timebase );
  plotfile_komment( "Kommentar zum Phasensignal :" );
  }

/*
**  Ausgeben der beiden Signal als Grafik auf Monitor
**  Monitor muß bereits in Grafikmode geschaltet sein
**    x : die Daten
*/
void _cdecl ausgabe_signale ( const phasenwerte* x )
  {
  grafik_puts( 2, 1, "Signal A" );
  grafik_grafuc( 1, &x->signalA );
  grafik_puts( 2,17, "Signal B" );
  grafik_grafuc( 0, &x->signalB );
  }

/*
**  Ausgabe des Signal und der Phasendifferenz als Grafik auf Monitor
**    x : die Daten
*/
void _cdecl ausgabe_grafik ( const phasenwerte* x )
  {
  if( ! grafik_open() )
    {
    char buffer[40];
    ausgabe_signale( x );
    waitkey();
    grafik_cls();
    sprintf( buffer, "Delta %11.3e m", deltam( &x->winkel ) );
    grafik_puts( 2, 1, buffer );
    grafik_grafl( 1, &x->winkel );
    waitkey();
    grafik_close();
    }
  }

/*
**  Ausgabe gemessene Signale und gerechnete Signale im Vergleich
**    x : die auszugebenden Daten
*/
void _cdecl ausgabe_doppelt( const phasenwerte* x )
  {
  if( ! grafik_open() )
    {
    int i;
    ausgabe_signale( x );
    for( i = 0; i < 2; ++i )
      {
      grafl y;
      calc_signal( i, x, &y );
      grafik_grafl( i, &y );
      free( y.data );
      }
    waitkey();
    grafik_close();
    }
  }

/*
**  Ausgabe des Informationskopfes auf Bildschirm
**    x : die auszugebenden Daten
*/
static void LOCAL kopf ( const phasenwerte* x )
  {
  char buffer [60];
  if( aufnahmezeit( x->aufnahme, buffer ) ) fputs( buffer, stdout );
  printf( "    # %s\n", x->komment );
  if( x->timebase > (float) 0 )
    printf( "Zeitbasis : %9.2e sec / DIV.\n", x->timebase );
  else
    putchar( '\n' );
  }

/*
**  Signal und Phasendifferenzwinkel in Tabelle ausgeben
**    x : die auszugebenden Daten
*/
void _cdecl ausgabe_tabelle ( const phasenwerte* x )
  {
  int i;
  grafl a, b;
  calc_signal( 1, x, &a );
  calc_signal( 0, x, &b );
  grafik_headline( "Tabellenausgabe" );
  kopf( x );
  puts( "\n   j  |  a[j]  | Rechn. |  b[j]  | Rechn. | Winkel °\n"
            "------+--------+--------+--------+--------+-------------" );
  for( i = 0; i < a.len && ! stoptaste(1); ++i )
    {
    if( ( i & 15 ) == 0 ) grafik_home( 0,5 );
    printf( " %4d |    %3d |%7ld |    %3d |%7ld | %11.3e\n",
            i,
	    x->signalA.data[i], a.data[i] / x->A,
	    x->signalB.data[i], b.data[i] / x->B,
            x->winkel.data[i] INGRAD );
    }
  free( b.data );
  free( a.data );
  }

/*
**  Ausgeben der Parameter auf Monitor
**    data : daraus die Parameter ausgeben
*/
void _cdecl ausgabe_parameter ( const phasenwerte* x )
  {
  grafik_headline( "Parameter" );
  kopf( x );
  printf( "\nA = %4d    A0 = %4d      INT_MAX/A = A1 = %4d\n\n"
	  "B = %4d    B0 = %4d      INT_MAX/B = B1 = %4d\n\n"
          "deltaPhi = %4d   = %9.3e°\n\n"
	  , x->A, x->A0, INT_MAX / x->A
	  , x->B, x->B0, INT_MAX / x->B
          , x->deltaphi, x->deltaphi INGRAD );
  waitkey();
  }

