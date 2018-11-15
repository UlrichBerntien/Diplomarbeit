#ifndef __ZTC__
#  define MK_FP(seg,off) ((void _far*) (((unsigned long) (seg)<<16 ) | (off)))
#  ifdef FP_OFF
#    undef FP_OFF
#    undef FP_SEG
#  endif
#  define FP_OFF(ptr) ( (unsigned) (unsigned long) (void _far*) (ptr) )
#  define FP_SEG(ptr) ( (unsigned) ((unsigned long) (void _far*) (ptr) >>16 ))
#endif

#define UNUSED(x) ( (void) (x) )

#define LOCAL _near _pascal

typedef unsigned char byte;

#define breite 512           /* Breite eines IMG-Bilds in Pixel     */
#define hoehe  256           /* Höhe eines IMG-(Halb)Bilds in Pixel */

/***** in imgview.c *************************************************/

/*
**  MAIN
*/
int main ( int argc, char* argv [] );

/*
**  Ausgabe einer Fehlermeldung und Programmende
**  form : für vfprintf
*/
void error ( const char* form, ... );

/*
**  malloc() mit Programmabbruch im Fehlerfall
**  size   : Größe des benötigten Speicherblocks
**  return : auf einen Speicherblock, nie NULL
*/
void* mallocOk ( unsigned size );

/***** in viewasm.asm ***********************************************/

/*
**  Erfragen des aktuellen Video-Modes über das Videobios
**  return : der Videomode
*/
byte _cdecl getVideoMode ( void );

/*
**  Setzen des aktuellen Video-Modes über das Videobios
**  mode : der gewünschte Videomode
*/
void _cdecl setVideoMode ( byte mode );

/*
**  Mitteln zweier Bildzeilen zu einer Bildzeile
**  zwei : Buffer mit zwei Bildzeilen (aufeinander folgend)
**  m    : Buffer für das Mittel zwischen den beiden Zeilen
*/
void _cdecl zeilenMittel ( const byte zwei [breite*2], byte m [breite] );

/*
**  Ausgabe des Bilds auf eine EGA/VGA-Karte
**  bild        : Zeiger auf das Bild, Array von Bytes der Größe breite*hoehe
**  farbtabelle : Tabelle zur Wandlung der 8 Bit Graustufen des Bilds auf
**                die 4 Bit Farb/Graustufen der Karte
**                Das Array muß auf dem Stack liegen !!!
*/
void _cdecl egaOut ( const byte* bild, const byte farbtabelle [256] );

/*
**  Kopieren von src nach dest, dabei alle Bytes aus src verdreifachen:
**  dest[0] = scr[0], dest[1] = scr[0], dest[2] = src[0], dest[3] = src[1] ...
**  dest : Feld für Destination Bytes, Länge size * 3
**  src  : Feld mit den Source Bytes, Länge size
**  size : Anzahl der zu kopierenden Source-Bytes (<= 64K/3)
*/
void _cdecl multiCopy ( byte* dest, const byte* src, unsigned size );

/***** in ega.c *****************************************************/

/*
**  Testen ob eine EGA-Karte aktiv ist
**  return : Ist eine EGA-Karte aktiv ?
*/
int ega_testen ( void );

/*
**  Zustand der EGA-Karte retten
*/
void ega_retten ( void );

/*
**  den mit ega_retten() geretteten Zustand wiederherstellen
*/
void ega_ruecksetzen ( void );

/*
**  die EGA-Karte für Ausgabe des IMG-(Halb)Bilds initialisieren
*/
void ega_initialisieren ( void );

/*
**  Ausgeben des IMG-(Halb)Bilds auf die EGA-Karte
*/
void ega_anzeigen ( const byte* zeilen [hoehe] );

/***** in vga.c *****************************************************/

int  vga_testen ( void );
void vga_retten ( void );
void vga_ruecksetzen ( void );
void vga_initialisieren ( void );
void vga_anzeigen ( const byte* zeilen [hoehe] );

/***** in svga.c ****************************************************/

int  svga_testen ( void );
void svga_retten ( void );
void svga_ruecksetzen ( void );
void svga_initialisieren ( void );
void svga_anzeigen ( const byte* zeilen [hoehe] );

/*
**  Setzen einer Graustufen Tabelle aus 256 Farben aus 256K Farben
*/
void setGrautabelle ( void );

/***** in tseng.c ***************************************************/

int  tseng_testen ( void );
void tseng_retten ( void );
void tseng_ruecksetzen ( void );
void tseng_initialisieren ( void );
void tseng_anzeigen ( const byte* zeile [hoehe] );