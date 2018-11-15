        PAGE 60,130
       .SALL
;---------------------------------------------------------------------
;  MS-Macro-Assembler V4.00
;  .08.1994 Ulrich Berntien + Jens Raacke
;
;  Hilfsfunktionen zur Beschleunigung des IMG-Viewers
;
;  für die Speichermodelle Compact (I8086C) und Large (I8086L)
;
;  20.08.1994  Beginn
;  06.09.1994  setVideoMode, getVideoMode, multiCopy aufgenommen
;---------------------------------------------------------------------

PUBLIC _getVideoMode, _setVideoMode, _zeilenMittel, _egaOut
PUBLIC _multiCopy

;---------------------------------------------------------------------

ifdef I8086L

  function MACRO fktname
    fktname PROC FAR
    argptr = 6
    lokalptr = 0
           ENDM

else
ifdef I8086C

  function MACRO fktname
    fktname PROC NEAR
    argptr = 4
    lokalptr = 0
           ENDM

else

  %OUT  Es werden nur die Speichermodelle Compact und Large
  %OUT  unterstützt.

endif
endif

;
;  Macro definiert eine Stringvariable zum Zugriff auf die übergebene Werte
;  ss und bp müssen wie üblich ausgerichtet sein.
;  argname : Name des Arguments
;  argtyp  : Typ des Arguments
;  argsize : Platz auf dem Stack in Bytes
;
argument MACRO argname, argtyp, argsize
  offsetOf&argname = argptr
  argname EQU argtyp PTR [bp+offsetOF&argname]
  argptr = argptr + argsize
         ENDM

;
;  Definition einer lokalen Variablen
;  ss und bp müssen wie üblich ausgerichtet sein
;  lokalname : Name der Variable
;  lokaltyp  : Typ der Variable
;  lokalsize : Platz auf dem Stack in Bytes
;
lokal MACRO lokalname, lokaltyp, lokalsize
  lokalptr = lokalptr + lokalsize
  offsetOF&lokalname = lokalptr
  lokalname EQU lokaltyp PTR [bp-offsetOF&lokalname]
      ENDM

;
;  Speicher für lokalen Speicher auf dem Stack belegen
;  ss und bp müssen wie üblich ausgerichtet sein
;
lokalAlloc MACRO
  sub sp,lokalptr
           ENDM

lokalFree MACRO
  mov sp,bp
          ENDM

;
;  Sichern von Registern auf dem Stack
;
pushing MACRO regs
  IRP reg, <regs>
    push reg
  ENDM
        ENDM

;
;  Laden von Registern vom Stack
;
poping MACRO regs
  IRP reg, <regs>
    pop reg
  ENDM
       ENDM

;---------------------------------------------------------------------

_TEXT segment PUBLIC WORD 'CODE'
   ASSUME es : nothing, ds : nothing, ss : nothing, cs : _TEXT

;
;  Höhe eines IMG-(Halb)Bils in Zeilen
;
hoehe EQU 256

;
;  Breite einer Bildzeile in Pixel (1Pixel = 1Byte = 8Bit)
;
breite EQU 512

;
;  Breite einer Zeile in Pixel der EGA/VGA-Karte
;
videoBreite EQU 640

;
;  Segment des Video Speichers der EGA/VGA-Karte
;
vsegment EQU 0A000h

;
;  Portadresse des Graphics-Controllers
;
controller EQU 03CEh

;
;  auf den Port des Graphics-Controllers wird immer ein WORD ausgegeben,
;  dabei indiziert das Lowbyte ein Register auf der Videokarte und das
;  Highbyte wird in dieses Register geschrieben
;
functionselect EQU 3
graphicsmode   EQU 5
bitmaskreg     EQU 8

;
;  Erfragen des Video-Modes über das Video-Bios
;  return : der Videomode  
;
function _getVideoMode
        mov	ax,0f00H
        int	10H
        ret
_getVideomode ENDP

;
;  Setzen des Video-Modes über das Video-Bios
;  mode : der gewünschte Videomode
;
function _setVideoMode
        push bp
        mov  bp,sp
  argument mode, BYTE, 2
        mov  al,mode
        xor	 ah,ah
        int  10H
        pop  bp  
        ret 
_setVideoMode ENDP

;
;  Mitteln zweier Bildzeilen
;  zwei : Buffer mit zwei Bildzeilen (aufeinander folgend)
;  m    : Buffer für eine Zeile, das Mittel der beiden Zeilen
;
;  void _cdecl zeilenMittel ( const byte zwei [breite*2], byte m[breite] );
;
function _zeilenMittel
        push  bp
        mov   bp,sp
  argument zwei, DWORD, 4
  argument m,    DWORD, 4
        pushing <ds,di,si>
        lds   si,zwei
        les   di,m
        mov   cx,breite
l001:   mov   dl,[si+breite]                  ;Byte aus zweiter Zeile laden
        lodsb                                 ;Byte aus erster Zeile laden
        add   al,dl
        rcr   al,1                            ;Mitteln der Grauwerte
        stosb                                 ;speichern des Mittelwerts
        loop  l001
        poping <si,di,ds,bp>
        ret
_zeilenMittel ENDP

;
;  Ausgeben des Bilds auf einer EGA/VGA-Karte
;  bild        : Zeiger auf das erste Byte im IMG-(Halb)Bild
;  farbtabelle : Zeiger auf das erste Byte in einer Tabelle
;                8Bit Grauwert -> 4Bit Farb/Grauwert
;                Das Array muß in dem Stacksegment liegen
;
;  void _cdecl eagOut ( const byte* bild, const byte farbtabelle );
;
function _egaOut
        push  bp
        mov   bp,sp
  argument bild,            DWORD, 4
  argument farbtabelle_off, WORD,  2
  argument farbtabelle_seg, WORD,  2
  lokal bit,      WORD, 2
  lokal bitmaske, WORD, 2
  lokalAlloc
        pushing <ds,si,di>
        mov   ax,ss
        cmp   ax,farbtabelle_seg
        jne   l105                            ;Farbtabelle muß in ss liegen
        mov   dx,controller
        mov   ax,graphicsmode + 0200h
        out   dx,ax                           ;read modus 0, write modus 2
        mov   ax,functionselect + 0000h
        out   dx,ax                           ;replace mode

        mov   ax,vsegment                     ;mit ds:di durch den Videospeicher
        mov   ds,ax
        mov   bit,0                           ;mit dem 0 Bit anfangen
        mov   ax,8000h                        ;das 0.Bit maskieren (im HighByte)
;
; Anfang, Schleife über alle Bits
;
l104:   mov   bitmaske,ax
        mov   dx,controller
        add   ax,bitmaskreg
        out   dx,ax                           ;Bitmaske zum Video-Controller
        les   si,bild                         ;mit es:si durch das IMG-Bild
        add   si,bit
        xor   di,di                           ;mit ds:di durch den Videospeicher

        mov   bx,farbtabelle_off
        mov   dx,hoehe                        ;mit dx die Zeilen zählen
;
; Anfang, Schleife über alle Zeilen
;
l103:   mov   cx,breite/8                     ;mit cx die Spalten zählen
;
; Anfang, Schleife über jede 8.Spalte
;
l101:   mov   al,es:[si]
        xlat  ss:[bx]                         ;durch die Farbtabelle
        xchg  al,ds:[di]
        inc   di                              ;8 Pixel weiter im Video
        add   si,8                            ;8 Byte weiter im IMG
        jc    l102
        loop  l101                            ;nächste Spalte
        jmp   l106                            ;Zeile ist fertig
l102:   mov   ax,es                           ;Bild > 64KB, daher Übertrag
        add   ax,1000h
        mov   es,ax
        loop  l101                            ;nächste Spalte
;
; Ende, Schleife über jede 8.Spalte
;
l106:   add   di,(videobreite-breite)/8       ;nächste Spalte im Videospeicher
        dec   dx
        jnz   l103                            ;nächste Zeile
;
; Ende, Schleife über jede Zeile
;
        mov   ax,bitmaske
        shr   ax,1                            ;Maske für das nächste Bit
        inc   bit                             ;das nächste Bit
        cmp   bit,8
        jl    l104
;
; Ende, Schleife über alle Bits
;
l105:   poping <di,si,ds>
  lokalFree
        pop   bp
        ret
_egaOut ENDP

;
;  Kopieren von src nach dest, dabei alle Bytes aus src verdreifachen:
;  dest[0] = scr[0], dest[1] = scr[0], dest[2] = src[0], dest[3] = src[1] ...
;  dest : Feld für Destination Bytes, Länge size * 3
;  src  : Feld mit den Source Bytes, Länge size
;  size : Anzahl der zu kopierenden Source-Bytes (<= 64K/3)
;
;  void _cdecl multiCopy ( byte* dest, const byte* src, unsigned size );
;
function _multiCopy
        push bp
        mov bp,sp
  argument dest, DWORD, 4
  argument src,  DWORD, 4 
  argument size, WORD, 4
        pushing <di,si,ds>
        les	di,dest
        lds	si,src
        mov	cx,size
l200:	lodsb
        mov	ah,al
	    stosw
        stosb
        dec	cx
        jnz	l200
        poping <ds,si,di,bp>
        ret
_multiCopy ENDP

_TEXT ends
      end

