#define eps 1e-30

extern int initok;             /* != 0 wenn Modul nicht initialisiert */

extern int wwtabellieren;   /* sollen wahre Werte tabelliert werden ? */

extern char* logfile;                 /* Name des Files fürs Protokol */
extern char* plotfile;            /* Name des Files für die n,T Plots */

extern double nm_pixel;      /* Wellenlängendiff. zwischen zwei Pixel */
extern double fixtemperatur;           /* s.o. Wert aus Parameterfile */
extern char* halb_methode;    /* Suchmethode nach der Halbwertsbreite */
extern char* limit_methode;    /* Methode Festlegen der Liniengrenzen */

extern char* printerfett;           /* Steuercodes für Druckerschrift */
extern char* printernormal;

/*
**  Tabelle mit den Temperaturen bei bestimmten Linienintensität zu
**  Kontinuum in 10 nm Intervall Verhältnissen
*/
#define tratiolen 15

typedef struct TRATIO
  {
  int count;                                   /* Anzahl der Einträge */
  double t [tratiolen];                       /* Temperatur in Kelvin */
  double alpha [tratiolen];         /* log. Verhätnis bei alpha Linie */
  double beta [tratiolen];          /* log. Verhältnis bei beta Linie */
  double gamma [tratiolen];        /* log. Verhältnis bei gamma Linie */
  }
  tratio;

extern const tratio* ptratio;

/*
**  Tabelle mit Koeffizineten für die Berechnung der Elektronendichte
**  aus der Starkverbreiterung
*/
#define stlen 5

typedef struct STARKDATEN
  {
  int cdichten;                        /* # der verschiedenen Dichten */
  double dichten [stlen];               /* die Dichten = Spaltenindex */
  int ctemperaturen;                     /* # der verschiedenen Temp. */
  double temperaturen [stlen];        /* die Temperaturen = Zeilindex */
  double c [stlen*stlen];                        /* die Koeffizienten */
  }
  starkdaten;

extern const starkdaten* starktabellen;
          /* Zeiger auf Array mit 3 starkdaten für alpha, beta, gamma */

/*
**  Block in dem alle Daten über das Spektrum gesammelt werden
*/
typedef struct infoblockTAG
  {
  const char* name;                             /* Name des Spektrums */
  const spektrum* sp;                      /* das zugehörige Spektrum */
  int kontinuum;                      /* Kontinuum in Pixel-Einheiten */
  double wkontinuum;                    /* wahrer Wert des Kontinuums */
  double kontinuumi;                              /* Kontinuum * 10nm */
  int linie;                             /* vorgegeben Linienposition */
  int linievon, liniebis;          /* von..bis wurde Linie integriert */
  double liniealles;                  /* Alles von..bis aufintegriert */
  double linieintegral;    /* das Integral über die Linie ohne Konti. */
  double verhaeltnis;        /* Verhältnis Linie/Kontinuumsintensität */
  int spitze;                          /* Pixelwert Maximum der Linie */
  double wspitze;                    /* wahrer Wert Maximum der Linie */
  double whalbespitze;                /* Hälfte des wahren Wertes -"- */
  int halbespitze;                    /* Pixelwert der Hälfte des -"- */
  int halbvon, halbbis;                  /* Splten der Halbertsbreite */
  double fwhm;                                          /* FWHM in nm */
  double fwhmA;                                          /* FWHM in A */
  char art;                             /* Art der Linie a,b,c oder k */
  double temperaturk;                         /* Temparatur in Kelvin */
  double temperaturev;                            /* Temperatur in eV */
  double cnt;                                 /* Koeffizient C(Ne,Te) */
  double ne;                                       /* Dichte in cm^-3 */
  }
  infoblock;

/* iris04.c */
void erfragegrenzen ( infoblock* info );

/* iris04a.c */
void doinit ( void );

/* iris04b.c */
int init_pixel2wahr ( void );
void spektrum2wahrwerte ( int wert [bpz] );
void liniesummieren( infoblock* info );
void halbwertsbreite ( infoblock* info );
void dichte ( infoblock* info );
void temperatur ( infoblock* info );
