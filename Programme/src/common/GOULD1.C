/*
**  C   ( Microsoft C V5.10 / Zortech C V2.1 / Turbo C V2.0 )
**
**  .02.1993 Ulrich Berntien
**
**  Befehle an Gould senden und Antwort empfangen
**  über serielle Schnittstelle
**
**  08.02.1993  Beginn
**  15.06.1993  Schalter in Kommandozeile
**  13.07.1993  Steuerung auf '.kommando' umgestellt
*/

#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <limits.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rs232.h"

#ifndef _far
#  define _far    far
#  define _near   near
#  define _pascal pascal
#endif

#define LOCAL _pascal _near

#define KB * 1024

/*
**  Kurzanleitung des Programms
*/
static const char manual [] =
  "GOULD1      .02.1993     Ulrich Berntien\n"
  "Kommunikation mit DSO Gould über die serielle Schnittstelle\n"
  "Aufrufformat:\n\n"
  "   GOULD1 [-t<timeout>] [-p<port>] <baudrate>\n\n"
  "<baudrate> = ganze Zahl z.B. 9600\n"
  "<timeout>  = max. Wartezeit in 1/10 sec auf Zeichenempfang\n"
  "             Standarteinstellung 1 Sekunde.\n"
  "<port>     = Nummer der Schnittstelle 1..4, Standart 1\n\n"
  "Die Übertragungen können durch Taste 's' abgebrochen werden.\n"
  "Das Programm wird durch keine Eingabe beendet.\n";

/*
**  Liste der Kommandos
*/
static const char kommandos [] =
  "Neben den Befehlen für die Gould können noch Befehle an das\n"
  "Programm eingegeben werde. Diese beginnen mit einem '.'.\n"
  "Soll ein Befehl für die Gould mit einem Punkt beginnen, so muß\n"
  "dieser verdoppelt werden '..'.\n"
  ".help                    Diesen Text ausgeben\n"
  ".exec <filename>         Ausführen von Befehlen aus einer Datei,\n"
  "                         Zeilen mit % am Anfang sind Kommentarzeilen.\n"
  ".output <filename>       Ausgabe in ein File lenken.\n"
  ".outputnew <filename>    wie .output, das File darf noch nicht existieren\n"
  ".outputappend <filename> wie .output, an ein bestehendes File wird\n"
  "                         angehängt.\n"
  ".output                  Umlenkung aufheben.\n"
  ".port <nummer>           umstellen auf anderen ser. Port (1..4)\n"
  ".timeout <1/10sec>       Zeit bis Timeout einstellen\n"
  ".reset                   ser. Schnittstelle des PCs neu initialisieren\n"
  ".system <befehl>         Befehl ans DOS übergeben.\n"
  ".system                  Kommandointerpreter starten.\n";

/********************************************************************/

/*
**  Ein/Ausgabebuffer
**  wird in der main Funktion angelegt und vernichtet
*/
struct buffert
  {
  char* ptr;
  size_t size;
  }
  buffer;

/*
**  Ausgabefile für Daten von der Gould
*/
static FILE* dtaout = stdout;

/*
**  Wartezeit bis Timeout erfolgt beim Zeichenempfang
**  von serieller Schnittstelle in clock_t Einheiten
*/
static clock_t timer = CLK_TCK;      /* 1 Sekunde */

/*
**  Benutzte Schnittstelle (COM1 bis COM4 erlaubt)
*/
static int com = 1;

/*
**  Falls Benutzer Übertragung abgebrochen hat
*/
static int userabort = 0;

static int LOCAL command ( char* );

/********************************************************************/

/*
**  Fehlermeldung ausgeben, Programm beenden
*/
static void LOCAL error ( const char* msg )
  {
  fprintf( stderr, "\ngould1-error : %s.\n", msg );
  exit( 1 );
  }

/*
**  Fehlermeldung nach Problemen mit Dateien ausgeben
*/
static void LOCAL fileerror ( void )
  {
  fprintf( stderr, "\nerror: %s\n", strerror( errno ) );
  }

/*
**  Ausgeben der Parameter der ser. Schnittstelle in Textform
**  ptr : die Parameter
*/
static void LOCAL outparameter ( const rs232parameter* ptr )
  {
  const char* parity;
  switch( ptr->parity )
    {
    case _rs232none : parity = "none";
                      break;
    case _rs232even : parity = "even";
                      break;
    case _rs232odd  : parity = "odd";
                      break;
    default         : parity = "unbekannter wert";
    }
  printf( "Schnittstelle COM%d\n"
          "Baudrate  : %d\n"
          "Datenbits : %d\n"
          "Stopbits  : %d\n"
          "Parity    : %s\n"
          "CTRL-S/Q gesteuert.\n"
          , ptr->com, ptr->baud, ptr->databits, ptr->stopbits, parity );
  }

/*
**  Einstellen der Schnittstelle
**  baud : Baudrate, falls = 0 alte Baudrate benutzen
*/
static void LOCAL init ( int baud )
  {
  static int baudrate = 9600;
  rs232parameter parameter;
  if( baud != 0 ) baudrate = baud;
  parameter.com = com;
  parameter.baud = baudrate;
  parameter.databits = 7;
  parameter.stopbits = 1;
  parameter.parity = _rs232even;
  if( rs232setparameter( &parameter ) )
    error( "Einstellen der Schnittstelle fehlgeschlagen" );
  else
    outparameter( &parameter );
  }

/*
**  Abfrage der Tastatur ob 's' gedrückt
**  return : Wurde nicht mit 's' unterbrochen ?
*/
static int LOCAL weiter ( void )
  {
  int result = 1;
  if( kbhit() )
    {
    int akku = getch();
    result = akku != 's' && akku != 'S';
    if( ! result ) userabort = 1;
    }
  return result;
  }

/*
**  Abfrage und Rücksetzen
**  return : Wurde Übertragung vom Benutzer abgebrochen ?
*/
static int LOCAL wurdeabgebrochen ( void )
  {
  int result = userabort;
  userabort = 0;
  return result;
  }

/*
**  Ausgeben der benötigten Zeit
**  delta : die benötigte Zeit in clock_t Einheiten
*/
static void LOCAL printdelta ( long delta )
  {
  ldiv_t sec;
  sec = ldiv( delta, (long) CLK_TCK );
  #if CLK_TCK == 1000
    printf( "in %ld.%03ld sec \xf1 0.050 sec.\n", sec.quot, sec.rem );
  #else /* CLT_TCK = 100 */
    printf( "in %ld.%02ld sec \xf1 0.050 sec.\n", sec.quot, sec.rem );
  #endif
  }

/*
**  Berechnet vergangene clock_t Einheiten seit dem Startzeitpunkt
**  start  : der Startzeitpunkt
**  return : Anzahl der vergangenen Einheiten
*/
static long LOCAL needclock ( clock_t start )
  {
  long delta = clock() - start;
  if( delta < 0 ) delta += LONG_MAX;
  return delta;
  }

/*
**  Zeichekette an die Gould senden
**  und Antwort darauf Empfangen mit Watchdog-Timer Kontrolle
**  cmd    : der String wird gesendet
**  return : die Antwort der Gould
*/
static const char* LOCAL transfer ( const char* cmd )
  {
  const char* result = NULL;
  char* last = buffer.ptr;
  char akku;
  volatile rs232buffer* ptr = rs232setbuffer( buffer.ptr );
  const char *volatile *out = rs232send( cmd );
  clock_t startclock;
  clock_t timeout;
  long ticks;
  while( *out != NULL && weiter() )
    {
    akku = **out;
    putchar( akku > 0x20 ? akku : '_' );
    putchar( '\b' );
    }
  if( *out == NULL )
    {
    puts( "CMD sended" );
    timeout = startclock = clock();
    while( ptr->lfs == 0 && weiter() && needclock(timeout) < timer )
      {
      akku = *ptr->now;
      putchar( akku > 0x20 ? akku : '_' );
      putchar( '\b' );
      if( ptr->now > last )
        {
        timeout = clock();
	last = ptr->now;
	if( ptr->now - buffer.ptr > buffer.size ) error( "inputbuffer overflow" );
        }
      }
    ticks = needclock( startclock );
    printf( "%d bytes ", ptr->now - buffer.ptr );
    if( ptr->lfs != 0 )
      {
      if( ptr->parityerrors == 0 && ptr->overflowerrors == 0 )
	puts( "recieved without errors." );
      else
	printf( "recieved with  %d Parity and %d Overflow - Errors.\n",
                 ptr->parityerrors, ptr->overflowerrors );
      last = ptr->now;
      *last = '\0';
      result = buffer.ptr;
      }
    printdelta( ticks );
    }
  rs232clear();
  return result;
  }

/*
**  String senden, ggf. Antwort in File 'dtaout' schreiben
**  cmd    : ASCIZ-String, CR+LF wird angehängt
**  return : hat Benutzer die Übertragung unterbrochen ?
*/
static int LOCAL exec ( char* cmd )
  {
  static const char noresponse [] = "< no response >";
  const char* response;
  int result = 0;
  char* ptr = cmd;
  char akku;
  while( ( akku = *ptr ) != '\0' ) *ptr++ = toupper( akku );
  ptr[0] = '\r';
  ptr[1] = '\n';
  ptr[2] = '\0';
  wurdeabgebrochen();
  response = transfer( cmd );
  if( response != NULL )
    fputs( response, dtaout );
  else
    {
    result = wurdeabgebrochen();
    puts( noresponse );
    }
  return result;
  }

/*
**  Eingabe von Konsole lesen
**  return : NULL, wenn kein Kommando gegeben
**           sonst die Eingabe
*/
static char* LOCAL getcmd ( void )
  {
  char* result = buffer.ptr;
  fputs( "GOULD>", stdout );
  result = fgets( buffer.ptr, buffer.size, stdin );
  if( result )
    {
    const char* source = result;
    char* dest = buffer.ptr;
    char akku;
    while( ( akku = *source++ ) != '\0' )
      if( isprint( akku ) ) *dest++ = akku;
    *dest = '\0';
    if( *buffer.ptr == '\0' ) result = NULL;
    }
  return result;
  }

/*
**  Eingabe aus einem File lesen
**  file   : Textfile, read only
**  return : != NULL, wenn Eingabe vorliegt
*/
static char* fgetcmd (  FILE* file )
  {
  char* result = buffer.ptr;
  int i = 0;
  int akku;
  errno = 0;
  while( ( akku = fgetc( file ) ) == '%' )
    if( ( akku = fgetc( file ) ) == '%' )
      break;
    else
      while( akku != '\n' && akku != EOF ) akku = fgetc( file );
  while( akku != ';' && akku != '\n' && akku != EOF )
    {
    result[i++] = (char) akku;
    if( i < buffer.size )
      akku = fgetc( file );
    else
      akku = EOF;
    }
  result[i++] = '\0';
  if( akku ==  EOF )
    {
    if( i >= buffer.size )
      {
      fputs( "inputbuffer overflow, inputline too long.\n", stderr );
      result = NULL;
      }
    else if( errno != 0 )
      {
      fputs( "trouble by reading command file.", stderr );
      fileerror();
      result = NULL;
      }
    else if( i < 2 )
      result = NULL;
    }
  return result;
  }

/*
**  Spaces am Anfang eines Strings überlesen
**  str    : der String
**  return : Zeiger in den string, auf erste nicht space Zeichen
*/
static char* LOCAL skipspace ( char* str )
  {
  while( *str != '\0' && isspace( *str ) ) ++str;
  return str;
  }

/*
**  Eingabe verarbeiten
**  cmd    : der Eingabestring
**  return : Ungültiger Befehl oder abgebrochene Übertragung ?
*/
static int LOCAL verarbeite ( char* cmd )
  {
  int result;
  cmd = skipspace( cmd );
  if( *cmd == '.' && *(++cmd) != '.' )
    result = command( cmd );
  else
    if( ! isspace( *cmd ) && *cmd != '\0' )
      result = exec( cmd );
    else
      result = 0;
  return result;
  }

/*
**  Befehle aus einer Datei ausführen
**  fname : Name der Datei
*/
static void LOCAL batch ( const char* fname )
  {
  FILE* input = fopen( fname, "r" );
  if( input == NULL )
    {
    fprintf( stderr, "can't open command file '%s'.", fname );
    fileerror();
    }
  else
    {
    char* cmd;
    while( ( cmd = fgetcmd( input ) ) != NULL && verarbeite( cmd ) == 0 ) {}
    if( cmd != NULL ) fputs( "commandfile processing aborted.\n", stderr );
    }
  fclose( input );
  }

/*
**  Eingaben vom Benutzer entgegennehmen und auswerten
*/
static void LOCAL dialog ( void )
  {
  char* cmd;
  while( ( cmd = getcmd() ) != NULL ) verarbeite( cmd );
  }

/*
**  Trennt Argumente vom Befehlsnamen ab
**  cmd    : das komplette Kommando, ACHTUNG ein '\0' wird eingefügt
**  return : Zeiger auf Anfang der Argumente
*/
static char* LOCAL separate ( char* cmd )
  {
  while( *cmd != '\0' && ! isspace( *cmd ) ) ++cmd;
  if( *cmd != '\0' ) *cmd++ = '\0';
  return skipspace( cmd );
  }

/*
**  Umleitung der Ausgabe zurücksetzen
*/
static void LOCAL dtareset ( void )
  {
  if( dtaout != stdout )
    {
    if( fclose( dtaout ) != 0 ) fileerror();
    dtaout = stdout;
    }
  }

/*
**  Umleiten der Ausgabe in ein File
**  fname : Name des Files
**  mode  : Filemode (c)reate, (w)rite oder (a)ppend
*/
static void LOCAL dtaredirect ( const char* fname, char mode )
  {
  int ok;
  dtareset();
  if( mode == 'c' )
    ok = access( fname, 0 );
  else if( mode == 'a' )
    ok = ! access( fname, 0 );
  else
    ok = 1;
  if( ok )
    {
    FILE* file = fopen( fname, mode == 'a' ? "a" : "w" );
    if( file != NULL )
      dtaout = file;
    else
      {
      fprintf( stderr, "can't %s file '%s'."
	       ,mode == 'a' ? "open" : "create"
	       ,fname );
      fileerror();
      }
    }
  else
    fprintf( stderr, "file '%s'%s exist.\n", fname
	     , mode == 'a' ? " not" : "" );
  }

/*
**  Setzen der Nummer der seriellen Schnittstelle 1..4
**  nummer : String, enthält die Nummer
*/
static void LOCAL setcom ( const char* nummer )
  {
  com = atoi( nummer );
  if( com < 1 || com > 4 )
    {
    fprintf( stderr, "seriell port : %s", nummer );
    error( "only ports 1..4 allowed" );
    }
  }

/*
**  Watchdog-Timer Zeit einstellen
**  time : String, enthält Zeit in 1/10 Sekunden
*/
static void LOCAL settimeout ( const char* time )
  {
  long akku = atol( time );
  if( akku < 1 )
    {
    fprintf( stderr, "Timeout : %s/10 seconds", time );
    error( "Waittime min. 1/10 second" );
    }
  else
    timer = ( akku * CLK_TCK ) / 10;
  }

/*
**  Aufruf der Shell
*/
static void LOCAL shell ( void )
  {
  char* cmd = getenv( "COMSPEC" );
  if( cmd == NULL ) cmd = "COMMAND.COM";
  if( spawnlp( P_WAIT, cmd ,cmd, NULL ) < 0 )
    {
    printf( "can't run '%s'.", cmd );
    fileerror();
    }
  }

/*
**  Befehl ausführen
**  cmd    : der Befehl
**  return : Ungültiger Befehl ?
*/
static int LOCAL command ( char* cmd )
  {
  static const char* cmds [] =
    {
    "exec", "output", "outputnew", "outputappend", "system", "help",
    "port", "reset", "timeout", NULL
    };
  int result = 0;
  int n = 0;
  char* arg = separate( cmd );
  while( cmds[n] != NULL && strcmp( cmd, cmds[n] ) != 0 ) ++n;
  switch( n )
    {
    case 0  : batch( arg );
	      break;
    case 1  : if( *arg == '\0' )
		dtareset();
	      else
		dtaredirect( arg, 'w' );
	      break;
    case 2  : dtaredirect( arg, 'c' );
	      break;
    case 3  : dtaredirect( arg, 'a' );
	      break;
    case 4  : if( *arg == '\0' )
		shell();
	      else
		system( arg );
	      break;
    case 5  : fputs( kommandos, dtaout );
	      break;
    case 6  : setcom( arg );
	      init( 0 );
	      break;
    case 7  : init( 0 );
	      break;
    case 8  : settimeout( arg );
	      break;
    default : fprintf( stderr, "unknown command '%s'\n", cmd );
	      result = 1;
    }
  putchar( '\n' );
  return result;
  }

/*
**  Auswerten eines Schalters
**  arg : der Schalter aus der Kommandozeile
*/
static void LOCAL schalter ( const char* arg )
  {
  switch( arg[0] )
    {
    case 't' : settimeout( arg + 1 );
               break;
    case 'p' : setcom( arg + 1 );
               break;
    default  : fprintf( stderr, "switch : %s", arg );
	       error( "unknown switch" );
    }
  }

/*
**  Anlegen der Ein/Ausgabebuffer
**  ptr : Zeiger auf anzulegende Buffer
*/
static void LOCAL initbuffer (struct buffert* ptr )
  {
  ptr->size = 25 KB;
  while( ( ptr->ptr = malloc( ptr->size ) ) == NULL )
    {
    ptr->size /= 2;
    if( ptr->size < 1 KB ) error( "Out of Heap, no input/output buffers" );
    }
  }

/*
**  M A I N
*/
int main ( int argc, char* argv[] )
  {
  if( argc < 2 )
    fprintf( stderr, "%s%s", manual, kommandos );
  else
    {
    int baud;
    while( --argc && *(++argv)[0] == '-' ) schalter( *argv + 1 );
    if( argc != 1 ) error( "only one parameter allowed" );
    baud = atoi( *argv );
    if( baud < 1 ) error( "baudrate < 1 not allowed" );
    init( baud );
    initbuffer( &buffer );
    dialog();
    free( buffer.ptr );
    }
  return 0;
  }