/* stredit.c */
void stredit ( char* buffer, int size );
void streditxy ( char* buffer, int size, int x, int y );
void cls ( void );
void cll ( int );
void putsxy ( int x, int y, const char* str );
void setcrrspos ( int x, int y );
void screensaver ( void );

/* dirget.c */
const char* dirget_first ( const char* name );
const char* dirget_next ( void );
const char* dirget_name ( void );
