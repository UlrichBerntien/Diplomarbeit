#ifdef __ZTC__
#  define volatile
#endif

enum rs232enum { _rs232none, _rs232even, _rs232odd };

typedef struct TAGrs232parameter
  {
  int com;                 /* 1 bis 4    */
  int baud;                /* 50 .. 9600 */
  int databits;            /* 5 .. 8     */
  int stopbits;            /* 1 oder 2   */
  enum rs232enum parity;
  }
  rs232parameter;

typedef struct TAGrs232buffer
  {
  char* start;            /* Anfang des Buffers                   */
  char* now;              /* dort nächstes Zeichen speichern      */
  int lfs;                /* Anzahl empfangene lf                 */
  int parityerrors;       /* Anzahl aufgetretener Parity-Errors   */
  int overflowerrors;     /* Anzahl aufgetretener Overflow-Errors */
  }
  rs232buffer;

int rs232setparameter ( const rs232parameter* );
const char *volatile * rs232send ( const char* );
volatile rs232buffer* rs232setbuffer ( char* );
void rs232clear ( void );
