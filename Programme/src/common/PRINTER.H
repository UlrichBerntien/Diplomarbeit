/*
**  Umschalten zwischen verschiedenen Drucker für die Ausgabe von
**  Text und Grafik
*/

/*
**  Unterstützte Druckerfamilien
*/
enum printer_t
  {
  _noPrinter = -1,
  _hpLaserjet,       /* HP-Laserjet und kompatible (HP-Deskjet)  */
  _necPinwriter,     /* NEC Pinwriter P2200 Serie und kompatible */
  _printerCount      /* Anzahl der unterstützten Drucker         */
  };

/*
**  Codes für die Druckeransteuerung
*/
enum printer_c
  {
  _reset = 0,       /* Drucker reset                  */
  _boldOn,          /* Fette Schriftart ein           */
  _boldOff,         /* Fette Schriftart aus           */
  _ibmzeichen,      /* IBM-PC Zeichensatz             */
  _pageFeed,        /* Anfang neue Seite              */
  _lineFeed,        /* Anfang neue Zeile              */
  _graphicOn,       /* Grafikmode an                  */
  _graphicOff,      /* Grafikmode aus                 */
  _codesCount       /* Anzahl der Steuercodes         */
  };

/*
**  Grafikformat des Druckers
*/
enum printer_g
  {
  _bitZeileL,    /* 1 Pixel hoher Streifen, Bit 0x80 im Byte steht links */
  _24bitZeileO,  /* 24 Pixel hoher Streifen, Bit 0x80 im Byte steht oben
                    die drei Bytes folgen: oben, mitte, unten            */ 
  };

/*
**  Setzten des Druckertyps für die weitere Ausgabe
**  prn     : diesen Drucker benutzten
**  return  : die alte Einstellung
*/
enum printer_t printer_set ( const enum printer_t prn );

/*
**  Erfragen des aktuellen Druckertyps
**  return : aktuelle Einstellung
*/
enum printer_t printer_get ( void );

/*
**  Erfragen des Grafikformats des aktuellen Druckers
**  return : Das Grafikformat
*/
enum printer_g printer_graphic ( void );

/*
**  Suchen des Printertys
**  kurz   : Kurzbezeichnung des Typs
**  return : der Printertyp oder _noPrinter
*/
enum printer_t printer_search ( const char* kurz );

/*
**  Ausgeben des Zeichencodes für den Drucker 
**  code  : gewünschter Steuercode
**  ofile : in diese File schreiben
*/
void printer_code ( const enum pinter_c code, FILE *const ofile );

/*
**  Einen Grafikstreifen ausgeben
**  buffer : die Daten
**  len    : Anzahl der Bytes in den Daten
**  ofile  : in dieses File schreiben
**  return : != 0, wenn alles ok
*/
int printer_stripeout ( const char* buffer, const int len, FILE *const ofile );
