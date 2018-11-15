/*
**  Aufbau des BMP-Files
**
**  Literatur:
**  . Th.W.Lipp : Die große Welt der Grafikformate (Teil1)
**    in Microsoft System Journal, 1/1993, Seiten 111 ff.
**  . G.Born : Scharfer Blick hinter die Kulissen, Das Bitmap-Format
**    in MC, 8/1993, Seiten 88 ff.
**
**  Aufbau der Datei:
**    bmp_fileheader
**    bmp_infoheader
**    bmp_rgbquad [ clrUsed ]
**    bmp_bitmap
*/

#pragma ZTC align 1

/*
**  Konstanten
*/
static const WORD  bmp_typ = 0x4D42;    /* Typkennung eines BMP-Files   */
static const DWORD bmp_rgb = 0L;        /* Code: Daten unkomprimiert    */
static const DWORD bmp_rle4 = 1L;       /* Code: Daten RLE4 komprimiert */
static const DWORD bmp_rle8 = 2L;       /* Code: Daten RLE8 komprimiert */

/*
**  Aufbau des Bitmap-Fileheaders
**  Enthält Informationen über die Datei.
*/
typedef struct bmp_fileheaderTAG
  {
  WORD  typ;                /* Typ der Datei (bmp_typ)           */
  DWORD size;               /* Größe der Datei in Bytes          */
  DWORD reserved;           /* reserviert, muß 0 sein            */
  DWORD offsetBitmap;       /* Offset von Dateianfang zur Bitmap */
  }
  bmp_fileheader;

/*
**  Aufbau des Bitmap-Infoheaders
**  Enthält Informationen über die Bitmap und die Farbtabelle.
*/
typedef struct bmp_infoheaderTAG
  {
  DWORD size;             /* Größe der Datenstruktur bmp_infoheader       */
  LONG  width;            /* Breite der Bitmap in Punkte (Pixel)          */
  LONG  height;           /* Höhe der Bitmap in Punkts (Pixel)            */
  WORD  planes;           /* Anzahl der Farbebenen, muß 1 sein            */
  WORD  bitCount;         /* Anzahl der Bits, die für einen Punkt         */
                          /* (Pixel) benötigt werden. Möglich sind:       */
                          /*  1 : Monochrome Bitmap mit genau 2 Farben    */
                          /*  4 : Bitmap mit maximal 16 Farben            */
                          /*  8 : Bitmap mit maximal 256 Farben           */
                          /* 24 : Bitmap mit maximal 16,7Mio Farben       */
  DWORD compression;      /* Typ der Datenkomprimierung. Möglich sind     */
                          /*  bmp_rgb  : nicht komprimiert                */
                          /*  bmp_rle4 : komprimiert nur bei bitCOunt = 4 */
                          /*  bmp_rle8 : komprimiert nur bei bitCOunt = 8 */
  DWORD sizeImage;        /* Größe der Bitmap in Byte                     */
  LONG  xPelsPerMeter;    /* horizontale Auflösung der Bitmap             */
  LONG  yPelsPerMeter;    /* vertikale Auflösung der Bitmap               */
  DWORD clrUsed;          /* Anzahl der verwendeten Farben                */
  DWORD clrImportant;     /* Anzahl der wichtigen Farben                  */
  }
  bmp_infoheader;

/*
**  Aufbau eines Elements der Farbtabelle.
**  Die Anzahl der Elemente wird durch das Feld clrUsed im
**  bmp_infoheader angegeben.
*/
typedef struct bmp_rgbquadTAG
  {
  BYTE rgbBlue;             /* Intensität der Grundfarbe Blau */
  BYTE rgbGreen;            /* Intensität der Grundfarbe Grün */
  BYTE rgbRed;              /* Intensität der Grundfarbe Rot  */
  BYTE reserved;            /* reserviert, muß 0 sein         */
  }
  bmp_rgbquad;

/*
**  Aufbau der Bitmap. Je nach Bits pro Pixel und Kompression.
**  1) 8-Bit Format nicht komprimiert :
**     Bild zeilenweise gespeichert. Pro Pixel ein Byte (8 Bit).
**     Jede Zeile auf ein vielfaches von 4 Byte (32 Bit) verlängert mit 0en.
**     Die tiefste Zeile des Bilds wird zu erst gespeichert.
**     Innerhalb einer Zeile die Pixel von Links nach Rechts.
**  2) 8-Bit Format Lauflängen komprimiert (RLE8) :
**     Das Bild wird auch hier zeilenweise gespeichert in der gleichen
**     Reihenfolge der Pixel wie im Fall 1.
**     Die Bildinformation wird in einzelne Datensätze, deren Länge
**     ein Vielfaches von 32Bit (2Bytes) ist, gespeichert.
**     Die möglichen Datensätze sind:
**        Inhalt (Bytes)         |  Bedeutung
**     --------------------------+-------------------------------------------
**      0x00 0x00                | Ende der Bildzeile.
**      0x00 0x01                | Ende der Bitmap.
**      0x00 0x02 x  y           | Verschiebung der aktuelle Postion in der
**                               | Bitmap um x Zeilen nach oben und um y
**                               | Spalten nach rechts.
**      0x00 n    d1 d2 d3 .. dn | Pixel mit den Werten d1,d2,..,dn. Die
**                               | Anzahl n muß > 3 sein. Ist n ungerade, so
**                               | wird ein 0 Byte angehängt.
**      r    d                   | die nächsten r Pixel haben den Wert d.
*/

#pragma ZTC align
