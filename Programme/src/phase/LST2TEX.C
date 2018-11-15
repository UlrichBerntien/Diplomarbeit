/*
**  C   ( Zortech C V2.1 / Microsoft C V5.10 / Turbo C V2.0 )
**
**  Ulrich Berntien .04.1993
**
**  Umsetzen eines normalen Programmlistings in eine TeX-Datei
**  für den Ausdruck.
**
**  22.04.1993  Beginn
**  24.05.1993  In den Listings Tabulatoren ausgeben
**  18.08.1993  Modul DIRGET.C eingebunden
*/

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "common.h"

#ifndef EXIT_SUCCESS        /* falls MSC V 5.10 */

#  define EXIT_SUCCESS 0
#  define EXIT_FAILURE 1

#  define strcmpl strcmpi

  /*
  **  String anhängen
  **  buffer : dorthin kopieren
  **  was    : der String wird kopiert
  **  return : Zeiger hinter letztes kopiertes Zeichen in buffer
  */
  static char* stpcpy( char* buffer, const char* was )
    {
    while( *was != '\0' ) *buffer++ = *was++;
    return buffer;
    }

#endif

#ifdef __TURBOC__
#  define strcmpl  stricmp
#endif

/********************************************************************/

/*
**  ASCII-Konstanten
*/
#define EOS '\0'
#define TAB '\t'
#define CR  '\r'

/*
**  ein Kilobyte
*/
#define KB * 1024

/********************************************************************/

/*
**  Kurzanleitung des Programms
*/
static const char manual [] =
  "LST2TeX    .04.1993    Ulrich Berntien\n"
  "Umsetzen von Programm-Sourcefiles in LaTeX-Datei für den\n"
  "Ausdruck.\n"
  "Aufrufformat:\n\n"
  "   LST2TeX arg1 [ arg2 ... argn ]\n\n"
  "arg :\n"
  "   @<filename>  weitere Argumente aus Responsefile lesen\n"
  "   <filneame>   diese(s) Source-File(s) übersetzen, kann Joker\n"
  "                *, ? enthalten.\n"
  "   -<filename>  In dieses File die Ausgabe schreiben,\n"
  "                Standart Filename : PROG.TEX\n"
  "   /xxx         Nach xxx Spalten Zeilenumbruch, 0 -> kein Umbruch\n"
  "   +<filename>  File unbearbeitet in die Ausgabe kopieren\n"
  "   :<command>   Kommando an System übergeben.\n";

/*
**  Fehlermeldungen
*/
static const char noheap [] = "Out of Heap\n";

/*
**  Eine Fehlermeldung ausgeben und Programm beenden
**  msg   : die Meldung
**  fname : Name des Files, das den Fehler ausgelöst hat
*/
static void error ( const char* msg, const char* fname )
  {
  fputs( "Fehler : ", stderr );
  fprintf( stderr, msg, fname );
  fprintf( stderr, "LIB-ERROR : %s\n", strerror( errno ) );
  exit( EXIT_FAILURE );
  }

/********************************************************************/

/*
**  so fängt es in TeX an
*/
static const char start [] =
  "\\section{Programm}\r\n"
  "\\par\r\n";

/*
**  so hört es in TeX auf
*/
static const char end [] =
  "";

/*
**  so fängt ein Listing eines Files in TeX an
*/
static const char startfile [] =
  "\\subsection*{%s}\r\n"
  "\\label{%s}\r\n"
  "{\\tiny\r\n"
  "\\begin{tabbing}\r\n";

/*
**  so hört ein Listing eines Files in TeX auf
*/
static const char endfile [] =
  "\\end{tabbing}"
  "}\r\n"
  "\\par\r\n";


/*
**  Spalten bis Zeilenumbruch erfolgt, 0 = kein Umbruch
*/
static int umbruch = 0;

/********************************************************************/

/*
**  Struktur für Tabulatoren
*/
typedef struct tabsTAG
  {
  int* diffs;   /* Zeichen zwischen den Tab.positionen, mit 0 terminiert */
  int* set;     /* Zeichen in TeX setzen zwischen die Tabs               */
  int* pos;     /* Tabulator Positionen, mit INT_MAX terminiert          */
  char* line;   /* Tabulatoren in TeX setzen                             */
  }
  tabs;

/*
**  Tabulatoren für C-Programme und Header-Files
*/
static int diffsC [] = { 2,2,2,2,2,2,2,4,4,4,4,4,4,0 };
static tabs tabulatorC =
  {
  diffsC,
  diffsC,
  NULL,
  NULL
  };

/*
**  Tabulatoren für ASM-Programme
*/
static int diffsASM [] = { 8, 6, 32, 0 };
static int setASM [] = { 8, 5, 21, 0 };
static tabs tabulatorASM =
  {
  diffsASM,
  setASM,
  NULL,
  NULL
  };

/*
**  Feststellen von welcher Art das File ist
**  filename : an der Erweiterung des Filenamens den Typ erkennen
**  return   : die Tabulator-Struktur für den Filetyp
*/
static const tabs* gettab ( const char* filename )
  {
  static const char asm_ext [] = ".ASM";
  const char* extstart = strrchr( filename, '.' );
  int isasm = strcmpl( extstart, asm_ext ) == 0;
  return isasm ? &tabulatorASM : &tabulatorC;
  }

/*
**  Tabulatoren in Zeile einsetzen
**  in  : Eingabestring, durch '\0' oder '\r' terminiert
**  out : Buffer für Ausgabestring
**  tab : die Information über die Tabulatoren
*/
static void inserttab ( const char* in, char* out, const tabs* tab )
  {
  char akku;
  int spalte;
  const int* tabpos = tab->pos;
  int wartendetabs = 1;
  int blanks = 0;
  for( spalte = 0; ( akku = in[spalte] ) != EOS && akku != CR; ++spalte )
    {
    if( isspace( akku ) )
      blanks++;
    else
      {
      while( blanks ) { --blanks; *out++ = 0x20; }
      *out++ = akku;
      }
    if( spalte > *tabpos )
      {
      ++wartendetabs;
      ++tabpos;
      }
    if( blanks >= 2 && spalte == *tabpos )
      {
      while( wartendetabs )
        {
        --wartendetabs;
        *out++ = TAB;
        }
      blanks = 0;
      }
    }
  *out = EOS;
  }

/*
**  Die noch freien Felder in der Tabulatoren-Struktur ausfüllen
**  tab : Zeiger auf die auszufüllende Struktur
*/
static void inittab ( tabs* tab )
  {
  static const char phantom [] = "\\hphantom{";
  static const char eol [] = "\\\\\r\n";
  char* ptr;
  int pos = 0;
  int chars = 0;
  int i;
  for( pos = 0; tab->diffs[pos] != 0; ++pos )
    chars += tab->set[pos];
  tab->pos = malloc( ( pos + 1 ) * sizeof (int) );
  tab->line = malloc( chars + pos * ( 3 + sizeof phantom ) + sizeof eol );
  if( tab->pos == NULL || tab->line == NULL ) error( noheap, NULL );
  ptr = tab->line;
  chars = -1;
  for( i = 0; i < pos; ++i )
    {
    int j;
    int fueller = tab->set[i];
    tab->pos[i] = chars += tab->diffs[i];
    ptr = stpcpy( ptr, phantom );
    for( j = 0; j < fueller; ++j ) *ptr++ = 'x';
    ptr = stpcpy( ptr, "}\\=" );
    }
  tab->pos[i] = INT_MAX;
  strcpy( ptr, eol );
  }

/*
**  Initialisieren der Tabulatoren für C und Assembler
*/
static void inittabs ( void )
  {
  inittab( &tabulatorC );
  inittab( &tabulatorASM );
  }

/********************************************************************/

/*
**  Öffnen eines Files, mit Fehlerbehandlung
**  fname  : Name des Files
**  mode   : Modus des fopen-Befehls
**  return : das geöffnete File
*/
static FILE* Fopen ( const char* fname, const char* mode )
  {
  FILE* file = fopen( fname, mode );
  if( file == NULL ) error( "kann File '%s' nicht öffnen.\n", fname );
  return file;
  }

/*
**  Schreibt String in File, mit Fehlerkontrolle
**  str   : der String wird geschreiben
**  file  : in das File
**  fname : Name des Files
*/
static void Fputs ( const char* str, FILE* file, const char* fname )
  {
  if( fputs( str, file ) == EOF )
    error( "beim Schreiben (fputs) in File '%s'.\n", fname );
  }

/*
**  Schreibt in File, mit Fehlerkontrolle
**  buffer : die zu schreibenden Daten
**  size   : Anzahl der zu schreibenden Bytes
**  file   : dorthin schreiben
**  fname  : Name des Files
*/
static void Fwrite ( const void* buffer, size_t size, FILE* file, const char* fname )
  {
  if( fwrite( buffer, size, 1, file ) != 1 )
    error( "Beim Schreibe (fwrite) in File '%s'.\n", fname );
  }

/*
**  Schreibt in File, mit Fehlerkontrolle
**  fname : Name des Files
**  file  : dorthin wird geschrieben
**  form  : form für vfprintf
*/
static void Fprintf ( const char* fname, FILE* file, const char* form, ... )
  {
  va_list argptr;
  va_start( argptr, form );
  if( vfprintf( file, form, argptr ) < 0 )
    error( "beim Schreiben (fvprintf) in File '%s'.\n", fname );
  va_end( argptr );
  }

/*
**  Schließt File mit Fehlerkontrolle
**  file  : das File wird geschlossen
**  fname : Name des Files
*/
static void Fclose ( FILE* file, const char* fname )
  {
  if( fclose( file ) != 0 )
    error( "beim Schließen des Files '%s'.\n", fname );
  }

/********************************************************************/

/*
**  Name des Ausgabe TeX-Files
*/
static const char texstdname [] = "PROG.TEX";
static const char* texfilename = texstdname;

/*
**  Filehandle des geöffneten TeX-Files
*/
static FILE* texfile = NULL;

/*
**  neues Ausgabe TeX-File ankündigen
**  ggf. das Ausgabe TeX-File schließen
**  fname : Name des Files (mit Pfad)
*/
static void newtexfile ( const char* fname )
  {
  if( texfile != NULL )
    {
    Fputs( end, texfile, fname );
    Fclose( texfile, fname );
    texfile = NULL;
    }
  if( texfilename != texstdname ) free( (void*) texfilename );
  texfilename = strdup( fname );
  if( texfilename == NULL ) error( noheap, NULL );
  }

/*
**  Zugriff auf das Ausgabe TeX-File ggf. öffnen des Files
**  return : File, write binary
*/
static FILE* gettexfile ( void )
  {
  if( texfile == NULL )
    {
    texfile = Fopen( texfilename, "ab" );
    setvbuf( texfile, NULL, _IOFBF, 30 KB );
    Fputs( start, texfile, texfilename );
    }
  return texfile;
  }

/*
**  Transformiert eine Zeile zeichenweise in TeX-Format
**  berücksichtigt dabei Umlaute (WISCII und IBM-PC Zeichensatz) und
**  TeX-Steuerzeichen
**  in  : Eingabestring
**  out : Buffer für die übersetzte Zeile
*/
static void zeile ( const char* in, char* out )
  {
  int index = 0;
  char* ptr = out;
  while( in[index] != EOS )
    {
    switch( in[index] )
      {
      case '\x20' : *ptr++ = '~';
                    break;
      case '{'    :
      case '}'    :
      case '#'    :
      case '$'    :
      case '_'    :
      case '%'    :
      case '&'    : *ptr++ = '\\';
                    *ptr++ = in[index];
		    break;
      case '|'    :
      case '>'    :
      case '<'    :
      case '-'    : *ptr++ = '$';
                    *ptr++ = in[index];
                    *ptr++ = '$';
                    break;
      case '*'    : if( ptr == out )
                      ptr = stpcpy( ptr, "{*}" );
                    else
                      *ptr++ = '*';
                    break;
      case '"'    : ptr = stpcpy( ptr, "{\\grqq}" );
                    break;
      case TAB    : ptr = stpcpy( ptr, "\\>" );
                    break;
      case '\x84' :        /* jeweils IBM-PC und WANG-PC Code */
      case '\xA3' : ptr = stpcpy( ptr, "\"a" );
                    break;
      case '\x94' :
      case '\xCC' : ptr = stpcpy( ptr, "\"o" );
                    break;
      case '\x81' :
      case '\xE8' : ptr = stpcpy( ptr, "\"u" );
                    break;
      case '\x8E' :
      case '\x93' : ptr = stpcpy( ptr, "\"A" );
                    break;
      case '\x99' :
      case '\xBC' : ptr = stpcpy( ptr, "\"O" );
                    break;
      case '\x9A' :
      case '\xD8' : ptr = stpcpy( ptr, "\"U" );
                    break;
      case '\xE1' :
      case '\xEE' : ptr = stpcpy( ptr, "\"s" );
                    break;
      case '\xF8' :
      case '\x80' : ptr = stpcpy( ptr, "$^\\circ$" );
                    break;
      case '^'    : ptr = stpcpy( ptr, "$\\uparrow$" );
                    break;
      case '\\'   : ptr = stpcpy( ptr, "$\\backslash$" );
                    break;
      default     : if( isprint(in[index]) ) *ptr++ = in[index];
      }
    ++index;
    }
  if( ptr == out ) *ptr++ = '~';
  *stpcpy( ptr, "\\\\\r\n" ) = EOS;
  }

/*
**  Zeilenumbruch durchführen, max. Zeilenlänge in globaler Variable
**  umbruch
**  str    : String mit der Zeile, wird ggf. abgeschnitten
**  return : Rest der Zeile oder NULL (free auf zurückgebenen String)
*/
static char* zeilenumbruch ( char* str )
  {
  int i = 0;
  char* rest;
  while( i < umbruch && str[i] != EOS ) ++i;
  if( str[i] != EOS )
    {
    rest = strdup( str + i );
    str[i] = EOS;
    i = 0;
    while( isspace( rest[i] ) && rest[i] != EOS ) ++i;
    if( rest[i] == EOS )
      {
      free( rest );
      rest = NULL;
      }
    }
  else
    rest = NULL;
  return rest;
  }

#if 0
**  Falls eine Zeile /***..***/ gebrochen wird,
**  nur abschneiden
**  line   : die Zeile
**  rest   : der Zeilenrest
**  return : der Zeilenrest oder NULL, wenn er gelöscht wurde
#endif
static char* starline ( char* line, char* rest )
  {
  int last;
  int i = 1;
  int match = *rest == '*';
  if( match )
    {
    while( rest[i] == '*' ) ++i;
    match = rest[i++] == '/';
    }
  if( match )
    {
    char akku;
    while( ( akku = rest[i] ) != EOS && isspace( akku ) ) ++i;
    match = rest[i] == EOS;
    }
  match = match && line[ last = ( strlen(line) - 1 ) ] == '*';
  if( match )
    {
    line[ last ] = '/';
    free( rest );
    rest = NULL;
    }
  return rest;
  }

/*
**  Übersetzen eines Files in das Ausgabe TeX-File
**  fname : Name für den Fileszugriff (mit Pfad)
**  name  : Name des Files ohne Pfad
*/
static void transform ( const char* fname, const char* name )
  {
  FILE* outfile = gettexfile();
  FILE* infile = Fopen( fname, "rb" );
  const tabs* tabart = gettab( fname );
  char* buffera = malloc( 1 KB );
  char* bufferb = malloc( 1 KB );
  if( buffera == NULL || bufferb == NULL ) error( noheap, NULL );
  printf( "transformiere '%s'\n", fname );
  Fprintf( texfilename, texfile, startfile, name, name );
  Fputs( tabart->line, outfile, texfilename );
  while( fgets( buffera, 200, infile ) != NULL )
    {
    char* rest;
    if( umbruch != 0 )
      {
      rest = zeilenumbruch( buffera );
      if( rest ) rest = starline( buffera, rest );
      }
    else
      rest = NULL;
    inserttab( buffera, bufferb, tabart );
    zeile( bufferb, buffera );
    Fputs( buffera, outfile, texfilename );
    if( rest )
      {
      zeile( rest, buffera );
      Fprintf( texfilename, texfile, "$\\hookrightarrow$%s", buffera );
      free( rest );
      }
    }
  Fputs( endfile, outfile, texfilename );
  free( buffera );
  free( bufferb );
  Fclose( infile, fname );
  }

/*
**  Aus Eingangsfile direkt in das Ausgabe TeX-File kopieren
**  fname : Filename (mit Pfad) des Eingabefiles
*/
static void copy( const char* fname )
  {
  FILE* outfile = gettexfile();
  FILE* infile = Fopen( fname, "rb" );
  char* buffer = malloc( 10 KB );
  int size;
  if( buffer == NULL ) error( noheap, NULL );
  printf( "kopiere '%s'\n", fname );
  while( ( size = fread( buffer, 1, 10 KB, infile ) ) > 0 )
    Fwrite( buffer, size, outfile, texfilename );
  free( buffer );
  Fclose( infile, fname );
  }

/********************************************************************/

/*
**  Eingabefile(s) in das Ausgabe TeX-File schreiben
**  fname : Filename (mit Pfad), kann Joker enthalten
**  trafo : Das File transformieren
*/
static void work ( const char* fname, int trafo )
  {
  const char* filename = dirget_first( fname );
  while( filename != NULL )
    {
    if( trafo )
      transform( filename, dirget_name() );
    else
      copy( filename );
    filename = dirget_next();
    }
  }

/*
**  Prototyp für rekursiven aufruf
*/
static void argument ( const char* );

/*
**  Argumente aus Responsefile holen
**  fname : Filename des Responsefiles
*/
static void responsefile ( const char* fname )
  {
  FILE* file = Fopen( fname, "r" );
  char buffer [128];
  printf( "befolge '%s'.\n", fname );
  while( fgets( buffer, sizeof buffer, file ) != NULL )
    {
    char akku;
    char* write = buffer;
    const char* read = buffer;
    while( *read != EOS )
      if( isprint( akku = *read++ ) ) *write++ = akku;
    *write = EOS;
    if( buffer[0] != EOS && buffer[0] != '%' )
      argument( buffer );
    }
  Fclose( file, fname );
  }

/*
**  Schalterargument auswerten
**  str : der Schalter
*/
static void schalter ( const char* str )
  {
  int ok = 1;
  int i;
  for( i = 0; ok && str[i] != '\0'; ++i )
    ok = isdigit( str[i] );
  if( ok )
    umbruch = atoi( str );
  else
    error( "Unbekannter Schalter : '\%s'\n", str );
  }

/*
**  Ein Argument aus Befehlszeile oder Responsefile bearbeiten
**  arg : das Argument
*/
static void argument ( const char* arg )
  {
  switch( arg[0] )
    {
    case '@' : responsefile( arg + 1 );
               break;
    case '-' : newtexfile( arg + 1 );
               break;
    case '+' : work( arg + 1, 0 );
               break;
    case '/' : schalter( arg + 1 );
               break;
    case ':' : puts( arg );
               system( arg + 1 );
               break;
    default  : work( arg, 1 );
    }
  }

/*
**  M A I M
*/
int main ( int argc, char* argv [] )
  {
  if( argc < 2 )
    {
    fputs( manual, stderr );
    exit( EXIT_FAILURE );
    }
  inittabs();
  while( ++argv, --argc )
    argument( *argv );
  newtexfile( "NUL" );
  return EXIT_SUCCESS;
  }

