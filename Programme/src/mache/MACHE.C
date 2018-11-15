/*
**  C  ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  .06.1993 Ulrich Berntien
**
**  Ausführen einer Art Batch-Datei mit Benutzerabfragen.
**
**  19.06.1993  Beginn
*/

#include <conio.h>
#include <ctype.h>
#include <process.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char manual [] =
  "MACHE    .06.1993    Ulrich Berntien\n"
  "Aufrufformat\n\n"
  "   MACHE [Schalter]  <Filenames>\n\n"
  "<Filenames> Ausführen dieser Files\n"
  "            Standarterweiterung ist '.MAT'. Die Files werden\n"
  "            gesucht in aktuellem Pfad, Aufrufpfad und die\n"
  "            Enviroment Variablen MACHEAPATH und PATH\n"
  "mögliche Schalter:\n"
  "  -d<outfile>  Befehle nicht ausführen, nur in File schreiben\n"
  "               Das File wird dafür neu angelegt\n"
  "  -D<outfile>  Befehle nicht ausführen, nurt in File schreiben\n"
  "               An ein bestehendes File wird angehängt\n"
  "  -d oder -D   Aufheben des Schalters -d<outfile> doer -D<outfile>\n"
  "\n"
  "Aufbau des Befehls-Files (MAT-File):\n"
  "Jede Zeile kann einen Befehl enthalten.\n"
  "Eine Zeile, die mit einem Leerzeichen beginnen sind Kommentarzeilen\n"
  "Alle anderen Zeilen beginnen mit einem Befehlszeichen. Diesem Zeichen\n"
  "kann ein weiters Symbol (Dialogzeichen) folgen.\n"
  "Auf diese(s) Zeichen folgt durch Leerzeichen getrennt die Argumente.\n"
  "  o.    Ausgaben des Textes\n"
  "  o!    -\"- und Bestätigung durch den Benutzer\n"
  "  o?    -\"- und Benutzer fragen : weitermachen oder abbrechen\n"
  "  c.    Befehl für Kommandointerpreter\n"
  "  c!    -\"- Programmabbruch falls der Kommandointerpreteraufruf scheitert.\n"
  "  c?    -\"- vorher den Benutzer fragen\n"
  "  e.    Ausführen eines Programms\n"
  "  e!    -\"- Programmabbruch falls ausgeführtes Programm Fehler meldet\n"
  "  e?    -\"- vorher den Benutzer fragen\n"
  "  s     Datum und Zeit für %-Codesequenzen aktuell setzen\n"
  "Vor der Befehlsinterpretation werden die Zeilen bearbeitet. Dabei werden\n"
  "mit % eingeleiteten Codesequenzen umgesetzt.\n"
  "Die Codesquenzen sind %[Zahl]Buchstabe, die Buchstaben bedeuten:\n"
  "  y     Jahr 0..99\n"
  "  Y     Jahr z.B. 1993\n"
  "  M     Monat 1..12\n"
  "  d     Tag 1..31\n"
  "  h     Stunde 0..23\n"
  "  m     Minuten 0..59\n"
  "  s     Sekunde 0..59\n"
  "Die Zahl gibt die min. Stellenanzahl an, bei einer führenden 0 werden\n"
  "die Stellen mit '0' statt mit ' ' aufgefüllt.\n"
  "Das Zeichen '%' wird mit der Codesequenz '%%' erzeugt.\n";

static const char fehler [] = "MACHE-Fehler : ";

/********************************************************************/

typedef struct filenameTAG
  {
  char* path;
  char* name;
  char* ext;
  char* all;
  }
  filename;

typedef enum antwortTAG
  {
  _weiter, _ueberspringen, _abbruch
  }
  antwort;

/********************************************************************/

/*
**  globale Varibale für Zortech C
**  Speicherverbrauch möglichst gering:
*/
int _okbigbuf = 0;

/*
**  Falls -d Schalter in der Commandline gesetzt werden
*/
static FILE* debugout = NULL;

/*
**  Hier wird der Name des zur Zeit offnen MAT-Files gespichert
**  (zur Ausgabe von Meldungen)
*/
static const char* matfilename;

/*
**  Datum und Zeit für die Übersetzung der '%' Codesequenzen
*/
static struct tm* outtime;

/********************************************************************/

#ifndef EXIT_SUCCESS

/*
**  strcpy mit Rückgabe Zeiger auf \0 des kopierten Strings
**  dest   : dorthin kopieren
**  source : diesen String kopieren
**  return : Zeiger auf \0 im dest - String
*/
static char* stpcpy ( char* dest, const char* source )
  {
  while( ( *dest++ = *source++ ) != '\0' ) {}
  return dest - 1;
  }

#endif

/*
**  Ausgeben einer Fehlermeldung und Programm benenden
**  form : Ausgabe der Fehlermeldung über vfprintf
*/
static void error ( const char* form, ... )
  {
  va_list argptr;
  fputs( fehler, stderr );
  va_start( argptr, form );
  vfprintf( stderr, form, argptr );
  va_end( argptr );
  exit( 1 );
  }

/*
**  Fehlermeldung ausgebeung und Libary-Fehlermeldung ausgeben
**  dann Progamm benende
**  form : Ausgabe der Fehlermeldung über vfprintf
*/
static void lerror ( const char* form, ... )
  {
  va_list argptr;
  int error = errno;
  fputs( fehler, stderr );
  va_start( argptr, form );
  vfprintf( stderr, form, argptr );
  va_end( argptr );
  fprintf( stderr, "(Libary-Error: %s)\n", strerror( error ) );
  exit( 1 );
  }

/*
**  malloc, falls kein Speicher mehr da Fehlermeldung
**  size   : Benötigter Speicher in Bytes
**  return : wie malloc, NULL nicht mehr möglich
*/
static void* Malloc ( size_t size )
  {
  void* ptr = malloc( size );
  if( ptr == NULL ) error( "Out of Heap, malloc gescheitert.\n" );
  return ptr;
  }

/*
**  Zerlegt Filename in seine Teile
**  fname : der Filename
**  teile : Struktur mit den Namensteilen
*/
static void zerlegefilename ( const char* fname, filename* teile )
  {
  char* buffer;
  const char* pathend = NULL;
  const char* ptr = fname;
  char akku;
  while( ( akku = *ptr++ ) != '\0' )
    if( akku == '\\' || akku == ':' ) pathend = ptr;
  buffer = Malloc( ptr - fname + 2 );
  teile->all = buffer;
  if( pathend )
    {
    teile->path = buffer;
    while( fname < pathend ) *buffer++ = *fname++;
    *buffer++ = '\0';
    }
  else
    teile->path = NULL;
  teile->name = buffer;
  while( ( akku = *fname++ ) != '\0' && akku != '.' ) *buffer++ = akku;
  *buffer++ = '\0';
  if( akku == '.' )
    {
    teile->ext = buffer;
    while( ( *buffer++ = *fname++ ) != '\0' ) {}
    }
  else
    teile->ext = NULL;
  }

/*
**  Aktuelles Datum und Zeit in die globale Struktur 'outtime'
**  für die Ausgabe setzen
*/
static void settimedate ( void )
  {
  time_t zeit;
  time( &zeit );
  outtime = localtime( &zeit );
  }

/*
**  Wartet auf Kenntnisnahme durch Benutzer
*/
static void warte ( void )
  {
  fputs( "<RETURN>", stdout );
  while( getch() != '\r' ) {}
  putchar( '\n' );
  }

/*
**  Fragt Benutzer ob weitere Ausführung gewünscht
**  return : _weiter oder _abbruch
*/
static antwort weiterabbruch ( void )
  {
  antwort result;
  int in;
  fputs( "Weiter Ausführung (a)bbrechen oder (f)ortsetzen ? ", stdout );
  do{
    in = getch();
    in = tolower( in );
    }
    while( in != 'a' && in != 'f' );
  if( in == 'a' )
    {
    puts( "Abbruch." );
    result = _abbruch;
    }
  else
    {
     puts( "Fortsetzen." );
     result = _weiter;
    }
  return result;
  }

/*
**  Fragt Benutzer nach Vorgehen bei einem Befehl
**  return : _weiter, _ueberspringen oder _abbruch
*/
static antwort weitersprungabbruch ( void )
  {
  antwort result;
  int in;
  fputs( "Befehl (a)usführen, (u)eberspringen oder (E)nde ? ", stdout );
  do{
    in = getch();
    in = tolower( in );
    }
    while( in != 'a' && in != 'u' && in != 'e' );
  switch( in )
    {
    case 'a' : result = _weiter;
               puts( "Ausführen." );
               break;
    case 'u' : result = _ueberspringen;
               puts( "Überspringen." );
               break;
    case 'e' : result = _abbruch;
               puts( "Abbruch." );
    }
  return result;
  }

/*
**  Überlese Whitespaces (Leerzeichen) am Stringanfang
**  str    : der String
**  return : Stringrest
*/
static const char* eatwhite ( const char* str )
  {
  char akku;
  while( ( akku = *str ) != '\0' && isspace( akku ) ) ++str;
  return str;
  }

/*
**  Prüft ob ein Zeichen ein erlaubtes Dialogzeichen (!?.) ist
**  c      : das Zeichen
**  return : Ist das Zeichen ungültig ?
*/
static int invalid ( char c )
  {
  return c != '?' && c != '!' && c != '.';
  }

/*
**  ein Programm ausführen
**  cmd    : Programmname [ Leerzeichen Argumente ]
**  return : Fehler aufgetreten ?
*/
static int callprogram ( const char* cmd )
  {
  int result;
  char* progname;
  int namelen = 0;
  while( cmd[namelen] != '\0' && ! isspace( cmd[namelen] ) ) ++namelen;
  progname = Malloc( namelen + 1 );
  strncpy( progname, cmd, namelen ) [namelen] = '\0';
  cmd = eatwhite( cmd + namelen );
  result = spawnlp( P_WAIT, progname, progname, cmd, NULL ) != 0;
  free( progname );
  return result;
  }

/*
**  Befehl weitergeben an proc
**  cmd    : der Befehl mit vorangesetzem Dialogzeichen
**  proc   : Prozedur, die den Befehl aus führt
**  return : Programmausführung fortsetzen
*/
static int work ( const char* cmd, int (*proc) ( const char* ) )
  {
  int result;
  int exec;
  char dialogzeichen = *cmd;
  if( invalid( dialogzeichen ) )
    error( "Unbekanntes Dialogzeichen '%c' in Zeile\n%s\n", dialogzeichen, cmd );
  cmd = eatwhite( cmd + 1 );
  printf( ">%s\n", cmd );
  if( dialogzeichen == '?' )
    {
    antwort ans = weitersprungabbruch();
    result = ans != _abbruch;
    exec = ans == _weiter;
    }
  else
    exec = 1;
  if( exec )
    {
    if( debugout != NULL )
      {
      fprintf( debugout, "%s\n", cmd );
      result = 1;
      }
    else
      result = (*proc)( cmd ) == 0 || dialogzeichen != '!';
    }
  return result;
  }

/*
**  Zeile ausgeben
**  line   : die Ausgabezeile mit vorangestelltem Dialogzeichen
**  return : Progammausführung fortsezten ?
*/
static int output ( const char* line )
  {
  int result;
  puts( line + 1 );
  switch( *line )
    {
    case '?' : result = weiterabbruch() == _weiter;
               break;
    case '!' : warte();
               result = 1;
               break;
    case '.' : result = 1;
               break;
    default  : error( "Unbekanntes Dialogzeichen '%c' in Ausgabezeile\n%s\n", *line, line );
    }
  return result;
  }

/*
**  Eine vorbereitete Befehlszeile ausführen
**  line   : die Befehlszeile
**  return : Progammausführung fortsetzen ?
*/
static int execute ( const char* line )
  {
  int result;
  switch( *line )
    {
    case 'c' : result = work( line + 1, system );
               break;
    case 'e' : result = work( line + 1, callprogram );
               break;
    case 'o' : result = output( line + 1 );
               break;
    case 's' : settimedate();
               result = 1;
               break;
    default  : error( "Unbekanntes Kommando '%c' in Zeile\n%s\n", *line, line );
    }
  return result;
  }

/*
**  Zahl gemäß Codezeichen aus '%' Codesequenz ermitteln
**  code   : das Codezeichen
**  return : der Wert
*/
static int decode ( char code )
  {
  int result;
  if( outtime == NULL )
    result = 0;
  else
    switch( code )
      {
      case 'y' : result = outtime->tm_year % 100;
                 break;
      case 'Y' : result = outtime->tm_year + 1900;
                 break;
      case 'M' : result = outtime->tm_mon + 1;
                 break;
      case 'd' : result = outtime->tm_mday;
                 break;
      case 'h' : result = outtime->tm_hour;
                 break;
      case 'm' : result = outtime->tm_min;
                 break;
      case 's' : result = outtime->tm_sec;
                 break;
      default  : error( "Das Zeichen '%c' in %-Codesequenz unbekannt.\n", code );
      }
  return result;
  }

/*
**  Eine mit '%' eingeleitet Codesequnez lesen und übersetzen
**  *code   : die Codesequenz
**  *buffer : Buffer für die Übersetzung
**  *size   : Größe des Buffers
*/
static void translatep ( const char** code, char** buffer, size_t* size )
  {
  char codebuffer [10];
  char *const last = codebuffer + sizeof codebuffer;
  char* ptr = codebuffer;
  const char* pcode = *code;
  int count;
  char akku;
  *ptr++ = '%';
  do{
    *ptr++ = akku = *pcode++;
    if( ptr == last ) error( "%-Codesequenz zu lang.\n%s\n", *code );
    }
    while( isdigit( akku ) );
  ptr[-1] = 'd';
  ptr[0] = '\0';
  count = sprintf( *buffer, codebuffer, decode( akku ) );
  *buffer += count;
  *size -= count;
  *code = pcode;
  }

/*
**  Eine Zeile aus dem MAT-File übersetzen
**  line       : die Zeile aus dem File
**  buffer     : Buffer für die Übersetzung
**  buffersize : Größe des Buffers in Byte
*/
static void translate ( const char* line, char* buffer, size_t buffersize )
  {
  const char* ptr = line;
  char akku;
  while( ( akku = *ptr++ ) != '\0' )
    {
    int aufnahme = akku != '%' || *ptr == '%';
    if( aufnahme )
      {
      if( akku == '%' ) ++ptr;
      *buffer++ = akku;
      --buffersize;
      }
    else
      translatep( &ptr, &buffer, &buffersize );
    if( buffersize < 10 ) error( "Übersetzung von Zeile\n%s\nzu lang.\n", line );
    }
  *buffer = '\0';
  }

/*
**  Versucht ein MAT-File zu öffnen
**  fn     : Struktur mit dem Filenamen
**  buffer : Buffer für den zusammengesetzten Filenamen
**  return : das zum Lesen geöffnete File oder NULL
*/
static FILE* tryopen ( const filename* fn, char* buffer )
  {
  FILE* file;
  char* ptr = buffer;
  if( fn->path != NULL )
    {
    ptr = stpcpy( ptr, fn->path );
    if( ptr > buffer && ptr[-1] != ':' && ptr[-1] != '\\' ) *ptr++ = '\\';
    }
  ptr = stpcpy( ptr, fn->name );
  if( fn->ext != NULL )
    {
    *ptr++ = '.';
    strcpy( ptr, fn->ext );
    }
  file = fopen( buffer, "r" );
  if( file != NULL ) matfilename = buffer;
  return file;
  }

/*
**  Versucht MAT-File zu öffnen, testet dabei Pfad durch
**  fn     : Struktur mit dem Filenamen, path-Teil wird ignoriert
**  buffer : Buffer für zusammengebauten Filenamen
**  path   : String mit <path>;<path>;<path>\0, wird unverändert zurückgegeben
**  return : der zum Lesen geöffnete File oder NULL
*/
static FILE* trypaths ( filename* fn, char* buffer, char* path )
  {
  FILE* file = NULL;
  if( path != NULL )
    {
    char akku;
    do{
      fn->path = path;
      while( ( akku = *path ) != '\0' && akku != ';' ) ++path;
      *path = '\0';
      file = tryopen( fn, buffer );
      *path++ = akku;
      }
      while( file == NULL && akku != '\0' );
    }
  return file;
  }

/*
**  Öffnet ein MAT-File, sucht das File in verschiedenen Pfaden
**  fname    : Filename des MAT-Files, ggf. wird Extension ergänzt
**  callpath : Aufrufspfad des Programms
*/
static FILE* openmatfile ( const char* fname, char* callpath )
  {
  static char matfile [120];
  FILE* file;
  filename fn;
  zerlegefilename( fname, &fn );
  if( fn.ext == NULL ) fn.ext = "MAT";
  file = tryopen( &fn, matfile );
  if( file == NULL && fn.path != NULL ) file = trypaths( &fn, matfile, "" );
  if( file == NULL ) file = trypaths( &fn, matfile, callpath );
  if( file == NULL ) file = trypaths( &fn, matfile, getenv( "MACHEPATH" ) );
  if( file == NULL ) file = trypaths( &fn, matfile, getenv( "PATH" ) );
  if( file == NULL )
    error( "Kann File %s nicht öffnen.\n", fname );
  free( fn.all );
  return file;
  }

/*
**  Eine Zeile aus MAT-File lesen, Kommentarzeilen werden übergangen
**  file       : aus diesem File lesen
**  buffer     : in diesem Buffer die Zeile speichern
**  buffersize : Größe des Buffers in Bytes
**  return     : Konnte eine Zeile gelesen werden ?
*/
static int getsmatfile ( FILE* file, char* buffer, size_t buffersize )
  {
  int ok;
  do{
    ok = fgets( buffer, buffersize, file ) != NULL;
    if( ok )
      {
      char* ptr = buffer;
      while( *ptr != '\0' ) ++ptr;
      if( ptr[-1] != '\n' )
        error( "Zeile in MAT-File %s zu lang:\n%s\n", matfilename, buffer );
      else
        ptr[-1] = '\0';
      }
    }
    while( ok && isspace( buffer[0] ) );
  return ok;
  }

/*
**  Debugfile anlegen, es werden keine Befehle mehr ausgeführt
**  nur noch in diesem File protokolliert
**  fname : Name des Debugfiles
**  mode  : Öffnungmode des Debugfiles
*/
static void debug ( const char* fname, const char* mode )
  {
  if( debugout != NULL ) fclose( debugout );
  debugout = fopen( fname, mode );
  if( debugout == NULL )
    lerror( "Kann Debugfile %s nicht öffnen.\n", fname );
  }

/*
**  wieder normale Programmausführung, keine Debugausgaben
*/
static void nodebug ( void )
  {
  if( debugout != NULL )
    {
    fclose( debugout );
    debugout = NULL;
    }
  }

/*
**  Bearbeiten eines Schalters aus der Kommandozeile
**  arg : der Schalter
*/
static void schalter ( const char* arg )
  {
  switch( arg[0] )
    {
    case 'd' : if( arg[1] == '\0' )
                 nodebug();
               else
                 debug( arg + 1, "w" );
               break;
    case 'D' : if( arg[1] == '\0' )
                 nodebug();
               else
                 debug( arg + 1, "a" );
               break;
    default  : error( "unbekannter Schalter: %s.\n", arg );
    }
  }

/*
**  Ausführen eines MAT-Files
**  fname    : Name des Files
**  callpath : Aufrufpfad des Programms
*/
static void ausfuehre ( const char* fname, char* callpath )
  {
  FILE* matfile = openmatfile( fname, callpath );
  char inline [160];
  char line [500];
  int weiter = 1;
  setbuf( matfile, NULL );
  while( weiter && getsmatfile( matfile, inline, sizeof inline ) )
    {
    translate( inline, line, sizeof line );
    weiter = execute( line );
    }
  fclose( matfile );
  }

/*
**  M A I N
*/
int main ( int argc, char* argv [] )
  {
  filename myname;
  if( argc < 2 )
    {
    fputs( manual, stderr );
    exit( 1 );
    }
  setbuf( stdout, NULL );
  if( *argv != NULL )
    zerlegefilename( *argv, &myname );
  else
    myname.path = NULL;
  settimedate();
  while( ++argv, --argc )
    {
    if( **argv == '-' )
      schalter( *argv + 1 );
    else
      ausfuehre( *argv, myname.path );
    }
  return 0;
  }

