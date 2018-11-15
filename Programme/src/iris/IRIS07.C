/*
**  C  ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .08.1993
**
**  Schnittstelle zu DOS-Funktionen
**
**  23.08.1993  Beginn
**  19.12.1994  Aufruf des Volkov-Commanders aufgenommen
*/

#include <ctype.h>
#include <direct.h>
#include <dos.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "iris.h"

/*
**  Datenstrukturen von DOS
*/
struct dostime
  {
  unsigned sec : 5;     /* Sekunden / 2 */
  unsigned min : 6;     /* Minuten      */
  unsigned std : 5;     /* Stunden      */
  };

struct dosdate
  {
  unsigned tag  : 5;    /* Tag         */
  unsigned mon  : 4;    /* Monat       */
  unsigned jahr : 7;    /* Jahr - 1980 */
  };

/*
**  Bestimmt im Pfad angegebenes Laufwerk
**  pfad   : Pfadname (ggf. mit Driveangabe)
**  return : 0 = Standartdrive, 1 = A, 2 = B, ..
*/
static int LOCAL getdrive ( const char* pfad )
  {
  int drive;
  while( *pfad != '\0' && isspace( *pfad ) ) ++pfad;
  if( pfad[0] != '\0' && pfad[1] == ':' )
    drive = tolower( pfad[0] ) - 'a' + 1;
  else
    drive = 0;
  return drive;
  }

/*
**  Holt Anzahl freie Bytes vom Laufwerk mit Nummer 'nr'
**  nr == 0 : aktuelles Laufwerk
**  nr == 1 : A,  nr == 2 : B,  ..
**  return : Anzahl der freien Bytes
*/
long diskfree ( int nr )
  {
  union REGS regs;
  regs.h.ah = 0x36;   /* Verbleibende Plattenkapazität ermitteln */
  regs.h.dl = (uchar) nr;
  intdos( &regs, &regs );
  return regs.x.ax == 0xFFFF
          ? 0l : (long) regs.x.ax * (long) regs.x.bx * (long) regs.x.cx;
  }


/*
**  Ein Kommando vom Konsole einlesen und an DOS senden
*/
void doscmd ( void )
  {
  char commando [70];
  fputs( "CMD>", stdout );
  commando[0]  = '\0';
  stredit( commando, sizeof commando );
  if( commando[0] != '\0' )
    {
    char oldpath [ pathlen+1 ];
    getcwd( oldpath, sizeof oldpath );
    system( commando );
    chdir( oldpath );
    getreturn();
    }
  }

/*
**  DOS-Shell oder Volkov-Commander aufrufen
**  volkov : Soll der Volkov-Commmander aufgerufen werden ?
*/
void dosshell ( int vc )
  {
  char* cmd;
  char oldpath [pathlen+1];
  if( vc )
    cmd = "VC.COM";
  else
    {
    cmd = getenv( "COMSPEC" );
    if( cmd == NULL ) cmd = "COMMAND.COM";
    }
  getcwd( oldpath, sizeof oldpath );
  if( spawnlp( P_WAIT, cmd ,cmd, NULL ) < 0 )
    {
    printf( "can't run '%s'. ", cmd );
    perror( NULL );
    }
  chdir( oldpath );
  }

#if defined __ZTC__

#  define DIRBUFFER struct FIND
#  define NAME      name
#  define SIZE      size
#  define DATE      date
#  define TIME      time
#  define FIRSTDIR(buffer,pfad) _dos_findfirst( pfad, 0, &buffer ) == 0
#  define NEXTDIR(bufer)        _dos_findnext( &buffer ) == 0

#elif defined __TURBOC__

#  include <dir.h>
#  define DIRBUFFER struct ffblk
#  define NAME      ff_name
#  define SIZE      ff_fsize
#  define DATE      ff_fdate
#  define TIME      ff_ftime
#  define FIRSTDIR(buffer,pfad) findfirst( pfad, &buffer, 0 ) == 0
#  define NEXTDIR(buffer)       findnext( &buffer ) == 0

#else /* MSC */

#  define DIRBUFFER struct find_t
#  define NAME      name
#  define SIZE      size
#  define DATE      wr_date
#  define TIME      wr_time
#  define FIRSTDIR(buffer,pfad) _dos_findfirst( pfad, 0, &buffer ) == 0
#  define NEXTDIR(buffer) _dos_findnext( &buffer ) == 0

#endif

/*
**  Datum und Zeit auf Konsole ausgegben
**  date : Datum im DOS-Format
**  time : Zeit im DOS-Format
*/
static void LOCAL printdatetime ( const unsigned* date, const unsigned* time )
  {
  struct dosdate d;
  struct dostime t;
  d = * (const struct dosdate*) date;
  t = * (const struct dostime*) time;
  printf( "%2d.%02d.%04d  %2d:%02d\n",
          d.tag, d.mon, d.jahr + 1980,
          t.std, t.min );
  }

/*
**  Directory auf Konsole ausgeben von gewünschem Pfad
*/
void dosdir ( void )
  {
  char pfad [ pathlen ];
  int ok;
  int files = 0;
  long all = 0l;
  DIRBUFFER buffer;
  fputs( "DIR ", stdout );
  pfad[0] = '\0';
  stredit( pfad, sizeof pfad );
  ok = FIRSTDIR( buffer, pfad );
  while( ok )
    {
    if( ++files % 24 == 0 ) getreturn();
    all += buffer.SIZE;
    printf( "    %12s   %10ld  ", buffer.NAME, buffer.SIZE );
    printdatetime( &buffer.DATE, &buffer.TIME );
    ok = NEXTDIR( buffer );
    }
  printf( "%9ld bytes in %d file(s)\n"
          "%9ld bytes free\n",
           all, files,
           diskfree( getdrive( pfad ) )
        );
  getreturn();
  }
