/*
**  C   ( Microsoft C V5.10 )
**
**  .12.1994 Ulrich Berntien
**
**  Bearbeiten einer LUT für ein optimales Bild.
**  Die LUT wird dabei graphisch auf dem Monitor dargestellt.
**
**  19.12.1994  Beginn
*/

#include <conio.h>
#include <graph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iris.h"
#include "isdefs.h"

/*
**  i.a. wird die getch() Funktion durch die eigene mygetch() Funktion
**  mit einer #define Anweisung ersetzt. Diese Funktion enthält einen
**  Bildschirmschoner, aber nur für den Text-Modus.
*/
#ifdef getch
#  undef getch
#endif

/*
**  Beschreibung der Tastenfunktionen
*/
static const char textKeys [] =
  "Stützpunkt bewegen           - Cursor Nord, Süd, Ost, West\n"
  "Stützpunkt löschen/einfügen  - Entf, Einfg\n"
  "von Stützpunkt zu Stützpunkt - Tabulator Ost, West\n"
  "von Punkt zu Punkt           - Control (Strg) + Cursor Ost, West";

/*
**  es wird die Standart Output-LUT der Frame-Grabber-Karte benutzt
*/
const int stdOLut = 0;

/*
**  Positionen für die einzelnen Elemente auf dem Grafikbildschirm
*/
enum positionsKonstanten
  {
  _histogrammX  = 300,     /* X-Koordinate Nullpunkt des Histogramms   */
  _histogrammY  = 315,     /* Y-Koordinate Nullpunkt des Histogramms   */
  _histogrammH  = 255,     /* Höhe des Histogramms in Pixel            */
  _nullpunktX   =   5,     /* X-Koordinate Nullpunkt der LUT-Grafik    */
  _nullpunktY   = 315,     /* Y-Koordinate Nullpunkt der LUT-Grafik    */
  _stuetzpunktY = 330,     /* Y-Koordinate Marker für die Stützstellen */
  _aktuellY     = 345,     /* Y-Koordinate Marker für die Position     */
  _markerLen    =  10      /* Länge eines Markers in Y-Richtung        */
  };

/*
**  die LUT und ihre erzeugenden Stützstellen
*/
typedef struct lutdataTAG
  {
  int wert [256];           /* die Werte der LUT 0..255    */
  int stuetzstelle [256];   /* ist dies eine Stützstelle ? */
  }
  lutdata;

/*
**  das Histogramm des Bilds
*/
typedef struct histogrammTAG
  {
  long verteilung [256];     /* Anzahl Pixel mit Grauwert i       */
  long maxCount;             /* größtes Vorkommen eines Grauwerts */
  uchar max;                 /* größter Grauwert im Bild          */
  uchar min;                 /* kleinster Grauwert im Bild        */
  }
  histogramm;

/*
**  die Sicherung der vom Graukeil überschriebenen Bildzeilen
*/
typedef struct keildataTAG
  {
  int startzeile;              /* in dieser Zeile beginnt der Keil */
  char buffer [ bpz * 10 ];    /* die 10 überschriebenen Zeilen    */
  }
  keildata;

/*
**  der Inhalt einer Output-LUT der Frame-Grabber-Karte
*/
typedef struct olutdataTAG
  {
  int nummer;                /* Nummer der Output-LUT */
  int red [256];             /* LUT für Farbe Rot     */
  int green [256];           /* LUT für Farbe Grün    */
  int blue [256];            /* LUT für Farbe Blau    */
  }
  olutdata;

/*
**  Alle Grafikfunktionen sind hier gekapselt.
**  Implementiert werden alle Funktionen mit der Grafik-Bibliothek
**  des Microsoft C-Compilers (V5.10)
**
\********************************************************************/

/*
**  Den Monitor in den Grafikmodus schalten und zurück schalten
**  on     : Grafikmodus einschalten ?
**  return : erfolgreich umgeschaltet ?
*/
static int LOCAL grafikSwitch ( int on )
  {
  int ok;
  if( on )
    {
    ok = _setvideomode( _VRES2COLOR ) != 0;
    if( ! ok )
      {
      puts( "Der Grafikmodus 640 x 480 Punkt schwarz/weis\n"
            "konnte nicht eingeschaltet werden.\n"
            "Daher ist diese Funktion nicht verfügbar." );
      getreturn();
      }
    else
      _setcolor(1);
    }
  else
    ok = _setvideomode( _DEFAULTMODE ) == 0;
  return ok;
  }

/*
**  Umschalten der Stiftfarbe : schreiben / löschen
**  Eine Funktion, die den Clear-Mode einschaltet, muß ihn auch wieder
**  ausschalten.
**  on : soll gelöscht werden ?
*/
static void LOCAL grafikClearMode ( int on )
  {
  _setcolor( on == 0 );
  }

/*
**  eine vertikale Linie von Unten nach Oben zeichenen
**  startX : X-Koordinate des Startpunkts
**  startY : Y-Koordinate des Startpunkts
**  len    : Länge der Linie in Pixel
*/
static void LOCAL grafikVertikale ( int startX, int startY, int len )
  {
  _moveto( startX, startY );
  _lineto( startX, startY - len );
  }

/*
**  eine horizontale Linie von Links nach Rechts zeichnen
**  startX : X-Koordinate des Startpunkts
**  startY : Y-Koordinate des Startpunkts
**  len    : Länge der Linie in Pixel
*/
static void LOCAL grafikHorizontale ( int startX, int startY, int len )
  {
  _moveto( startX, startY );
  _lineto( startX + len, startY );
  }

/*
**  eine Linie zeichnen
**  startX : X-Koordinate des Startpunkts
**  startY : Y-Koordinate des Startpunkts
**  stopX  : X-Koordinate des Endpunkts
**  stopY  : Y-Koordinate des Endpunkts
*/
static void LOCAL grafikLine ( int startX, int startY, int stopX, int stopY )
  {
  _moveto( startX, startY );
  _lineto( stopX, stopY );
  }

/*
**  Ausgabe von Text in den Grafikbildschirm
**  x    : Spalte des ersten Zeichens
**  y    : Zeile des ersten Zeichens
**  text : der auszugebende Text
*/
static void LOCAL grafikText ( int x, int y, const char* text )
  {
  _settextposition( y, x );
  _outtext( (char _far*) text );
  }

/********************************************************************/

/*
**  Histogramm auf dem Grafik-Bildschirm darstellen
**  data : die Daten des zu Zeichnenden Histogramms
*/
static void LOCAL histogrammDraw ( const histogramm* data )
  {
  int i;
  grafikHorizontale( _histogrammX, _histogrammY, 256 );
  for( i = 0; i < 256; ++i )
    grafikVertikale( _histogrammX+i, _histogrammY,
           (int) ( ( data->verteilung[i] * _histogrammH )/ data->maxCount ) );
  }

/*
**  Histogramm zu einem Bild erstellen
**  frb    : Nummer des Frame Buffers mit dem Bild
**  return : Zeiger auf die Daten des Histogramms (free nicht vergessen)
**           oder NULL, wenn kein Speicher dafür mehr frei
*/
static histogramm* LOCAL histogrammMake ( int frb )
  {
  histogramm* data = malloc( sizeof (histogramm) );
  if( data != NULL )
    {
    uchar buffer [bpz];
    int i;
    memset( data, 0, sizeof (histogramm) );
    for( i = 0; i < zpb; ++i )
      {
      int j;
      isb_get_pixel( frb, i, wpz, (char*) buffer );
      for( j = 0; j < bpz; ++j ) ++data->verteilung[ buffer[j] ];
      }
    for( i = 0; i < 256; ++i )
      if( data->maxCount < data->verteilung[i] )
        data->maxCount = data->verteilung[i];
    i = 0;
    while( i < 255 && data->verteilung[i] == 0L ) ++i;
    data->min = (uchar) i;
    i = 255;
    while( i > 0 && data->verteilung[i] == 0L ) --i;
    data->max = (uchar) i;
    }
  return data;
  }

/********************************************************************/

/*
**  Zeichnet das Stück zwischen zwei Stützstellen in die LUT-Grafik
**  lut : die vollständigen Daten der LUT
**  von : Position der ersten Stützstelle
**  bis : Position der zweiten StÞtzstelle
*/
static void lutDrawTeil ( const lutdata* lut, int von, int bis )
  {
  grafikLine( _nullpunktX + von, _nullpunktY - lut->wert[von],
              _nullpunktX + bis, _nullpunktY - lut->wert[bis] );
  }

/*
**  Marker der aktuellen Position verschieben
**  alt : alte Position des Markers
**  neu : neue Position des Markers
*/
static void LOCAL lutDrawAktuell ( int alt, int neu )
  {
  grafikClearMode( 1 );
  grafikVertikale( _nullpunktX + alt, _aktuellY, _markerLen );
  grafikClearMode( 0 );
  grafikVertikale( _nullpunktX + neu, _aktuellY, _markerLen );
  }

/*
**  Zeichnen bzw. Löschen eines Markers von einer Stützszelle
**  position : Position der Stützstelle (0..255)
*/
static void LOCAL lutDrawStuetzstelle ( int position )
  {
  grafikVertikale( _nullpunktX + position, _stuetzpunktY, _markerLen );
  }

/*
**  LUT auf dem Grafik-Bildschirm darstellen
**  lut : die vollständige LUT
*/
static void LOCAL lutDraw ( const lutdata* lut )
  {
  int i;
  grafikVertikale( _nullpunktX-1, _nullpunktY+1, 256 );
  grafikHorizontale( _nullpunktX-1, _nullpunktY+1, 256 );
  i = 0;
  lutDrawStuetzstelle( 0 );
  while( i < 255 )
    {
    const int start = i;
    while( i < 255 && lut->stuetzstelle[i] == 0 ) ++i;
    lutDrawStuetzstelle( i );
    lutDrawTeil( lut, start, i );
    ++i;
    }
  lutDrawAktuell( 0, 0 );
  }

/*
**  Schreiben der LUT in die Frame-Grabber-Karte
**  lut : die vollständigen Daten der LUT
*/
static void LOCAL lutSchreibe ( lutdata* lut )
  {
  is_load_olut( stdOLut, lut->wert, lut->wert, lut->wert );
  is_select_olut( stdOLut );
  }

/*
**  Suchen nach der vorherigen Stützstelle
**  lut      : die vollständige LUT
**  position : vor dieser Position suchen
**  return   : Position der ersten Stützstelle vor der angegebenen
*/
static int LOCAL lutPreStuetzstelle ( const lutdata* lut, int position )
  {
  while( position > 0 && lut->stuetzstelle[--position] == 0 ) {}
  return position;
  }

/*
**  Suchen nach der nächsten Stützstelle
**  lut      : die vollständigen Daten der LUT
**  position : hinter dieser Position wird gesucht
**  return   : die erste Stützstelle nach der angegebenen
*/
static int LOCAL lutPostStuetzstelle ( const lutdata* lut, int position )
  {
  while( position < 255 && lut->stuetzstelle[++position] == 0 ) {}
  return position;
  }

/*
**  Berechnen der Werte der LUT zwischen zwei Stützstellen
**  lut : die Daten der LUT
**  von : die erste Stützstelle
**  bis : die zweite StÞtzstelle
*/
static void LOCAL lutBerechneTeil ( lutdata* lut, int von, int bis )
  {
  int i;
  const int steigung = lut->wert[bis] - lut->wert[von];
  const int delta = bis - von;
  if( delta == 0 )
    {
    }
  else if( steigung > 0 )
    {
    int akku = lut->wert[von];
    int akkurest = 0;
    div_t st;
    st = div( steigung, delta );
    for( i = 0; i < delta; ++i )
      {
      lut->wert[von+i] = akku;
      akku += st.quot;
      if( ( akkurest += st.rem ) > delta )
        {
        akkurest -= delta;
        ++akku;
        }
      }
    }
  else
    {
    int akku = lut->wert[von];
    int akkurest = 0;
    div_t st;
    st = div( -steigung, delta );
    for( i = 0; i < delta; ++i )
      {
      lut->wert[von+i] = akku;
      akku -= st.quot;
      if( ( akkurest += st.rem ) > delta )
        {
        akkurest -= delta;
        --akku;
        }
      }
    }
  }

/*
**  Berechnen alle Werte der LUT aus den Stützstellen
**  lut : die Daten der LUT, die Wert an nicht Stützstellen werden berechnet
*/
static void LOCAL lutBerechne ( lutdata* lut )
  {
  int von = 0;
  while( von < 255 )
    {
    const int nach = lutPostStuetzstelle( lut, von );
    lutBerechneTeil( lut, von, nach );
    von = nach;
    }
  }

/*
**  Einführen einer neuen Stützstelle : alle Berechnungen und Zeichnungen
**  lut      : die Daten der LUT werden aktualisiert
**  position : dort die neue Stützstelle
*/
static void LOCAL lutNeueStuetzstelle ( lutdata* lut, int position )
  {
  if( position > 0 && position < 255 && lut->stuetzstelle[position] == 0 )
    {
    const int pre = lutPreStuetzstelle( lut, position );
    const int post = lutPostStuetzstelle( lut, position );
    grafikClearMode( 1 );
    lutDrawTeil( lut, pre, post );
    grafikClearMode( 0 );
    lut->stuetzstelle[position] = 1;
    lutBerechneTeil( lut, pre, position );
    lutBerechneTeil( lut, position, post );
    lutDrawStuetzstelle( position );
    lutDrawTeil( lut, pre, position );
    lutDrawTeil( lut, position, post );
    lutSchreibe( lut );
    }
  }

/*
**  Löschen einer Stützstelle : alle Berechnungen und Zeichnungen
**  lut      : die Daten der LUT werden aktualisiert
**  position : die Position der zu löschenden Stützstelle
*/
static void LOCAL lutLoescheStuetzstelle ( lutdata* lut, int position )
  {
  if( position > 0 && position < 255 && lut->stuetzstelle[position] )
    {
    const int pre = lutPreStuetzstelle( lut, position );
    const int post = lutPostStuetzstelle( lut, position );
    grafikClearMode( 1 );
    lutDrawStuetzstelle( position );
    lutDrawTeil( lut, pre, position );
    lutDrawTeil( lut, position, post );
    grafikClearMode( 0 );
    lut->stuetzstelle[position] = 0;
    lutBerechneTeil( lut, pre, post );
    lutDrawTeil( lut, pre, post );
    lutSchreibe( lut );
    }
  }

/*
**  Verändert den Wert einer Stützstelle
**  lut      : die Daten der LUT werden aktualisiert
**  position : diese Stützstelle ändern
**  delta    : um dieses soll der Wert geändert werden
*/
static void LOCAL lutAenderStuetzstelle ( lutdata* lut, int position, int delta )
  {
  int neu;
  int ok = position >=0 && position < 256 && lut->stuetzstelle[position];
  if( ok )
    {
    neu = lut->wert[position] + delta;
    ok = neu >= 0 && neu < 256;
    }
  if( ok )
    {
    const int pre = lutPreStuetzstelle( lut, position );
    const int post = lutPostStuetzstelle( lut, position );
    grafikClearMode( 1 );
    lutDrawTeil( lut, pre, position );
    lutDrawTeil( lut, position, post );
    grafikClearMode( 0 );
    lut->wert[position] = neu;
    lutBerechneTeil( lut, pre, position );
    lutBerechneTeil( lut, position, post );
    lutDrawTeil( lut, pre, position );
    lutDrawTeil( lut, position, post );
    lutSchreibe( lut );
    }
  }

/*
**  Verändert Position der Stützstelle und der aktuellen Position
**  lut    : die Daten der LUT werden aktualisiert
**  alt    : die alte Position der Stützstelle
**  neu    : die gewünschte Position
**  return : die neue Position, nach Kontrolle der gewünschten Position
*/
static int LOCAL lutSchiebeStuetzstelle ( lutdata* lut, int alt, int neu )
  {
  int ok =    alt > 0 && alt < 255 && lut->stuetzstelle[alt]
           && neu > 0 && neu < 255 && alt != neu;
  if( alt < neu )
    {
    int i;
    for( i = alt+1; i <= neu && ok; ++i ) ok = lut->stuetzstelle[i] == 0;
    }
  else
    {
    int i;
    for( i = alt-1; i >= neu && ok; --i ) ok = lut->stuetzstelle[i] == 0;
    }
  if( ok )
    {
    const int pre = lutPreStuetzstelle( lut, alt );
    const int post = lutPostStuetzstelle( lut, alt );
    grafikClearMode( 1 );
    lutDrawStuetzstelle( alt );
    lutDrawTeil( lut, pre, alt );
    lutDrawTeil( lut, alt, post );
    grafikClearMode( 0 );
    lut->stuetzstelle[alt] = 0;
    lut->stuetzstelle[neu] = 1;
    lut->wert[neu] = lut->wert[alt];
    lutBerechneTeil( lut, pre, neu );
    lutBerechneTeil( lut, neu, post );
    lutDrawStuetzstelle( neu );
    lutDrawTeil( lut, pre, neu );
    lutDrawTeil( lut, neu, post );
    lutDrawAktuell( alt, neu );
    lutSchreibe( lut );
    }
  return ok ? neu : alt;
  }

/*
**  Verändern der aktuellen Position
**  alt    : die alte Position
**  neu    : die gewünschte Position
**  return : die neue Position, nach Kontrolle der gewünschten Position
*/
static int LOCAL lutSchiebe ( int alt, int neu )
  {
  if( alt != neu && neu >= 0&& neu < 256 )
    lutDrawAktuell( alt, neu );
  else
    neu = alt;
  return neu;
  }

/*
**  Erstellen der LUT-Daten für den Start
**  hist   : die Daten des Histogramms, oder NULL
**  return : die Daten der LUT (free nicht vergessen)
**           oder NULL im Fehlerfall
*/
static lutdata* LOCAL lutMake ( const histogramm* hist )
  {
  lutdata* lut = malloc( sizeof (lutdata) );
  if( lut != NULL )
    {
    memset( lut, 0, sizeof (lutdata) );
    lut->stuetzstelle[0] = 1;
    lut->wert[0] = 0;
    lut->stuetzstelle[255] = 1;
    lut->wert[255] = 255;
    if( hist != NULL )
      {
      lut->stuetzstelle[hist->min] = 1;
      lut->wert[hist->min] = 0;
      lut->stuetzstelle[hist->max] = 1;
      lut->wert[hist->max] = 255;
      }
    lutBerechne( lut );
    }
  else
    errmalloc();
  return lut;
  }

/********************************************************************/

/*
**  Einblenden eines Graukeils und sichern des Bildbereiches
**  frb   : Nummer des Frame Buffers
**  zeile : erste Zeile des Graukeils
**  data  : dort den überschriebenen Bildbereich abspeichern
*/
static void LOCAL keilEinblenden ( int frb, int zeile, keildata* data )
  {
  char keil [bpz];
  int i;
  const int zeilen = sizeof(data->buffer) / bpz;
  data->startzeile = zeile;
  for( i = 0; i< bpz; ++i ) keil[i] = (char) ( i / (bpz/256) );
  isb_get_pixel( frb, zeile, zeilen * wpz, data->buffer );
  for( i = 0; i < zeilen; ++i )
    isb_put_pixel( frb, zeile+i, wpz, keil );
  }

/*
**  auf Wunsch wird ein Graukeil in das Bild eingeblendet
**  frb    : Nummer des Frame Buffers
**  return : die gesicherten Daten aus dem Bild, oder NULL
*/
static keildata* LOCAL keilMake ( int frb )
  {
  keildata* data = malloc( sizeof (keildata) );
  if( data != NULL )
    {
    char answ;
    puts( "Soll in das Bild ein Graukeil eingeblendet werden ?\n"
          "\t o - in die oberen Zeilen\n"
          "\t u - in die unteren Zeilen\n"
          "\t - - nicht einblenden" );
    answ = getcset( "ou-" );
    switch( answ )
      {
      case 'o' : keilEinblenden( frb, 0, data );
                 break;
      case 'u' : keilEinblenden( frb, zpb-10, data );
                 break;
      case '-' : free( data );
                 data = NULL;
                 break;
      }
    }
  return data;
  }

/*
**  den Graukeil auf Wunsch aus dem Bild entfernen
**  frb  :
**  data : die Daten des Graukeils, werden hier freigegeben
*/
static void LOCAL keilRecall ( int frb, keildata* data )
  {
  if( data != NULL )
    {
    fputs( "Soll der Graukeil aus dem Bild entfernt werden", stdout );
    if( janein() )
      isb_put_pixel( frb, data->startzeile,
                     sizeof (data->buffer)/2, data->buffer );
    free( data );
    }
  }

/********************************************************************/

/*
**  Sichern einer vollständigen Output-LUT der Frame-Grabber-Karte
**  nummer : die Nummer der Output-LUT
**  return : die gesicherten Daten, oder NULL im Fehlerfall
**           (free() bzw. oloutRecall() nicht vergessen
*/
static olutdata* LOCAL olutStore ( int nummer )
  {
  olutdata* olut = malloc( sizeof(olutdata) );
  if( olut != NULL )
    {
    olut->nummer = nummer;
    is_read_olut( nummer, olut->red, olut->green, olut->blue );
    }
  return olut;
  }

/*
**  Zurücksetzen einer Output-LUT und Freigeben des Speichers
**  olut : die Daten der Output-LUT, werden hier freigeben
*/
static void LOCAL olutRecall ( olutdata* olut )
  {
  if( olut != NULL )
    {
    is_load_olut( olut->nummer, olut->red, olut->green, olut->blue );
    free( olut );
    }
  }

/********************************************************************/

/*
**  Editieren der LUT im Dialog
**  lut : die vollständigen Daten der LUT
*/
static void LOCAL dialog ( lutdata* lut )
  {
  int ende = 0;
  int position = 0;
  do{
    int key = getch();
    if( key == 0 ) key = getch();
    switch( key )
      {
      case 0x09 : position = lutSchiebe( position, lutPostStuetzstelle(lut,position) );
                  break;
      case 0x0f : position = lutSchiebe( position, lutPreStuetzstelle(lut,position) );
                  break;
      case 0x73 : position = lutSchiebe( position, position -1 );
                  break;
      case 0x74 : position = lutSchiebe( position, position +1 );
                  break;
      case 0x1b : ende = 1;
                  break;
      }
    if( lut->stuetzstelle[position] )
      switch( key )
        {
        case 0x48 : lutAenderStuetzstelle( lut, position, +1 );
                    break;
        case 0x50 : lutAenderStuetzstelle( lut, position, -1 );
                    break;
        case 0x53 : lutLoescheStuetzstelle( lut, position );
                    break;
        case 0x4b : position = lutSchiebeStuetzstelle( lut, position, position-1 );
                    break;
        case 0x4d : position = lutSchiebeStuetzstelle( lut, position, position+1 );
                    break;
        }
    else
      switch( key )
        {
        case 0x52 : lutNeueStuetzstelle( lut, position );
                    break;
        }
    }
    while( ! ende );
  }

/*
**  Texte auf den Bildschirm mit LUT-Grafik und Histogramm
*/
static void LOCAL textDraw ( void )
  {
  grafikText( 32,  1, "LUT optimieren" );
  grafikText(  9,  3, "Output - LUT" );
  grafikText( 55,  3, "Histogramm" );
  grafikText( 36, 21, "<- Stützstellen" );
  grafikText( 36, 22, "<- aktuelle Position" );
  grafikText(  1, 26, textKeys );
  }

/*
**  Editieren der LUT
**  frb : Nummer des Frame Buffers mit dem Bild
*/
void luteditor ( int frb )
  {
  histogramm* h;
  lutdata* lut;
  olutdata* olut = olutStore( stdOLut );
  keildata* keil;
  puts( "Histogramm über das Bild wird erstellt." );
  h = histogrammMake( frb );
  lut = lutMake( h );
  keil = keilMake( frb );
  if( lut != NULL && grafikSwitch(1) )
    {
    histogrammDraw( h );
    lutDraw( lut );
    lutSchreibe( lut );
    textDraw();
    dialog( lut );
    }
  grafikSwitch(0);
  keilRecall( frb, keil );
  puts( "Die LUT kann in die Grauwerte übertragen werden." );
  permanent( frb, stdOLut );
  olutRecall( olut );
  free( h );
  free( lut );
  }
