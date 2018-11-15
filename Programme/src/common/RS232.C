/*
**  C   ( Microsoft C V5.10 / Zortech C V2.10 / Turbo C V2.0 )
**
**  .02.1993  Ulrich Berntien
**
**  Behandlung der seriellen Schnittstelle:
**      Einstellungen (Baud,Databits,..)
**      String senden
**      Empfang in linearen Buffer
**
**  Literatur:
**    . IBM - Handbuch
**    . Markus Joos : Objektorientierte Programmierung
**                    der RS232C-Schnittstelle
**      in MC August 1992, Seiten 90 ff.
**
**  04.02.1993  Beginn
**  19.10.1993  Meldungstexte auf englisch
*/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include "rs232.h"

/********************************************************************/

#ifndef _interrupt
#  define _far       far
#  define _near      near
#  define _cdecl     cdecl
#  define _pascal    pascal
#  define _interrupt interrupt
#endif

#define LOCAL _pascal _near

#ifdef __TURBOC__
#  define _dos_setvect setvect
#  define _dos_getvect getvect
#endif

#ifdef __ZTC__

#  include <int.h>
#  define INTERRUPT int _cdecl
#  define volatile

  /*
  **  Setzen einer Interrupt-Umleitung
  **  intnum     : Nummer des Interrupts 0 .. 255
  **  inthandler : Funktion für die Interruptbehandlung
  */
  static void LOCAL setint( unsigned intnum, int (* inthandler)() )
    {
    typedef int intfunc ( struct INT_DATA* );
    if( int_intercept( intnum, (intfunc*) inthandler, 0 ) )
      {
      fputs( "Runtime-Libary : int_intercept faild\n"
	     "Interrupt-redirection don't work\n", stderr );
      exit( EXIT_FAILURE );
      }
    }

  /*
  **  Löschen einer Interrupt-Umleitung
  **  (die Funktion int_restore der Zortech Libary arbeitet nicht
  **    korrekt, wenn stacksize == 0 bei int_intercept gesetzt)
  **  intnum : Nummer des Interrupts 0 .. 255
  */
  static void LOCAL resetint ( unsigned intnum )
    {
    unsigned offset, segment;
    struct INT_DATA _far* data;
    int_getvector( intnum, &offset, &segment );
    data = MK_FP( segment, offset + 5 );
    int_setvector( intnum, data->prevvec_off, data->prevvec_seg );
    }

#else

#  define INTERRUPT void _interrupt

  /*
  **  Orginaladresse den der umgeleiteten Interrupts
  */
  static void ( _interrupt _far* old [256] ) ( void );

  /*
  **  Setzen einer Interrupt-Umleitung
  **  intnum     : Nummer des Interrupts 0 .. 255
  **  inthandler : Funktion für die Interruptbehandlung
  */
  static void LOCAL setint ( unsigned intnum, void ( interrupt _far* inthandler)() )
    {
    old[intnum] = _dos_getvect( intnum );
    _dos_setvect( intnum, inthandler );
    }

  /*
  **  Löschen einer Interrupt-Umleitung
  **  intnum : Nummer des Interrupts 0 .. 255
  */
  static void LOCAL resetint ( unsigned intnum )
    {
    _dos_setvect( intnum, old[intnum] );
    }

#endif

/********************************************************************/

/*
**  ASCII-Codes ctrl-s / ctrl-q
*/
#define ctrl_s 19
#define ctrl_q 17

/*
**  Interrupts vom INS 8250 ausgelöst für COM 1,2,3,4
*/
static int uart_int [4] = { 0xC, 0xB, 0xD, 0xF };

/*
**  Registeradresse der PIC (Programable Interrupt Controller)
*/
#define PIC_CMD 0x20
#define PIC_FLAGS 0x21

/*
**  EOI (End Of Interrupt) für den PIC
*/
#define PIC_EOI 0x20

/*
**  Maskenflags im Interrupt-Controler für COM 1,2,3,4
*/
static int pic_flag [4] = { 0x10, 0x08, 0x20, 0x80 };

/*
**  Basisadresse des INS 8250 für COM 1,2,3,4
*/
static int uart_base [4] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };

/*
**  Register des INS 8250
*/
#define EMPFANGSPUFFER       0   /* lesen und schreiben */
#define INTERRUPTAKTIVIERUNG 1
#define INTERRUPTKENNUNG     2   /* nur lesen */
#define LEITUNGSSTEUERUNG    3
#define MODEMSTEUERUNG       4
#define LEITUNGSSTATUS       5
#define MODEMSTATUS          6
#define TEILERSPEICHERLOW    0   /* Teilerspeicherbit = 1 */
#define TEILERSPEICHERHIGH   1   /* Teilerspeicherbit = 1 */

/*
**  Aufbau der Register
*/
struct interruptaktivierungREG
  {
  unsigned datenverfuegbar : 1;
  unsigned datensendbar    : 1;
  unsigned leitungsstatus  : 1;
  unsigned modemstatus     : 1;
  };

union interruptaktivierung { int x; struct interruptaktivierungREG r; };

struct interruptkennungREG
  {
  unsigned keiner  : 1;
  unsigned ursache : 2;
  };

union interruptkennung { int x; struct interruptkennungREG r; };

struct leitungssteuerungREG
  {
  unsigned wortlaenge          : 2;
  unsigned stopbits            : 1;
  unsigned parityfreigabe      : 1;
  unsigned parityeven          : 1;
  unsigned ausgleichsparity    : 1;
  unsigned leitungunterbrechen : 1;
  unsigned teilerspeicherbit   : 1;
  };

union leitungssteuerung { int x; struct leitungssteuerungREG r; };

struct modemsteuerungREG
  {
  unsigned dtr      : 1;
  unsigned rts      : 1;
  unsigned out1     : 1;
  unsigned out2     : 1;
  unsigned schleife : 1;
  };

union modemsteuerung { int x; struct modemsteuerungREG r; };

struct leitungsstatusREG
  {
  unsigned dataready           : 1;
  unsigned overflowerror       : 1;
  unsigned parityerror         : 1;
  unsigned frameerror          : 1;
  unsigned unterbrechung       : 1;
  unsigned sendebereit         : 1;
  unsigned schieberegisterleer : 1;
  };

union leitungsstatus { int x; struct leitungsstatusREG r; };

struct modemstatusREG
  {
  unsigned deltacts  : 1;
  unsigned deltadsr  : 1;
  unsigned teri      : 1;
  unsigned deltalrsd : 1;
  unsigned cts       : 1;
  unsigned dsr       : 1;
  unsigned ri        : 1;
  unsigned rlsd      : 1;
  };

union modemstatus { int x; struct modemstatusREG r; };

/********************************************************************/

/*
**  Buffer für die empfangenen Bytes
**  buffer.now == NULL --> es gibt keinen Buffer
*/
static volatile rs232buffer buffer;

/*
**  zu sendender String
**  Zeiger auf nächstes zu sendende Zeichen
**  nowsend == NULL --> es gibt nichts zu senden
*/
static const char *volatile nowsend;

/*
**  Basisadresse der Schnittstelle
*/
static int base;

/*
**  Modul initialisiert ?
*/
static int init = 0;

/*
**  umgeleiteter Interrupt, Flag des Interrupt im PIC
*/
static int intnummer;
static int maskenflag;


/*
**  Ausgabe über ctrl-s /ctrl-q erlaubt ?
*/
static int xon;

/********************************************************************/

/*
**  Fehlermeldung des RS232 - Moduls ausgeben
*/
static void LOCAL error ( const char* msg )
  {
  fputs( "RS232 : ", stderr );
  fputs( msg, stderr );
  fputc( '\n', stderr );
  }

/*
**  Interruptbehandlung
*/
static INTERRUPT dointerrupt ( void )
  {
  union leitungsstatus status;
  status.x = inp( base + LEITUNGSSTATUS );
  if( status.r.dataready )
    {
    const char akku = (char) inp( base + EMPFANGSPUFFER );
    if( akku == ctrl_s )
      xon = 0;
    else if( akku == ctrl_q )
      xon = 1;
    else if( buffer.now != NULL )
      {
      *buffer.now++ = akku;
      if( akku == '\n' ) ++buffer.lfs;
      if( status.r.parityerror ) ++buffer.parityerrors;
      if( status.r.overflowerror ) ++buffer.overflowerrors;
      }
    }
  if( status.r.sendebereit )
    {
    inp( base + INTERRUPTKENNUNG ); /* Interrupt zurücksetzen */
    if( nowsend != NULL && xon )
      {
      const char akku = *nowsend++;
      if( akku == '\0' )
        nowsend = NULL;
      else
        outp( base + EMPFANGSPUFFER, akku );
      }
    }
  outp( PIC_CMD, PIC_EOI );
#ifdef __ZTC__
  return 1;
#endif
  }

/*
**  Interrupt wieder in alten Zustand
**  den zugehörigen IRQ im PIC sperren
*/
static void rs232exit ( void )
  {
  if( intnummer != 0 )
    {
    outp( base + INTERRUPTAKTIVIERUNG, 0 );
    outp( PIC_FLAGS, inp( PIC_FLAGS ) | maskenflag );
    resetint( intnummer );
    }
  }

/*
**  globale Variablen initialisieren
*/
static void LOCAL doinit ( void )
  {
  if( atexit( rs232exit ) )
    error( "cant't install exit-function." );
  else
    {
    intnummer = 0;
    maskenflag = 0;
    buffer.start = buffer.now = NULL;
    nowsend = NULL;
    }
  init = 1;
  }

/*
**  Abfangen des Interrupts der ser. Schnittstelle einstellen
**  com : Nummer der Schnittstelle
*/
static void LOCAL setinterrupt ( int com )
  {
  const int takenummer = uart_int [ com-1 ];
  if( intnummer != takenummer )
    {
    if( intnummer != 0 ) resetint( intnummer );
    setint( intnummer = takenummer, dointerrupt );
    }
  }

/*
**  Einstellen der Baud-Rate
**  baud   : die Baudrate
*/
static void LOCAL setbaud ( int baud )
  {
  const unsigned teiler = (unsigned) ( 115200L / baud );
  union leitungssteuerung steuer;
  steuer.x = inp( base + LEITUNGSSTEUERUNG );
  steuer.r.teilerspeicherbit = 1;
  outp( base + LEITUNGSSTEUERUNG, steuer.x );
  outp( base + TEILERSPEICHERLOW, teiler );
  outp( base + TEILERSPEICHERHIGH, teiler >> 8 );
  steuer.r.teilerspeicherbit = 0;
  outp( base + LEITUNGSSTEUERUNG, steuer.x );
  }

/*
**  Einstellen Anzahl der Datenbits,Stopbits und Parity
**  parameter : die Parameter
**  return    : alles ok ?
*/
static int LOCAL setlaengen ( const rs232parameter* parameter )
  {
  int ok = 1;
  if( parameter->databits < 5 || parameter->databits > 8 )
    {
    error( "Databits only 5 to 8." );
    ok = 0;
    }
  if( parameter->stopbits < 1 || parameter->stopbits > 2 )
    {
    error( "Stopbits only 1 or 2." );
    ok = 0;
    }
  if( ok )
    {
    union leitungssteuerung steuer;
    steuer.r.wortlaenge = parameter->databits - 5;
    steuer.r.stopbits = parameter->stopbits - 1;
    steuer.r.parityfreigabe = parameter->parity == _rs232none ? 0 : 1;
    steuer.r.parityeven = parameter->parity == _rs232even;
    steuer.r.ausgleichsparity = 0;
    steuer.r.leitungunterbrechen = 0;
    steuer.r.teilerspeicherbit = 0;
    outp( base + LEITUNGSSTEUERUNG, steuer.x );
    }
  return ok;
  }

/*
**  Schnittstellen Interrupts aktivieren
**  setzt globale Variable maskenflag
**  com : Nummer der Schnittstelle
*/
static void LOCAL setbereitschaft ( int com )
  {
  union modemsteuerung modem;
  union interruptaktivierung aktiv;
  modem.x = 0;
  modem.r.dtr = 1;
  modem.r.rts = 1;
  modem.r.out2 = 1;              /* Leitungstreiber einschalten (?) */
  outp( base + MODEMSTEUERUNG, modem.x );
  aktiv.x = 0;
  aktiv.r.datenverfuegbar = 1;
  aktiv.r.datensendbar = 1;
  outp( base + INTERRUPTAKTIVIERUNG, aktiv.x );
  maskenflag = pic_flag[ com-1 ];
  outp( PIC_FLAGS, inp( PIC_FLAGS ) & ~maskenflag );
  /* alle möglichen Interrupt-Zustände des INS 8250 zurücksetzen : */
  inp( base + LEITUNGSSTATUS );
  inp( base + EMPFANGSPUFFER );
  inp( base + INTERRUPTKENNUNG );
  inp( base + MODEMSTATUS );
  }

/*
**  Einstellen der seriellen Schnittstelle
**  parameter : auf diese Werte einstellen
**  return    : Fehler aufgetreten ?
*/
int rs232setparameter ( const rs232parameter* parameter )
  {
  int ok;
  if( ! init ) doinit();
  ok = parameter->com > 0 && parameter->com < 5;
  if( ! ok ) error( "Com.Port only com = 1..4 available.");
  base = uart_base[ parameter->com - 1 ];
  if( parameter->baud < 1 )
    {
    error( "Bits/second invalid" );
    ok = 0;
    }
  if( ok )
    {
    setbaud( parameter->baud );
    ok = setlaengen( parameter );
    }
  if( ok )
    {
    setinterrupt( parameter->com );
    xon = 1;
    setbereitschaft( parameter->com );
    }
  return ! ok;
  }

/*
**  einen String senden
**  string : die zu sendenden Daten
**  return : Adresse eines Zeigers der NULL wird, wenn alle Daten gesendet
*/
const char *volatile * rs232send ( const char* string )
  {
  if( ! init ) doinit();
  if( *string != '\0' && xon )
    {
    union leitungsstatus status;
    nowsend = string;
    status.x = inp( base + LEITUNGSSTATUS );
    if( status.r.sendebereit )
      outp( base + EMPFANGSPUFFER, *nowsend++ );
    }
  return &nowsend;
  }

/*
**  Buffer für den Empfang der Daten von der RS-232
**  die Länge wird beim Empfang nicht getestet
**  ptr    : dort die Daten abspeichern
**  return : der benutzte RS-232 Buffer-Header
*/
volatile rs232buffer* rs232setbuffer ( char* ptr )
  {
  if( ! init ) doinit();
  buffer.overflowerrors = buffer.parityerrors = 0;
  buffer.lfs = 0;
  buffer.start = buffer.now = ptr;
  return &buffer;
  }

/*
**  Empfangsbuffer und Sendebuffer auf NULL
**  (der Speicher wird dadurch nicht freigegeben)
*/
void rs232clear ( void )
  {
  buffer.now = buffer.start = NULL;
  nowsend = NULL;
  }

