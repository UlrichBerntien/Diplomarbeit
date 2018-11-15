/*
**  Gemeinsame Funktionen für TGPLOT.C und TGTEX.C
*/

#ifndef _far
#  define _far    far
#  define _near   near
#  define _pascal pascal
#  define _cdecl  cdecl
#endif

#define LOCAL _near _pascal
#define KB * 1024

#define UNUSED(x) ( (void) x );

typedef const struct tgp_zeile zeile;
typedef const struct tgp_graf graf;
typedef const struct tgp_ff ff;

/* TG.... */

void error ( const char* form, ... );

/* TG00.C */

void plotfile_open ( const char* name );
void plotfile_close ( void );
enum tgp plotfile_next ( void );
zeile* plotfile_getzeile ( void );
graf* plotfile_getgraf ( void );
ff* plotfile_getff ( void );

/* TG01.C */

void out_open ( const char* name );
void out_close ( void );
FILE* out_getfile ( void );
void out_puts ( const char* );
void out_printf ( const char* ,... );

