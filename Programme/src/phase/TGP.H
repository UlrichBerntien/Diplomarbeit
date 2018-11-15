/*
**  Beschreibt Fileaufbau für die Plotprogramme
*/


enum tgp { _tgp_zeile, _tgp_graf, _tgp_ff, _tgp_eof };

struct tgp_zeile
  {
  enum tgp typ;            /* typ = _tgp_zeile           */
  char data [70];          /* ASCIZ String max. 70 lang  */
  };

struct tgp_graf
  {
  enum tgp typ;            /* typ = _tgp_graf                          */
  int count;               /* Anzahl der Bytes im Plot max. TGPGRAFLEN */
  unsigned char data [8];  /* zu plottende Daten, Dummywert 8          */
  };

#define TGPGRAFLEN 4000

struct tgp_ff
  {
  enum tgp typ;            /* typ = _tgp_ff, Formfeed ausgeben */
  };
