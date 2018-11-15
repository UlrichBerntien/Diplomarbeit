/*
**  Aufbau des TIF-Formats aus
**    Th.W.Lipp : Die große Welt der Grafikformate (Teil4)
**    in Microsoft System Journal, 4/1993, Seiten 90 ff.
**  dazu auch
**    Ingo Eickmann : Grafik im Griff
**    in mc, Oktober 1990, Seiten 56 ff.
*/

/*
**  Konstanten
*/
static const WORD tiff_little = 0x4949;  /* Daten im Little Endian Format */
static const WORD tiff_big    = 0x4D4D;  /* Daten im Big Endian Format    */
static const WORD tiff_id     = 0x002A;  /* ID-Code für TIF-Format        */

/*
**  Aufbau des Fileheaders
**  Enthält Informationen über das File
*/
typedef struct tiff_headerTAG
  {
  WORD endian;       /* Format der Zahlen, tiff_little für Intel */
  WORD id;           /* Identifikation des TIF-Formats, tiff_id  */
  LONG first_ifd;    /* Fileoffset des ersten IFDs               */
  }
  tiff_header;

/*
**  Codes für Datentypen im TIF-Format, wird in tiff_entry benutzt
*/
typedef enum tiff_typTAG
  {
  _BYTE      = 1,        /*  8 Bit unsigned                  */
  _ASCII     = 2,        /*  8 Bit chars mit '\0' terminiert */
  _WORD      = 3,        /* 16 Bit unsigned                  */
  _DWORD     = 4,        /* 32 Bit unsigned                  */
  _RATIONAL  = 5,        /* Bruch aus zwei DWORDs            */
  _SBYTE     = 6,        /*  8 Bit signed                    */
  _UNDEFINED = 7,        /*  8 Bit                           */
  _SHORT     = 8,        /* 16 Bit signed                    */
  _LONG      = 9,        /* 32 Bit signed                    */
  _SRATIONAL = 10,       /* Bruch aus zwei LONG              */
  _FLOAT     = 11,       /* 32 Bit IEEE Fließkomma           */
  _DOUBLE    = 12        /* 64 Bit IEEE Fließkomma           */
  }
  tiff_typ;

/*
**  Codes für die verschiedenen Daten, die in einem Tiff-Entry vorkommen
**  können
*/
typedef enum tiff_tagTAG
  {
  _TAGclass           = 0x00FD,
  _TAGsubFileTyp      = 0x00FF,
  _TAGimageWidth      = 0x0100,
  _TAGimageLength     = 0x0101,
  _TAGbitsPerSample   = 0x0102,
  _TAGcompression     = 0x0103,
  _TAGphotometric     = 0x0106,
  _TAGdescription     = 0x010E,
  _TAGsoftware        = 0x0110,
  _TAGstripOffset     = 0x0111,
  _TAGsamplesPerPixel = 0x0115,
  _TAGrowsPerStrip    = 0x0116,
  _TAGstripByteCount  = 0x0117,
  _TAGxResolution     = 0x011A,
  _TAGyResolution     = 0x011B,
  _TAGresolutionUnit  = 0x0128,
  _TAGdateTime        = 0x0132,
  _TAGartist          = 0x013B,
  _TAGpredictor       = 0x013D,
  _TAGcolorMap        = 0x0140,
  _TAGhostComputer    = 0x013C,
  _TAGcopyright       = 0x8298
  }
  tiff_tag;

/*
**  Codes, die im data Feld des Tiff-Entry's vorkommen können
*/
typedef enum tiff_dataTAG
  {
  _dataUncompressed = 1,    /* Arten der Bitmapspeicherung */
  _dataCCITT = 2,
  _dataGroup3Fax = 3,
  _dataGroup4Fax = 4,
  _dataLZW = 5,
  _dataJPEG = 6,
  _dataPackBits = 32773,
  _dataWhiteIsZero = 0,     /* Arten der Farbkodierung */
  _dataBlackIsZero = 1,
  _dataRGB = 2,
  _dataPalette = 3,
  _dataTransparencyMask = 4,
  _dataCYMK = 5,
  _dataYCC = 6,
  _dataCIE = 8,
  _dataRelativ = 1,         /* Arten der Löngenangaben */
  _dataInch = 2,
  _dataCm = 3
  }
  tiff_data;

/*
**  Alle Zahlen, String und sonstige Werte werden in einem festen
**  Format im TIFF-File gespeichert, dieses Verfahren erleichtert
**  die Erweiterung des Formats erheblich.
*/
typedef struct tiff_entryTAG
  {
  short tag;             /* Identifikationsnummer (beschreibt Inhalt) */
                         /* ein Wert aus tiff_tag                     */
  short typ;             /* Format des Datums, ein Wert aus tiff_typ  */
  DWORD count;           /* Anzahl der Datenwerte des 'typ'           */
  DWORD data;            /* Die Daten, oder falls mehr als 32 Bit der */
                         /* Fileoffset an dem die Daten gespeichert   */
  }
  tiff_entry;

/*
**  Aufbau eines Elements der Farbtabelle.
**  Die Intensität im TIFF-File = Intensität im BMP-File * 256
*/
typedef struct tiff_rgbTAG
  {
  WORD   red[0x100];        /* Intensität der Grundfarbe Rot  */
  WORD green[0x100];        /* Intensität der Grundfarbe Grün */
  WORD  blue[0x100];        /* Intensität der Grundfarbe Blau */
  }
  tiff_rgb;

/**** LZW-Kompression ***********************************************/

/*
**  Die LZW-Codes im TIF-Format sind auf 13 Bits beschränkt ohne
**  den letzten LZW-Code beschränkt, also
*/
#define lzw_tiff_count 4094

/*
**  kein gültiger LZW-Code
*/
#define lzw_nocode 0xFFFF

/*
**  Spezieller LZW-Code von TIFF : löschen der LZW-Code-Tabelle
**  = 2 hoch data_size
*/
static const WORD lzw_tiff_clear = 256;

/*
**  Spezieller LZW-Code von TIFF : Ende der Daten
**  = 2 hoch data_size + 1
*/
static const WORD lzw_tiff_end = 257;

/*
**  Ab hier werden die neuen LZW-Codes vergeben
**  = 2 hoch data_size + 2
*/
#define lzw_tiff_neu 258
