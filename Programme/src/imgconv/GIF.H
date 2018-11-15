/*
**  Aufbau des GIF-Formats aus
**    Th.W.Lipp : Die große Welt der Grafikformate (Teil3)
**    in Microsoft System Journal, 3/1993, Seiten 119 ff.
**  siehe dazu auch
**    Dietmar Bückart : Heiße Pixel
**    in mc, Mai 1991, Seiten 106 ff.
**
**  Aufbau der Datei:
**    gif_header
**    optional: Globale Farbtabelle
**    Beliebig oft wiederholbar :
**      Teilbild
**      optional: GIF-Erweiterungsblock
**    gif_terminator
**
**  Hier wird die globale Farbtabelle und genau ein Teilbild benutzt
**  und 8 Bit pro Pixel angenommen.
*/

#pragma ZTC align 1

/*
**  Konstanten
*/
static const BYTE gif_signatur[6] =        /* Kennung des GIF-Formats   */
   "GIF89a";
static const BYTE gif_signaturAlt[6] =     /* alte Kennung              */
   "GIF87a";
static const BYTE gif_ext_sep = '!';       /* Kennung eines Erw.blocks  */
static const BYTE gif_ext_end = 0x00;      /* -"- Ende eines Erw.blocks */
static const BYTE gif_image_sep = ',';     /* Kennung eines Teilbilds   */
static const BYTE gif_image_end = 0x00;    /* -"- Ende eines Teilbilds  */
static const BYTE gif_terminator = ';';    /* Kennung des Dateiendes    */

/*
**  Aufbau des GIF Resolution Flags
**  (Zortech C beginnt mit dem Speichern der Bitfeldern bei dem
**   niederwertigsten Bit.)
**  Benutzt 8 Bit, aber der Compiler belegt dafür 16 Bit, daher wird
**  dieses Feld als BYTE im gif_header geführt.
*/
typedef struct gif_resolutionTAG
  {
  unsigned pixel : 3;          /* Bits pro Pixel - 1                        */
  unsigned sort : 1;           /* Farben nach Anzahl Pixel/Farbe sortiert ? */
  unsigned colors : 3;         /* 2^(colors+1) = Anzahl Farben in Tabelle   */
  unsigned colormap : 1;       /* existiert gobale Farbtabelle ?            */
  }
  gif_resolution;

/*
**  Aufbau des Fileheaders
**  Enthält Informationen über das Bild
*/
typedef struct gif_headerTAG
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
  gif_header;

/*
**  Aufbau eines Elements der Farbtabelle.
**  Die Anzahl der Farben wird bestimmt durch die Angabe 'colors'
**  Anzahl = 2 hoch (colors+1)
*/
typedef struct gif_rgbTAG
  {
  BYTE rgbRed;              /* Intensität der Grundfarbe Rot  */
  BYTE rgbGreen;            /* Intensität der Grundfarbe Grün */
  BYTE rgbBlue;             /* Intensität der Grundfarbe Blau */
  }
  gif_rgb;

/*
**  Aufbau eines Teilbilds
**    gif_image_descriptor
**    optional : lokale Farbtabelle (Aufbau wie globale Farbtabelle)
**    BYTE code_size
**    beliebig oft wiederholt : gif_data
**    gif_image_end
**  code_size = Anzahl Datenbits - 1 mit der die LZW-Codes beginnen
*/

/*
**  wird im GIF Image Descriptor benutzt
**  Benutzt 8 Bit, aber der Compiler belegt dafür 16 Bit, daher wird
**  dieses Feld als BYTE im gif_image_descriptor geführt.
*/
typedef struct gif_image_flagsTAG
  {
  unsigned colors : 3;         /* #Farben in Tabelle = 2 hoch (colors+1)    */
  unsigned reserved : 2;       /* muß 0 sein                                */
  unsigned sort : 1;           /* Farben nach Anzahl Pixel/Farbe sortiert ? */
  unsigned interlaced : 1;     /* = 0, bei sequentieller Folge der Zeilen   */
  unsigned colormap : 1;       /* folgt eine lokale Farbtabelle ?           */
  }
  gif_image_flags;

/*
**  Aufbau des GIF Image Descriptors
**  enthält Informationen über ein Teilbild
*/
typedef struct gif_image_descriptorTAG
  {
  BYTE separator;                /* Kennung für ein Teilbild ','         */
  WORD left;                     /* Abstand zum linken Bildrand in Pixel */
  WORD top;                      /* Abstand zum oberen Bildrand in Pixel */
  WORD width;                    /* Breite des Teilbilds in Pixel        */
  WORD height;                   /* Höhe des Teilbilds in Pixel          */
  BYTE flags;                    /* gif_image_flags                      */
  }
  gif_image_descriptor;

/*
**  Aufbau eines Datenblocks innerhalb eines Teilbilds oder eines
**  GIF-Erweiterungsblocks
*/
typedef struct gif_dataTAG
  {
  BYTE len;                /* Anzahl der folgenden Bytes */
  BYTE x [0xFF];           /* Maximal 255 Datenbytes     */
  }
  gif_data;

#pragma ZTC align
