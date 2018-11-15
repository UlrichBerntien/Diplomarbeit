/*
**  Header-File der phase??.C und phase2.c Dateien
*/

#ifndef _far
#  define _near   near
#  define _far    far
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#define LOCAL _near _pascal

/*
**  Ein Vollkreis = 360° = 2Pi =
*/
#define CIRC 2048

/*
**  Umrechnung Winkel in Bogenmaß nach Winkel in CIRC
*/
#define TOCIRC * (CIRC/6.283185308)

/*
**  Umrechnung Winkel in CIRC nach Winkel in Grad
*/
#define INGRAD * (360.0/CIRC)

typedef unsigned char uchar;
typedef unsigned int word;

/*
**  Datenstruktur für einen Graphen aus uchar's
*/
typedef struct grafucTAG
  {
  uchar* data;              /* die Daten          */
  int len;                  /* Anzahl der uchar's */
  uchar min;                /* Min. Wert in data  */
  uchar max;                /* Max. Wert in data  */
  }
  grafuc;

/*
**  Datenstruktur für einen Graphen aus long's
*/
typedef struct graflTAG
  {
  long* data;               /* die Daten          */
  long min;                 /* Min. Wert in data  */
  long max;                 /* Max. Wert in data  */
  int len;                  /* Anzahl der long's  */
  }
  grafl;

/*
**  Datenstruktur für Signal- und Phasenwinkel-Werte
*/
typedef struct phasenwerteTAG
  {
  time_t aufnahme;      /* Datum und Zeit der Messung                  */
  char komment [20];    /* z.B. Schußnummer                            */
  float timebase;       /* Zeit pro Div. (1Div=10Punkte)               */
  grafuc signalA;       /* alle drei graf.. müssen gleiche Länge haben */
  grafuc signalB;       /*                                             */
  grafl winkel;         /* phi in CIRC                                 */
  int A, A0;            /* a = A0 + sin(phi) / A                       */
  int B, B0;            /* b = B0 + sin(phi+deltaphi) / B              */
  int deltaphi;         /* Phasendifferenzwinkel in CIRC               */
  }
  phasenwerte;

/* PHASE2.C  */

extern const char* txterr;
extern const char* noheap;

/*  PHASE00.C */

int  _cdecl stoptaste ( int wait );
int  _cdecl waitkey ( void );
int  _cdecl janein ( void );
int  _cdecl readstring ( const int len, char* buffer );
void _cdecl error ( const char* msg );
void _cdecl verror ( const char* form, ... );

/* PHASE01.C */

int   _cdecl gould_read ( int kanal, grafuc* x );
float _cdecl gould_timebase ( int kanal );
int   _cdecl gould_lock ( void );
int   _cdecl gould_unlock ( void );

/* PHASE02.C */

void _cdecl plotfile_init ( const char* fname );
void _cdecl plotfile_graf ( int len, const uchar* x );
int  _cdecl plotfile_grafl ( const grafl* x );
void _cdecl plotfile_komment ( const char* str );
void _cdecl plotfile_printf ( const char* form, ... );
void _cdecl plotfile_ff ( void );

/* PHASE03.C */

int  _cdecl grafik_open ( void );
void _cdecl grafik_close ( void );
void _cdecl grafik_cls ( void );
void _cdecl grafik_clear ( int oben );
void _cdecl grafik_puts( int x, int y, const char* str );
void _cdecl grafik_grafuc ( int oben, const grafuc* x );
void _cdecl grafik_grafl ( int oben, const grafl* x );
void _cdecl grafik_headline ( const char* name );
void _cdecl grafik_home ( int x, int y );
int  _cdecl grafik_menu ( const char** liste, int selekt );

/* PHASE04.C */

void _cdecl ausgabe_plotfile ( const phasenwerte* x, int calc );
void _cdecl ausgabe_signale ( const phasenwerte* x );
void _cdecl ausgabe_grafik ( const phasenwerte* x );
void _cdecl ausgabe_doppelt ( const phasenwerte* x );
void _cdecl ausgabe_tabelle ( const phasenwerte* x );
void _cdecl ausgabe_parameter ( const phasenwerte* x );

/* PHASE05.C */

void _cdecl calc_init ( void );
void _cdecl calc_auswertung ( phasenwerte* data, int grafikaus );
void _cdecl calc_signal ( int a, const phasenwerte* x, grafl* signal );

/* PHASE06.C */

int  _cdecl file_dload ( phasenwerte* data );
void _cdecl file_dsave ( const phasenwerte* data );
int  _cdecl file_dosz ( phasenwerte* data, int kanal [2] );

