/*
**  C    ( Zortech C V2.1 / Micrsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .10.1992
**
**  Auswerten der Spektren
**  Werte aus dem Steuerfile lesen
**
**  13.06.1993  aus iris04.c gelöst
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iris.h"
#include "iris04.h"

/********************************************************************/

/*
**  File mit Konstanten für die Auswertung
**  Das File ist ein einfaches Textfile
**  Zeilen die mit '%' beginnen sind Kommentarzeilen
**   Zeile|        Inhalt
**  ------+--------------------------------------------------------------------
**    1   | Dateiname für das Protokoll der Auswertung
**    2   | Dateiname für die Plotdaten der n(x) und T(x) Plots
**    3   | String "..." mit Umschaltcode des Druckers auf fette Schrift
**    4   | String "..." mit Umschaltcode des Druckes auf normale Schrift
**    5   | String "R*" oder "I*", Suchmethode nach der Halbwertsbreite
**    6   | String "A*" oder "M*", Methode des Festlegens der Liniengrenzen
**    7   | Wellenlängedifferenz [nm] zwischen zwei Pixel
**    8   | Tabelle für Temperatur Bestimmung
**        | Temperatur [K]    Wert H alpha    Wert H beta   Wert H gamma
**        | die Werte sind Linienintensität / Kontinuumsintensität in 10 nm
**    x   | Tabelle mit 'ende' abgeschlossen
**    y   | die 'fixtemperatur' in K: falls Wert > 0.0 dann wird diese Temp.
**        | bei der Berechnung der Dichte benutzt, sonst wird die oben
**        | berechnete Temperatur benutzt
**    z   | 3 Tabellen zur Berechnung der Dichte aus der Starkverbreiterung
*/

TEXT parameterfile [] = "IRIS_SP.TXT";

int initok = 0;         /* != 0 wenn Modul nicht initialisiert */

char* logfile;                 /* Name des Files fürs Protokol */
char* plotfile;            /* Name des Files für die n,T Plots */

double nm_pixel;      /* Wellenlängendiff. zwischen zwei Pixel */
double fixtemperatur;           /* s.o. Wert aus Parameterfile */
char* halb_methode;    /* Suchmethode nach der Halbwertsbreite */
char* limit_methode;    /* Methode Festlegen der Liniengrenzen */

char* printerfett;           /* Steuercodes für Druckerschrift */
char* printernormal;

const tratio* ptratio;

const starkdaten* starktabellen;
          /* Zeiger auf Array mit 3 starkdaten für alpha, beta, gamma */

TEXT ende [] = "ende";

/********************************************************************/

/*
**  eine Zeile aus File lesen, dabei
**    das '\n' am Ende der Zeile wird gelöscht
**    leere Zeilen und Zeilen mit '%' am Anfang werden ignoriert
**  file   : File, read
**  buffer : Buffer für die Eingabezeile
**  size   : Größe des Buffers in Bytes
**  return : alles ok ?
*/
static int LOCAL readline ( FILE* file, char* buffer, size_t size )
  {
  int ok;
  do
    ok = fgets( buffer, size, file ) == buffer;
  while( ok && ( buffer[0] == '\n' || buffer[0] == '%' ) );
  if( ! ok )
    puts( "Fehler beim lesen aus File" );
  else
    {
    int i = 0;
    while( buffer[i] != '\0' && buffer[i] != '\n' ) ++i;
    ok = buffer[i] == '\n';
    buffer[i] = '\0';
    if( ! ok )
      printf( "Zeile im Parameterfile zu lang :\n%s  ...\n", buffer );
    }
  return ok;
  }

/*
**  Lesen Fließkommazahlen aus einem String, die Zahlen sind durch whitespaces
**    getrennt
**  buffer : Daraus die Fließkommazahlen lesen, Inhalt wird verändert (!)
**  c      : Anzahl der zu lesenden Zahlen
**  x      : Dort die Zahlen speichern
**  return : alles ok ?
*/
static int LOCAL scandoubles ( char* buffer, int c, double* x )
  {
  int ok = 1;
  while( ok && c > 0 )
    {
    while( *buffer != '\0' && *buffer <= 0x20 ) ++buffer;
    ok = *buffer != '\0';
    if( ok )
      {
      char* ptr = buffer;
      while( *ptr != '\0' && *ptr > 0x20 ) ++ptr;
      *ptr = '\0';
      ok = sscanf( buffer, "%lg", x ) == 1;
      buffer = ptr + 1;
      }
    if( ok )
      {
      ++x;
      --c;
      }
    }
  if( ! ok )
    printf( "Erwarte noch %d Zahlen :\n%s\n", c, buffer );
  return ok;
  }

/*
**  Einlesen einer Fließkommazahl aus dem File
**  file   : File, read
**  x      : dort die Fleißkommazahl speichern
**  return : alles ok ?
*/
static int LOCAL readdouble ( FILE* file, double* x )
  {
  char buffer [200];
  int ok = readline( file, buffer, sizeof buffer )
           && scandoubles( buffer, 1, x );
  return ok;
  }

/*
**  Lesen einer Zeile die einen String enthält
**  file   : File, read
**  string : darüber wird der neuangelegte String zurückgebenen
**  return : alles ok ?
*/
static int LOCAL readstring ( FILE* file, char** string )
  {
  char buffer [200];
  int ok = readline( file, buffer, sizeof buffer );
  if( ok )
    {
    const char* ptr = buffer;
    while( *ptr == 0x20 || *ptr < 0 ) ++ptr;
    ok = ( *string = strdup( ptr ) ) != NULL;
    if( ! ok ) errmalloc();
    }
  return ok;
  }

/*
**  Einlesen der Tabelle mit den Temperaturen und den Verhältnissen
**  file   : File, read
**  pptr   : darüber wird die neuangelegte Tabelle zurückgegeben
**  return : alles ok ?
*/
static int LOCAL readratios ( FILE* file, tratio** pptr )
  {
  tratio* ptr;
  int ok = ( ptr = malloc( sizeof (tratio) ) ) != NULL;
  *pptr = ptr;
  if( ok )
    {
    char buffer [200];
    double in [4];
    int now = 0;
    int weiter = 1;
    while( ok && weiter && now < tratiolen )
      {
      ok = readline( file, buffer, sizeof buffer );
      if( ok && ( weiter = strcmp( buffer, ende ) ) != 0 )
        {
        ok = scandoubles( buffer, 4, in );
        if( ! ok )
          puts( "Erwarte eine Zeile der Temperturtabelle" );
        else
          {
          ptr->t[now] = in[0];
          ptr->alpha[now] = in[1];
          ptr->beta[now]  = in[2];
          ptr->gamma[now] = in[3];
          ++now;
          }
        }
      }
    if( ok && weiter && now >= tratiolen )
      {
      puts( "Liste mit den Temperaturen ist zu lang." );
      ok = 0;
      }
    ptr->count = ok ? now : 0;
    }
  else
    errmalloc();
  return ok;
  }

/*
**  Einlesen einer Liste mit Fließkommazahlen
**  file   : File, read
**  count  : maximale Zahl der Zahlen / Rückgabe der gelesenen Anzahl
**  x      : dort die gelesenen Zahlen speichern
**  return : alles ok ?
*/
static int LOCAL readliste ( FILE* file, int* count, double* x )
  {
  char buffer [200];
  int ok = readline( file, buffer, sizeof buffer );
  int i = 0;
  while( ok && strcmp( buffer, ende ) && i < *count )
    {
    ok = scandoubles( buffer, 1, x );
    if( ok )
      {
      ++x;
      ++i;
      ok = readline( file, buffer, sizeof buffer );
      }
    }
  if( i >= *count )
    {
    printf( "Maximal %d Zahlen in der Liste\n", *count );
    ok = 0;
    }
  *count = i;
  return ok;
  }

/*
**  Einlesen der Werte einer Line in die 'starkdaten' struktur
**  file   : File, read
**  data   : dort die gelesenen Werte speichern
**  return : alles ok ?
*/
static int LOCAL readstarkdata ( FILE* file, starkdaten* data )
  {
  int ok = 1;
  data->ctemperaturen = stlen;
  ok = readliste( file, &data->ctemperaturen, data->temperaturen );
  if( ok )
    {
    int i = 0;
    double x[stlen+1];
    char buffer [200];
    ok = readline( file, buffer, sizeof buffer );
    while( ok && strcmp( buffer, ende ) )
      {
      ok = i < stlen;
      if( ok )
        ok = scandoubles( buffer, data->ctemperaturen+1, x );
      else
        puts( "Zuviele Zeilen in Tabelle" );
      if( ok )
        {
        data->dichten[i] = x[0];
        memcpy( data->c + i*stlen, x + 1, stlen * sizeof (double) );
        ++i;
        ok = readline( file, buffer, sizeof buffer );
        }
      }
    if( ! ok )
      puts( "Wollte aus Tabelle mit den C(Ne,Te) lesen" );
    data->cdichten = i;
    }
  else
    puts( "Wollte Dichten/cm^-3 einlesen" );
  return ok;
  }

/*
**  Einlesen der Daten für die Starkverbreiterungs-Auswertung
**    alle drei Linien (alpha,beta,gamma) werden eingelesen
**  file   : File, read
**  return : alles ok ?
*/
static int LOCAL readstarks ( FILE* file )
  {
  int ok = ( starktabellen = malloc( 3 * sizeof (starkdaten) ) ) != NULL;
  if( ! ok )
    errmalloc();
  else
    {
    ok = readstarkdata( file, (starkdaten*) starktabellen );
    if( ! ok ) puts( "bei der Alpha - Linie" );
    }
  if( ok)
    {
    ok = readstarkdata( file, (starkdaten*) starktabellen + 1 );
    if( ! ok ) puts( "bei der Beta - Linie" );
    }
  if( ok )
    {
    ok = readstarkdata( file, (starkdaten*) starktabellen + 2 );
    if( ! ok ) puts( "bei der Gamma - Linie" );
    }
  return ok;
  }

/*
**  Zwei Zeichen als Hexziffer interpretiern
**  z1     : erste Hexziffer (niederwertige)
**  z2     : zweite Hexziffer (höherwertige)
**  wert   : dort den Wert der Hexzahl speichern
**  return : alles ok ?
*/
static int LOCAL hex ( int z1, int z2, char* wert )
  {
  int ok;
  int akku;
  z1 = tolower( z1 );
  z2 = tolower( z2 );
  akku = z1 - '0';
  if( akku > 9 ) akku = z1 - 'a' + 10;
  ok = akku > -1 && akku < 16;
  *wert = (char) akku;
  akku = z2 - '0';
  if( akku > 9 ) akku = z2 - 'a' + 10;
  ok = ok && akku > -1 && akku < 16;
  *wert = *wert * 16 + (char) akku;
  if( ! ok )
    puts( "Fehler beim Einlesen einer Hex-Zahl" );
  return ok;
  }

/*
**  Einlesen der Steuercodes für Drucker
**  file   : File, read
**  str    : dort den neuangelegten String mit den Steuercode zurückgeben
**  return : alles ok ?
*/
static int LOCAL readcodes ( FILE* file, char** str )
  {
  char buffer [200];
  char* dest = buffer;
  const char* source = buffer;
  int ok = readline( file, buffer, sizeof buffer );
  if( ok )
    {
    while( *source < 0 || *source == 0x20 )  ++source;
    ok = *source++ == '"';
    }
  if( ok )
    {
    while( ok && *source != '"' && *source != '\0' )
      {
      if( source[0] == '\\' )
        if( source[1] == 'x' )
          {
          ok = hex( source[2] ,source[3], dest++ );
          source += 4;
          }
        else
          {
          *dest++ = *source++;
          if( *source != '\0' ) *dest++ = *source++;
          }
      else
        *dest++ = *source++;
      }
    *dest = '\0';
    ok = *source == '"';
    }
  if( ok )
    ok = ( *str = strdup( buffer ) ) != NULL;
  if( !ok )
    puts( "Kann Druckersteuercodes nicht lesen" );
  return ok;
  }

/*
**  Initialisierung des Moduls 'iris04.c'
**  Setzt Globale Variablen   initok, logfile, ptratio, ...
*/
void doinit ( void )
  {
  FILE* input = fopen( parameterfile, "rt" );
  int ok = input != NULL;

  if( ok )
    printf( "Lese Parameterfile '%s'\n", parameterfile );
  else
    printf( "Das Parameterfile '%s' kann nicht geöffnet werden\n", parameterfile );
  ok = ok  && readstring( input, &logfile )
           && readstring( input, &plotfile )
           && readcodes( input, &printerfett )
           && readcodes( input, &printernormal )
           && readstring( input, &halb_methode )
           && readstring( input, &limit_methode )
           && readdouble( input, &nm_pixel )
           && readratios( input, (tratio**) &ptratio )
           && readdouble( input, &fixtemperatur )
           && readstarks( input );
  fclose( input );
  initok = ok;
  }

