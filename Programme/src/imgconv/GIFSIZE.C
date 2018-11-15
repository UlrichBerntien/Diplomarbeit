/*
**  Turbo C 2.0
**  .09.1994  Jens Raacke + Ulrich Berntien
**
**  Achtung : beim Kompilieren ausrichten der Datenstrukturen
**            auf Byte-Grenzen einschalten.
**
**  14.09.1994  Beginn, Entstanden aus 2GIF.C (IMG-Konverter)
*/

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************************************************************/

/*
**  Kurzbeschreibung des Programms
*/
static const char manual [] =
  "GIFSIZE .09.1994 J.Raacke + U.Berntien\n"
  "Aufrufformat:\n"
  "\n"
  "     gifsize  <infile> [outfile]\n"
  "\n"
  "<infile>  : Name eines GIF-Files.\n"
  "            Falls keine Extension angegeben, wird \"gif\" benutzt.\n"
  "[outfile] : Name eines anzulegenden ASCII-Files.\n"
  "            Dort wird die Breite und die Höhe des Bilds\n"
  "            in Pixel abgelegt.\n"
  "            Falls keine Extension angegeben, wird \"siz\" benutzt.\n"
  "            Falls der Name nicht angegeben ist, wird <infile>\n"
  "            benutzt.\n"
  "";

/*
**  Standart-Extension für ein GIF-File
*/
static const char extensionGif [] = ".gif";

/*
**  Standart-Extension für das ASCII-File
*/
static const char extensionAscii [] = ".siz";

/********************************************************************/

/*
**  Aufbau des GIF-Formats:
**    gifHeader
**    optional: Globale Farbtabelle
**    Beliebig oft wiederholbar :
**      Teilbild
**      optional: GIF-Erweiterungsblock
**    GIF-Terminator
*/

/*
**  einfache Typen
*/
typedef unsigned char BYTE;
typedef unsigned short WORD;

/*
**  Anfang der Kennung des GIF-Formats
*/
static const BYTE gifSignatur[3] = "GIF";

/*
**  Aufbau des Fileheaders
**  Enthält Informationen über das Bild
*/
typedef struct gifHeaderTAG
  {
  BYTE signatur [6];               /* Signatur der Datei (gif_signatur) */
  WORD width;                      /* Bildbreite in Pixel               */
  WORD height;                     /* Bildhöhe in Pixel                 */
  BYTE flag;                       /* gif_resolution                    */
  BYTE background;                 /* Hintergrundfarbe                  */
  BYTE pixel_aspect_ratio;         /* Abbildungsverhältnis =            */
                                   /*           Pixelbreite / Pixelhöhe */
                                   /* pixel_aspect_ratio =              */
                                   /*       Abbildungsverhältnis*64 -15 */
  }
  gifHeader;

/*
**  Lesen des Headers aus dem GIF-File
**  name   : Name des GIF-Files
**  header : dort den Header abspeichern
**  return : war das Lesen erfolgreich ?
*/
static int readHeader ( const char* name, gifHeader* header )
  {
  int ok;
  int fh = open( name, O_RDONLY | O_BINARY );
  ok = fh > -1;
  if( ok ) ok = read( fh, header, sizeof (gifHeader) ) == sizeof (gifHeader);
  if( ok ) close( fh );
  return ok;
  }

/*
**  Testen ob der Header von einem GIF-File ist
**  header : der Header
**  return : ist der Header ok ?
*/
static int checkHeader ( const gifHeader* header )
  {
  return memcmp( header->signatur, gifSignatur, sizeof gifSignatur ) == 0;
  }

/*
**  Ausgeben der Breite und der Höhe des GIF-Files
**  laut Header in Pixel in Klartext in ein ASCII-File
**  name   : Name des anzulegenden ASCII-Files
**  header : die Daten des Headers
**  return : alles ok ?
*/
static int writeSize ( const char* name, const gifHeader* header )
  {
  char buffer [20];
  int ok;
  int fh;
  int size;
  size = sprintf( buffer, "%d\n%d\n", header->width, header->height );
  ok = size != EOF && size < sizeof buffer;
  if( ok ) ok = ( fh = creat( name, 0xff ) ) > -1;
  if( ok ) ok = write( fh, buffer, size ) != size;
  if( ok ) close( fh );
  return ok;
  }

/*
**  Anhängen einer Extension an einen Filenamen.
**  name   : der Name ggf. ohne Extension
**  ext    : Standart-Extension
**  option : == 0 -> vorhandene Extension bleibt erhalten
**           != 0 -> vorhandene Extension wird überschrieben
**  return : neu angelegter Name mit Extension
*/
static char* extension ( const char* name, const char* ext, int option )
  {
  const int lenName = strlen( name );
  const int lenExt = strlen( ext );
  char* buffer = malloc( lenName + lenExt + 1 );
  int i;
  if( buffer == NULL )
    {
    fputs( "Out of Memory\n", stderr );
    exit( EXIT_FAILURE );
    }
  memcpy( buffer, name, lenName+1 );
  i = lenName;
  while( i > 0 && buffer[i] != '.' && buffer[i] != '\\' ) --i;
  if( i == 0 || buffer[i] == '\\' )
    memcpy( buffer + lenName, ext, lenExt+1 );
  if( option != 0 && buffer[i] == '.' )
    memcpy( buffer + i, ext, lenExt + 1 );
  return buffer;
  }

/*
**  M A I N
*/
int main ( int argc, const char** argv )
  {
  gifHeader header;
  const char* nameGif;
  const char* nameAscii;
  if( argc != 2 && argc != 3 )
    {
    fputs( manual, stderr );
    exit( EXIT_FAILURE );
    }
  nameGif = extension( argv[1], extensionGif, 0 );
  if( argc == 3 )
    nameAscii = extension( argv[2], extensionAscii, 0 );
  else
    nameAscii = extension( argv[1], extensionAscii, 1 );
  if(    readHeader( nameGif, &header )
      && checkHeader( &header )
      && writeSize( nameAscii, &header ) )
    return EXIT_SUCCESS;
  else
    return EXIT_FAILURE;
  }