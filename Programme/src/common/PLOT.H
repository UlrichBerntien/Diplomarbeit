/*
**  Plotten auf den aktuellen Drucker
*/

/*
**  mögliche Plotmodes, die Modes können mit oder verknüpft werden
*/
enum plot_t
  {
  _plotcuttop = 1,        /* obere leere Zeilen abschneiden  */
  _plotcutbottom = 2      /* untere leere Zeilen abschneiden */
  };

/*
**  Setzten des Plotmodes für die folgenden Plots
**  mode   : der neue Plotmode
**  return : der alte Plotmode
*/
enum plot_t plot_setmode ( enum plot_t mode );

/*
**  Einstellen der Punkte pro Division
**  d.h. die Teilstriche an den Achsen werden alle h (bzw. v) Punkte gesetzt
**  h : Punkte pro Division auf der horizontalen Achse
**  v : Punkte pro Division auf der vertikalen Achse
*/
void plot_div ( int h, int v );

/*
**  Initialisieren des Plot Moduls
**  falls ein Fehler auftritt wird das Programm beendet
*/
void plot_init ( void );

/*
**  Ausgabe eines Plots in das Ausgabefile
**  ofile  : Ausgabe in dieses File, write binary
**  werte  : zu plottende Werte
**  len    : Anzahl der Werte
**  zdots  : Anzahl der Zeilen zwischen zwei Werten
**  return : 0, wenn die Ausagbe gelungen ist
*/
int plot ( FILE* ofile, const unsigned char* werte, int len, int zdots );

