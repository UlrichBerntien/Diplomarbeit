/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .03.1993  Ulrich Berntien
**
**  Mach-Zehnder-Interferometer Auswertung mit 2 - Phasendetektion
**
**   . Daten aus zwei Kanälen der Gould lesen
**   . Daten aus OSZ-File einlesen
**   . Umwandlung Amplitudeninformation -> Phasendifferenzwinkelinformation
**   . Daten zum Plotten in File schreiben
**   . Daten auf Bildschirm graphisch / tabellarisch darstellen
**   . Daten in File schreiben / aus File lesen
**
**  30.03.1993  Beginn, entstanden aus PHASE1.C (1-Phasendetektion)
**  07.04.1993  Module PHASE04.C und PHASE05.C abgespalten
**  08.04.1993  Modul PHASE06.C aufgenommen
**  18.08.1993  die Längen der Datenfelder variabel gemacht
*/

#include <math.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "phase.h"

/*
**  Kurzanleitung
*/
static const char manual0 [] =
  "PHASE2   .03.1993  Ulrich Berntien\n"
  "Auswertung 2-Phasendetektion Mach-Zehnder-Interferometer.\n"
  "Lesen der Signale von der Gould über serielle Schnittstelle.\n"
  "Ausgabe der Signale und der Phasenverschiebung grafisch auf\n"
  "Monitor oder über das Programm TGPLOT.C auf HP-LaserJet\n";
static const char manual1 [] =
  "Aufrufformat :\n\n"
  "    PHASE1 <KanalA> <KanalB> <Plotfile>\n\n"
  "<KanalA>    Nummer des Kanal (1-4) der Gould mit dem Signal\n"
  "<KanalB>       -\"-    mit dem 90° phasenverschobenem Signal\n"
  "<Plotfile>  Name der Datei, an die Daten für TGPLOT angehängt werden.\n";

/*
**  Textkonstanten für Fehlermeldungen
*/
const char* txterr = "PHASE2 Fehler : ";
const char* noheap = "Out of Heap";

/** EINGABE ********************************************************/

/*
**  Simulation von Signalen
**    data : die Felder 'signalA' und 'signalB' werden ausgefüllt
**           die data - Felder werden angelegt
**  return : == 0
*/
static int LOCAL simuliere ( phasenwerte* data )
  {
  const int len = 2000;
  const int offset = 128;
  const int ampl = 100;
  const double pi = 4 * atan( 1.0 );
  const double step = pi / 360;
  double x = (double) rand() / 100.0;
  double dx = (double) rand() / 1000.0 + 90.0;
  int i;
  grafik_headline( "Simulation" );
  data->signalA.data = malloc( len * sizeof (uchar) );
  data->signalB.data = malloc( len * sizeof (uchar) );
  if( data->signalA.data == NULL || data->signalB.data == NULL ) error( noheap );
  data->signalA.len = data->signalB.len = len;
  printf( "Startwinkel = %e\n", x );
  printf( "DPhi = %e\n", dx );
  dx *= pi/180;
  x *= pi/180;
  for( i = 0; i < len; ++i, x += step )
    {
    data->signalA.data[i] = (uchar) ( offset + ampl * sin( x ) );
    data->signalB.data[i] = (uchar) ( offset + ampl * sin( x + dx ) );
    }
  data->signalA.max =
  data->signalB.max = (uchar) ( offset + ampl );
  data->signalA.min =
  data->signalB.min = (uchar) ( offset - ampl );
  return 0;
  }

/*
**  Legt Feld für die zu berechnenden Phasenwinkel an
**    x : darin wird das Feld angelegt
*/
static void LOCAL winkelfeld ( phasenwerte* x )
  {
  if( x->signalA.len != x->signalB.len )
    error( "Anzahl der Werte in den Signalen ist unterschiedlich" );
  x->winkel.data = malloc( x->signalA.len * sizeof (long) );
  if( x->winkel.data == NULL ) error( noheap );
  x->winkel.len = x->signalA.len;
  }

/*
**  Signale von Gould lesen und auswerten
**    kanal     : Kanalnummer des Signals und des um 90°
**                phasenverschobenen Signals auf der Gould
**    data      : dort die Daten speichern
**    grafikaus : != 0, dann Zwischenergebnisse der Auswertung als Graphik
**    return    : != 0, wenn ein Fehler auftrat
*/
static int LOCAL einlesen ( int kanal [2], phasenwerte* data, int grafikaus )
  {
  int error;
  grafik_headline( "Signale lesen" );
  printf( "Signal von Gould Kanäle %d und %d lesen\n", kanal[0], kanal[1] );
  time( &data->aufnahme );
  error = gould_lock();
  if( ! error )
    {
    error = gould_read( kanal[0], &data->signalA );
    if( error ) puts( "Signal A konnte nicht gelesen werden." );
    }
  if( ! error )
    {
    error = gould_read( kanal[1], &data->signalB );
    if( error ) puts( "Signal B konnte nicht gelesen werden." );
    }
  if( ! error )
    {
    strcpy( data->komment, "direkt" );
    data->timebase = gould_timebase( kanal[0] );
    }
  gould_unlock();

/** error = simuliere( data ); **/

  if( error )
    waitkey();
  else
    {
    winkelfeld( data );
    calc_auswertung( data, grafikaus );
    }
  return error;
  }

/*
**  Signale aus OSZ-File lesen und auswerten
**    kanal      : die Nummern der beiden Knäle
**    data      : dort die Daten speichern
**    grafikaus : != 0, dann Zwischenergebnissse der Auswertung als Grafik
**    return    : != 0, wenn Fehler auftrat
*/
static int LOCAL oszeinlesen ( int kanal [2], phasenwerte* data, int grafikaus )
  {
  int error = file_dosz ( data, kanal );
  if( error )
    waitkey();
  else
    {
    winkelfeld( data );
    calc_auswertung( data, grafikaus );
    }
  return error;
  }

/*
**  Befehl an das Betriebssystem absetzen
*/
static void LOCAL syscall ( void )
  {
  char* cmd = getenv( "COMSPEC" );
  if( cmd == NULL ) cmd = "COMMAND.COM";
  grafik_headline( "Betriebssystem" );
  if( spawnlp( P_WAIT, cmd ,cmd, NULL ) < 0 )
    {
    printf( "Kann '%s' nicht starten.\n", cmd );
    perror( "PERROR" );
    waitkey();
    }
  }

/*
**  Angelegte Datenfelder löschen
**    x : die variabel-lange Felder darin werden gelöscht
*/
static void LOCAL loesche ( phasenwerte* x )
  {
  free( x->signalA.data );
  x->signalA.data = NULL;
  x->signalA.len = 0;
  free( x->signalB.data );
  x->signalB.data = NULL;
  x->signalB.len = 0;
  free( x->winkel.data );
  x->winkel.data = NULL;
  x->winkel.len = 0;
  }

/*
**  Menü
**   kanal : Nummern der Kanäle der Gould mit den Signalen
*/
static void LOCAL menu ( int kanal [2] )
  {
  static const char grafik[] = "keine Grafik bei der Rechnung (4)";
  static const char nografik[] = "Grafik bei der Rechnung (4)";
  static const char punkt6[] = "Tabelle ausgeben (5)";
  static const char* menu [] =
    {
    "DOS Shell",
    "Ende (0)",
    "Daten einlesen (1)",
    "OSZ-File einlesen (2)",
    "Daten aus File laden (3)",
    grafik,
    NULL,
    "Grafik ausgeben (6)",
    "in Plotfile schreiben (7)",
    "    -\"- mit Fitkurven (8)",
    "Vergleich Signal - Fitkurve (9)",
    "Ausgeben der Parameter (10)",
    "Daten in File speichern (11)",
    NULL
    };
  int wahl = 2;
  int datenok = 0;
  int grafikaus = 0;
  phasenwerte daten;
  do{
    grafik_headline( "P H A S E 2" );
    wahl = grafik_menu( menu, wahl );
    switch( wahl )
      {
      case  0 : syscall();
                break;
      case  1 : puts( "Programmende." );
		break;
      case  2 :
      case  3 :
      case  4 : if( datenok ) loesche( &daten );
		switch( wahl )
		  {
		  case 2 : datenok = ! einlesen( kanal, &daten, grafikaus );
			   break;
		  case 3 : datenok = ! oszeinlesen( kanal, &daten, grafikaus );
			   break;
		  case 4 : datenok = ! file_dload( &daten );
		  }
		if( ! datenok ) loesche( &daten );
                menu[6] = datenok ? punkt6 : NULL;
		break;
      case  5 : grafikaus = ! grafikaus;
                menu[5] = grafikaus ? nografik : grafik;
                break;
      case  6 : ausgabe_tabelle( &daten );
                break;
      case  7 : ausgabe_grafik( &daten );
                break;
      case  8 : ausgabe_plotfile( &daten, 0 );
                break;
      case  9 : ausgabe_plotfile( &daten, 1 );
                break;
      case 10 : ausgabe_doppelt( &daten );
                break;
      case 11 : ausgabe_parameter( &daten );
                break;
      case 12 : file_dsave( &daten );
                break;
      }
    }
    while( wahl != 1 );
  loesche( &daten );
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  int i;
  int kanal[2];
  if( argc != 4 ) verror( "%s%s", manual0, manual1 );
  for( i = 0; i < 2; ++i )
    {
    kanal[i] = atoi( argv[1+i] );
    if( kanal[i] < 1 || kanal[i] > 4 )
      verror( "%sungültige Kanal Nummer %d\n", txterr, kanal[i] );
    }
  setbuf( stdout, NULL );
  plotfile_init( argv[3] );
  calc_init();
  menu( kanal );
  return 0;
  }

