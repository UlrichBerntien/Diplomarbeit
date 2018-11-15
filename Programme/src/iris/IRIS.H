#ifndef _far
#  define _far    far
#  define _near   near
#  define _cdecl  cdecl
#  define _pascal pascal
#endif

#define LOCAL _near _pascal
#define TEXT static const char

#define pathlen 65               /* max. Pfadnamenlänge     */
#define extlen 4                 /* max. Extensions-Länge   */
#define namelen 9                /* Länge Filename mit '\0' */

#define bpz 512                  /* Bytes pro Bildzeile     */
#define wpz 256                  /* Words pro Bildzeile     */
#define zpb 512                  /* Zeilen pro Bild         */

#define hell 0xE8                /* Helligkeitswerte zum Spektren zeichnen */
#define dunkel 0x30

typedef unsigned char uchar;

typedef struct SPEKTRUM
  {
  int von, bis;
  int wert [bpz];
  struct SPEKTRUM* next;
  }
  spektrum;

#define getch mygetch

/* iris00.c */
int   mygetch ( void );
void  errmalloc ( void );
void  clearkbd ( void );
void  getreturn ( void );
int   charinvalid ( const char*, const char* );
void  getfname ( char*, int );
void  getpath ( char*, int );
char* dateiname ( char*, const char*, const char* );
int   getzahl( int, int );
char  getcset ( const char* );
char  askcset ( const char* );
int   janein ( void );
int   getstop ( void );
void  is_draw_str ( int, int, int, const char* );
void  drawstr_background ( void );

/* iris01.c */
int permanent ( int frb, int olut_nummer );
void fastpart ( int frb );
void kopfstand ( int frb );
void bildspiegeln ( int frb );

/* iris02.c */
void setbildpfad ( void );
void savedialog ( int frb, const char* fname );
void saveSplitterDialog ( int frb, const char* fname );
void loaddialog ( int frb );
void savevoll ( int frb );
void loadvoll ( int frb );
void loadvollNONIRIS ( int frb );

/* iris03.c */
void changescan ( void );
void spektren ( int );
void spektrenplus ( int );
void spektrumout ( int, const int, const spektrum* );
void press ( int );
int  zeilencrrs ( int, int );

/* iris04.c */
double pixel2wahr ( int );                 /* in iris04b.c */
void spauswertung ( int, spektrum*, int, const char* );
void spwahrewerte ( int, spektrum*, int );
void nt_grafik ( int );
void wwtabelle ( void );

/* iris05.c */
void helligkeit ( int );
void displaygrauwerte ( int );

/* iris06.c */
void goulddaten ( int forceinit, int frage );
void gouldnamen ( void );

/* iris07.c */
long diskfree ( int nr );
void doscmd ( void );
void dosshell ( int volkov );
void dosdir ( void );

/* iris08.c */
void frg_init ( const char* isfile );
void frg_termit ( void );
void frg_framegrabber ( int nummer );
int  frg_aufnehmen ( int frb );
void setsavefiles ( void );
void savefiles ( int frb );

/* iris09.c */
void luteditor ( int frb );

/* ubpixm.asm*/
int _cdecl _far isb_get_pixel ( int, int, unsigned, char _far* );
int _cdecl _far isb_put_pixel ( int, int, unsigned, const char _far* );

/* stredit.c */
void stredit ( char* buffer, int sizeofbuffer );
