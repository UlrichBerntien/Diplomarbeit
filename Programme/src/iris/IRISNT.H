/*
**  Typ zum Speichern der Dichteverteilung n(x) und der Temperatur-
**  verteilung t(x) übder den Spalte (x)
*/
typedef struct ntspeicherTAG
  {
  char name [20];         /* Name der Grafik (Schußnummer)           */
  char art;               /* Art der Line a,b,c                      */
  int mittelueber;        /* jedes Spektrum gemittelt über .. Zeilen */
  int von,bis;            /* Spektren von Zeile .. bis Zeile ..      */
  double n [256];         /* Dichten in cm^-3                        */
  double t [256];         /* Temperaturen in eV                      */
  }
  ntspeicher;
