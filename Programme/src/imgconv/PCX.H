/*
**  Aufbau des PCX - Files
**
**  Liteartur:
**  . Th.W.Lipp : Die große Welt der Grafikformate (Teil2)
**    in Microsoft System Journal, 2/1993, Seiten 139 ff.
**  . D.Bückart : Grafik mit Format, Aufbau von PCX-Dateien
**    in MC 12/1990, Seiten 104 ff.
**
**  Aufbau der Datei:
**    pcx_header
**    pcx_bitmap
**    optional: pcx_rgbtabelle
*/

#pragma ztc align 1

/*
**  Konstanten
*/
#define pcx_typ     0x0A     /* Kennung für ein PCX-File          */
#define scr_typ     0xCD     /* Kennung für ein SCR-File          */
#define pcx_ver25   0x00     /* Code für PCX Version 2.5          */
#define pcx_ver28m  0x02     /*  für Version 2.8 mit Farbtabelle  */
#define pcx_ver28o  0x03     /*  für Version 2.8 ohne Farbtabelle */
#define pcx_ver3    0x05     /*  für Version 3.0 mit Farbtabelle  */
#define pcx_farb    0x0C     /* Kennung der Farbtabelle           */

/*
**  Aufbau des Fileheaders
**  Enthält Informationen über das Bild
*/
typedef struct pcx_headerTAG
  {
  BYTE typ;                /* Typ der Datei (pcx_typ)               */
  BYTE ver;                /* PCX-Version (pcx_ver)                 */
  BYTE compressed;         /* Datenkomprimiertung:   0 = unkodiert  */
                           /*                     1 = RLE-Kodierung */
  BYTE bitPerPixel;        /* # Bits pro Pixel pro Farbebene        */
  WORD xmin;               /* x-Koordinate der linken oberen Ecke   */
  WORD ymin;               /* y-Koordinate der linken oberen Ecke   */
  WORD xmax;               /* x-Koordinate der rechten unteren Ecke */
  WORD ymax;               /* y-Koordinate der rechten unteren Ecke */
  WORD dpiHor;             /* Horizontale Auflösung in Dots/Inch    */
  WORD dpiVer;             /* Vertikale Auflösung in Dots/Inch      */
  BYTE tabelle[48];        /* Farbtabelle oder unbenutzt            */
  BYTE reserved1;          /* muß auf 0 gesetzt sein                */
  BYTE planes;             /* Anzahl der Farbeneben (maximal 4)     */
  WORD bytesPerLine;       /* # Bytes pro Bildzeile pro Farbenebe   */
  WORD colorForm;          /* Interpretation der Farbtabelle :      */
                           /* 1 : Farbe oder S/W, 2 : Graustufen    */
  BYTE reserved2[58];      /* Leerbytes zum Auffüllen des Headers   */
  }
  pcx_header;

/*
**  Aufbau der Farbtabelle
*/
typedef struct pcx_rgbtabelleTAG
  {
  BYTE kennung;             /* Kennung der Farbtabelle (pcx_farb) */
  BYTE rgb [0x300];         /* RGB-Werte Rot/Grün/Blau            */
  }
  pcx_rgbtabelle;

/*
**  Aufbau der Bitmap bei 8 Bit pro Pixel und einer Farbebene
**  1) nicht komprimiert :
**     Das Bild wird zeilenweise von Oben nach Unten gespeichert. Die Pixel
**     in den Zeilen werden von Links nach Rechts gespeichert. Pro Pixel
**     werden 8 Bit = 1 Byte gespeichert.
**  2) Lauflängen komprimiert (RLE8) :
**     Das Bild wird wieder von Oben nach Unten und von Links nach Rechts
**     gespeichert.
**     Innerhalb einer Zeile werden die Bytes komprimiert gespeichert. Dabei
**     gibt es zwei verschiedene Formen, die durch die höchstwertigsten
**     beiden Bits unterschieden werden:
**       Inhalt (1 oder 2 Byte) |     Bedeutung
**      ------------------------+------------------------------------------
**       00xxxxxx               | Das Byte stellt ein Pixel mit dem Wert
**                              | 00xxxxxx dar.
**       11rrrrrr xxxxxxxx      | Die beiden Bytes stellen 00rrrrrr Pixels
**                              | mit dem Wert xxxxxxxx dar.
**     Pixel mit einem Wert von > 00111111 (= 0x3F) müssen in der zweiten
**     Form gespeichert werden. Wiederholungen von mehr als 00111111 (0x3F)
**     müssen in mehrere Codes aufgespalten werden.
*/

#pragma ZTC align
