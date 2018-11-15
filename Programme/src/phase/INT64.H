/*
**  64 - Bit Integer-Arithmetik
**  und spezielle 32 - Bit Funktionen
*/

#ifndef _far
#  define _far    far
#  define _near   near
#  define _pascal pascal
#  define _cdecl  cdecl
#endif

typedef int int64 [4];

void _far _pascal to64 ( int64 , int );
int  _far _pascal from64 ( int*, const int64 );
void _far _pascal null64 ( int64 );
void _far _pascal let64 ( int64, const int64 );
void _far _pascal double64 ( double*, const int64 );

void _far _pascal inc64 ( int64 , int );
void _far _pascal inc64l ( int64 , long );
void _far _pascal inc64x ( int64, const int64 );
void _far _pascal dec64x ( int64, const int64 );

void _far _pascal neg64 ( int64 );
void _far _pascal sub64 ( int64 , const int64 , const int64 );
int  _far _pascal cmp64( const int64, const int64 );

void _far _pascal mul64 ( int64 , const int64 , int );
void _far _pascal mul64l ( int64 , const int64 , long );
void _far _pascal mul64ll ( int64 , long, long );

int _far _pascal div64 ( int64 , const int64 , const int64 );

long _far _pascal mul32 ( int, int );
long _far _pascal gauss32 ( int, int );
