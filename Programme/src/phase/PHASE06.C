/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .04.1993  Ulrich Berntien
**
**  - Daten im internen Format speichern/laden
**  - Daten im OSZ-Format laden
**
**  08.04.1993  Beginn
**  18.08.1993  Länge der Daten variabel
*/

#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "phase.h"

/********************************************************************/

#ifdef __TURBOC__

  /*
  **  Umwandlung struct tm -> time_t
  **
  **  Berechnung der Nummer des Tages folgt Formel aus
  **    H.Karttunnen et.al.
  **    Astronomie, Eine Einführung
  **    Springer-Verlag, Berlin (1990)
  */
  static time_t LOCAL mktime ( struct tm* ntime )
    {
    time_t result;
    int g,f,A;
    if( ntime->tm_mon >= 2 )
      {
      f = ntime->tm_year + 1900;
      g = ntime->tm_mon + 2;
      }
    else
      {
      f = ntime->tm_year + 1899;
      g = ntime->tm_mon + 14;
      }
    A = 2 - (int) ( f / 100 ) + (int) ( f / 400 );
    result = 365L * f + (long) ( f / 4 ) +
	     30L * g + ( 6001L * g ) / 10000 +
	     ntime->tm_mday + A -
	     719592L;
    result *= 24;
    result += ntime->tm_hour - 17;  /* ???? */
    result *= 60;
    result += ntime->tm_min;
    result *= 60;
    result += ntime->tm_sec;
    return result;
    }

#endif

/********************************************************************/

/*
**  Anzahl der Werte pro Kanal der Gould
*/
#define LENGOULD 1008

/*
**  Buffer für Filenamen (letzter Filename wird erhalten)
*/
static char namebuffer [70];

/*
**  öfters benutzte Texte
*/
static const char fn [] = "\nFilename ?\n";
static const char semicolon [] = ";";
static const char komma [] = ",";

/*
**  Fehlermeldung mit Filename ausgeben
**    error : Meldung
**    fname : Filename
*/
static void LOCAL errorout ( const char* error, const char* fname )
  {
  printf( error, fname );
  perror( "PERROR" );
  }

/*
**  Daten eines uchar - Grafen aus File lesen (internes Format)
**    x      : dort die Daten verankern
**             Feld len muß gültig sein, Feld data wird angelegt
**    file   : File, read binary
**    return : Ist ein Fehler aufgetreten ?
*/
static int LOCAL freadgrafuc ( grafuc* x, FILE* file )
  {
  size_t size = x->len * sizeof (uchar);
  x->data = malloc( size );
  if( x->data == NULL ) error( noheap );
  return fread( x->data, size, 1, file ) != 1;
  }

/*
**  Daten eines long - Graphen aus File lesen (internes Format)
**    x      : dort die Daten verankern
**	       Feld len muß gültig sein, Feld data wird angelegt
**    file   : File, read binary
**    return : Ist ein Fehler aufgetreten ?
*/
static int LOCAL freadgrafl ( grafl* x, FILE* file )
  {
  size_t size = x->len * sizeof (long);
  x->data = malloc( size );
  if( x->data == NULL ) error( noheap );
  return fread( x->data, size, 1, file ) != 1;
  }

/*
**  Daten im internen Format aus Datei laden
**    fname  : Filename
**    data   : dorthin die Daten schreiben
**             die variabellangen Felder werden angelegt !
**    return : != 0, wenn Fehler aufgetreten
*/
static int LOCAL file_load ( const char* fname, phasenwerte* data )
  {
  FILE* file = fopen( fname, "rb" );
  int error = file == NULL;
  if( error )
    errorout( "Kann File '%s' nicht öffnen zum Lesen.\n", fname );
  else
    {
    error = fread( data, sizeof (phasenwerte), 1, file ) != 1
	    || freadgrafuc( &data->signalA, file )
	    || freadgrafuc( &data->signalB, file )
	    || freadgrafl( &data->winkel, file );
    if( error ) errorout( "Fehler beim Lesen aus File '%s'.\n", fname );
    fclose( file );
    }
  return error;
  }

/*
**  Daten in internen Format in eine Datei schreiben
**    fname  : Name des Files
**    data   : diese Daten speichern
**    return : != 0, wenn Fehler aufgetreten
*/
static int LOCAL file_save ( const char* fname, const phasenwerte* data )
  {
  FILE* file = fopen( fname, "wb" );
  int error = file == NULL;
  if( error )
    errorout( "Kann File '%s' nicht erzeugen.\n", fname );
  else
    {
    error = fwrite( data, sizeof (phasenwerte), 1, file ) != 1
	    || fwrite( data->signalA.data, data->signalA.len * sizeof (uchar), 1, file ) != 1
	    || fwrite( data->signalB.data, data->signalB.len * sizeof (uchar), 1, file ) != 1
	    || fwrite( data->winkel.data, data->winkel.len * sizeof (long), 1, file ) != 1;
    if( error ) errorout( "Fehler beim Schreibe in File '%s'.\n", fname );
    fclose( file );
    }
  return error;
  }

/** OSZ - FILE ******************************************************/

/*
**  Name eines Satzes einlesen
**  alle Sätze im OSZ-File hane die Form "name=inhalt;"
**  beim letzen Satz kann Anstelle des ';' ein '\n' stehen
**    file   : File, read
**    name   : Buffer für den Namen
**    len    : Länge des Buffers
**    return : != 0, wenn ein Name gelesen wurde
*/
static int LOCAL getname ( FILE* file, char* name, int len )
  {
  char akku;
  int i = 0;
  do name[i++] = akku = (char) fgetc( file );
  while( akku != '=' && akku != EOF && i < len );
  name[i-1] = '\0';
  if( akku == EOF && i > 1 ) puts( "EOF in einem Satznamen." );
  if( akku != '=' && i == len ) puts( "Satzname länger als Buffer." );
  return akku == '=';
  }

/*
**  Zeichen überspringen, bis ';', '\n' oder EOF
**  d.h. bis zum Anfang des nächsten Datensatzes
**    file : File, read
*/
static void LOCAL skip ( FILE* file )
  {
  int akku;
  do akku = fgetc( file ); while( akku != ';' && akku != '\n' && akku != EOF );
  }

/*
**  Liste von Strings durchsuchen
**    map    : Liste der String mit NULL abgeschlossen
**    str    : der gesuchte String
**    return : Index des String oder Index des NULL's
*/
static int LOCAL mapscan ( const char* map [], const char* str )
  {
  int i = 0;
  while( map[i] != NULL && strcmp( map[i], str ) ) ++i;
  return i;
  }

/*
**  erwarteten String aus File lesen
**  ';' am Ende kann fehlen, wenn dort Dateiende
**    file   : File, read
**    str    : der erwartet String
**    return : != 0, wenn der erwartet String nicht kam
*/
static int LOCAL match ( FILE* file, const char* str )
  {
  int error = 0;
  while( *str != '\0' && ! error )
    {
    const int akku = fgetc( file );
    error = *str != (char) akku;
    if( error && *str == ';' && akku == EOF ) error = 0;
    ++str;
    }
  return error;
  }

/*
**  Ein Signal aus OSZ-File einlesen
**    file   : File, read
**    x      : dorthin die Daten schreiben, Feld data wird angelegt
**    return : Ist ein Fehler aufgetreten ?
*/
static int LOCAL readgrafuc ( FILE* file, grafuc* x )
  {
  int i;
  int errflag;
  int max = 0x00;
  int min = 0xff;
  uchar* data = malloc( LENGOULD * sizeof (uchar) );
  if( data == NULL ) error( noheap );
  x->data = data;
  x->len = LENGOULD;
  errflag = match( file, "#H" );
  if( errflag )
    puts( "Massendaten nur in Hex erlaubt." );
  else
    {
    for( i = 0; i < LENGOULD && ! errflag; ++i )
      {
      int j;
      int akku = 0;
      for( j = 0; j < 2 && ! errflag; ++j )
        {
        int x = fgetc( file );
	errflag = ! isxdigit( x );
        akku = akku * 16 + x - ( x > '9' ? 'A' - 10 : '0' );
        }
      data[i] = (uchar) akku;
      if( akku > max ) max = akku;
      if( akku < min ) min = akku;
      errflag = match( file, i+1 == LENGOULD ? semicolon : komma );
      }
    if( errflag ) puts( "Fehler in den Massendaten." );
    }
  x->max = (uchar) max;
  x->min = (uchar) min;
  return errflag;
  }

/*
**  Fließkommazahl aus OSZ-File lesen (mit ';' abgeschlossen)
**    file   : File, read
**    return : die gelesen Zahl, oder 0.0 wenn nicht lesbar
*/
static float LOCAL readfloat ( FILE* file )
  {
  float akku;
  if( fscanf( file, "%e", &akku ) != 1 || match( file, semicolon ) )
    akku = (float) 0;
  return akku;
  }

/*
**  Datum aus OSZ-File einlesen
**    file : File, read
**    ptr  : dort wird das Datum gespeichert
*/
static void LOCAL readdate ( FILE* file, struct tm* ptr )
  {
  fscanf( file, "%d.%d.%d", &ptr->tm_mday, &ptr->tm_mon, &ptr->tm_year );
  ptr->tm_mon -= 1;
  ptr->tm_year -= 1900;
  match( file, semicolon );
  }

/*
**  Zeit aus OSZ-File lesen
**    file : File, read
**    ptr  : dort wird die Zeit gespeichert
*/
static void LOCAL readtime ( FILE* file, struct tm* ptr )
  {
  fscanf( file, "%d:%d", &ptr->tm_hour, &ptr->tm_min );
  match( file, semicolon );
  }

/*
**  String aus OSZ-Files lesen (mit ';' abgeschlossen)
**    file   : File, read
**    buffer : Buffer für den String
**    len    : länge des Buffers
*/
static void LOCAL readkomment ( FILE* file, char* buffer, int len )
  {
  int i = 0;
  int akku;
  do buffer[i++] = (char) ( akku = fgetc( file ) );
  while( i < len && akku != ';' && akku >= 0x20 );
  if( akku != ';' ) skip( file );
  buffer[i-1] = '\0';
  }

/*
**  Daten im OSZ-Format aus einer Datei lesen
**    fname  : der Filename
**    data   : dorthin die Daten schreiben
**    kanal  : die Kanäle (1..4) mit den Signalen
**    return : != 0, wenn Fehleraufgetreten ist
*/
static int LOCAL file_osz ( const char* fname, phasenwerte* data, int kanal [2] )
  {
  static char* namemap [] =
    { "TRC?A", "TRC?A", "TRHS?A", "TRHS?A", "DATE", "TIME", "SCHUSSNR", NULL };
  FILE* file = fopen( fname, "rb" );
  int error = file == NULL;
  if( error )
    errorout( "Kann OSZ-File '%s' nicht öffnen zum Lesen.\n", fname );
  else
    {
    char buffer [10];
    int i;
    int errora = 1;
    int errorb = 1;
    float timea = (float) 0;
    float timeb = (float) 0;
    struct tm datetime;
    memset( &datetime, 0, sizeof datetime );
    data->komment[0] = '\0';
    setvbuf( file, NULL, _IOFBF, 4 * 1024 );
    for( i = 0; i < 2; ++i )
      namemap[i+2][4] = namemap[i][3] = (char) ( '0' + kanal[i] );
    while( getname( file, buffer, sizeof buffer ) && ! error )
      {
      switch( mapscan( namemap, buffer ) )
        {
        case 0  : errora = error = readgrafuc( file, &data->signalA );
                  break;
        case 1  : errorb = error = readgrafuc( file, &data->signalB );
                  break;
        case 2  : timea = readfloat( file );
                  break;
        case 3  : timeb = readfloat( file );
                  break;
        case 4  : readdate( file, &datetime );
                  break;
        case 5  : readtime( file, &datetime );
                  break;
        case 6  : readkomment( file, data->komment, sizeof data->komment );
                  break;
        default : skip( file );
        }
      }
    if( errora || errorb )
      {
      puts( "Signale konnten nicht eingelesen werden." );
      error = 1;
      }
    data->timebase = timea != timeb ? (float) 0 : timea;
    data->aufnahme = mktime( &datetime );
    if( data->komment[0] == '\0' ) strcpy( data->komment, "OSZ-FILE" );
    fclose( file );
    }
  return error;
  }

/** DIALOGE *********************************************************/

/*
**  Dialog, Laden Daten im internen Format
**    data   : dorthin die Daten speichern
**    return : != 0, wenn Fehler aufgetreten
*/
int _cdecl file_dload ( phasenwerte* data )
  {
  int error;
  grafik_headline( "File laden" );
  puts( fn );
  if( readstring( sizeof namebuffer, namebuffer ) > 1 )
    {
    putchar( '\n' );
    error = file_load( namebuffer, data );
    if( error ) waitkey();
    }
  else
    error = 1;
  return error;
  }

/*
**  Dialog, Laden Daten im OSZ-Format
**    data   : dorthin die Daten speichern
**    kanal  : Nummer der beiden Kanäle mit den Signalen
**    return : != 0, wenn Fehler aufgetreten
*/
int _cdecl file_dosz ( phasenwerte* data, int kanal [2] )
  {
  grafik_headline( "OSZ-File einlesen" );
  puts( fn );
  readstring( sizeof namebuffer, namebuffer );
  putchar( '\n' );
  return file_osz( namebuffer, data, kanal );
  }

/*
**  File vor überschreiben sichern
**  falls File existiert, frage Benutzer
**    fname  : Name des Files
**    return : != 0, File darf nicht überschrieben werden
*/
static int LOCAL sichere ( const char* fname )
  {
  int result = access( fname, 0 );
  if( result == 0 )
    {
    printf( "File '%s' existiert bereits, überschreiben", fname );
    result = ! janein();
    }
  return result;
  }

/*
**  Dialog, Speichern Daten im internen Format
**    data : diese Daten speichern
*/
void _cdecl file_dsave ( const phasenwerte* data )
  {
  int nameda;
  int verboten;
  grafik_headline( "File schreiben" );
  do{
    puts( fn );
    nameda = readstring( sizeof namebuffer, namebuffer ) > 1;
    if( nameda ) verboten = sichere( namebuffer );
    }
    while( nameda && verboten );
  if( nameda )
    {
    putchar( '\n' );
    if( file_save( namebuffer, data ) ) waitkey();
    }
  }

