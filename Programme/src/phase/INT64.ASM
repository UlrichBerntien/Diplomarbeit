   PAGE 60,120
   .SALL
;
; Microsoft - Macro - Assembler V 4.00
;
; INT64   - 64 Bit Integer-Arithmetik
; für Speichermodelle mit 32-Bit Datenbereich HUGE,
; LARGE, COMPACT
;
; Ulrich Berntien .03.1993
;
; Literatur :
;   Donald E. Knuth : The Art of Computer Programming
;   Addison-Wesley; Reading, Mass. (1981)
;   Volume II, Chapter 4
; dabei berücksichtigen :
;   bei Knuth : Index 0 = höchstwertigste Stelle
;   bei Intel : Index 0 = niederwertigste Stelle
;
; 24.03.1993  Beginn
;-------------------------------------
; die 64-Bit Integer werden gespeichert als vier 16-Bit
; typedef int int64 [4];
;-------------------------------------

ilen    EQU   4                               ; Anzahl der Ints in int64

; Register push'en
;-------------------------------------
pushing MACRO regs
         IRP reg,<regs>
          push reg
         ENDM
        ENDM

; Register pop'en
;-------------------------------------
poping MACRO regs
        IRP reg,<regs>
         pop reg
        ENDM
       ENDM

; reg += 2
;-------------------------------------
INC2 MACRO reg
       inc reg
       inc reg
     ENDM

; reg -= 2
;-------------------------------------
DEC2 MACRO reg
       dec reg
       dec reg
     ENDM

int64   SEGMENT PUBLIC WORD 'CODE'
;=====================================
        ASSUME cs : int64, ds : NOTHING, ss : NOTHING, es : NOTHING
        .8087

; void _far _pascal to64 ( int64 a, int b )
; a = (int64) b
;-------------------------------------
PUBLIC   TO64
TO64    PROC  FAR
        push  bp
        mov   bp,sp
        push  di
        mov   ax,[bp+06]                      ;ax = b
        les   di,[bp+08]                      ;es:di = a
        stosw
        cwd
        mov   ax,dx
        mov   cx,ilen-1
        rep stosw
        poping <di,bp>
        ret   6
TO64    ENDP

; int _far _pascal form64 ( int* a, const int64 b )
; *a = (int) b
; return != 0, wenn b nicht im int-Bereich liegt
;-------------------------------------
PUBLIC   FROM64
FROM64  PROC FAR
        push  bp
        mov   bp,sp
        push  si
        les   si,[bp+06]                      ;es:si = b
        add   si,ilen * 2 - 2                 ;es:si = b + ilen -1
        std
        mov   cx,ilen - 1
        lodsw
        or    ax,ax
        jnz   l700                            ;b < 0
l701:   or    ax,ax
        jnz   l702                            ;Bereichsüberschreitung
        lodsw
        loop  l701
        cmp   ax,7FFFH
        jbe   l704
l702:   mov   ax,1                            ;Fehler zurückgeben
        jmp SHORT l703
                                              ;falls b < 0
l700:   cmp   ax,0FFFFH
        jne   l702                            ;Bereichsüberschreitung
        lodsw
        loop  l700
        cmp   ax,-7FFFH
        jb    l702                            ;Bereichsüberschreitung
l704:   les   bx,[bp+10]                      ;es:bx = *a
        mov   es:[bx],ax
        xor   ax,ax                           ;Umwandlung gelungen
l703:   cld
        poping <si,bp>
        ret   8
FROM64 ENDP

; void _far _pascal null64 ( int64 a )
; a = (int64) 0
;-------------------------------------
PUBLIC   NULL64
NULL64  PROC  FAR
        push  bp
        mov   bp,sp
        push  di
        les   di,[bp+06]                      ;es:di = a
        mov   cx,ilen
        xor   ax,ax
        rep stosw
        poping <di,bp>
        ret   4
NULL64  ENDP

; void _far _pascal let64 ( int 64 a, const int64 b )
; a = b
;-------------------------------------
PUBLIC   LET64
LET64   PROC FAR
        push  bp
        mov   bp,sp
        pushing <ds,si,di>
        lds   si,[bp+06]                      ;ds:si = b
        les   di,[bp+10]                      ;es:di = a
        mov   cx,ilen
        rep movsw
        poping <di,si,ds,bp>
        ret 8
LET64   ENDP

        EVEN
c1      DW 16                                 ;Variablen für double64
cbase   DW 16 * ilen
buffer  DD 0

; void _far _pascal double64 ( double* a, const int64 b )
; *a = (double) b
;-------------------------------------
PUBLIC   DOUBLE64
DOUBLE64 PROC FAR
        push  bp
        mov   bp,sp
        push  ds
        fild  c1
        lds   bx,[bp+06]                      ;ds:bx = b
        add   bx,( ilen -  1 ) * 2            ;ds:bx = b + ilen - 1
        mov   ax,[bx]
        mov   WORD PTR buffer,ax
        fild  buffer
        mov   cx,ilen-1
l500:   DEC2  bx
        mov   ax,[bx]
        fscale
        mov   WORD PTR buffer,ax
        fiadd buffer
        loop  l500
        mov   bx,[bp+06]                      ;ds:bx = b
        cmp   WORD PTR [bx+ilen*2-2], 0
        jge   l501
        fild  cbase                           ;falls b < 0
        fld1
        fscale
        fsubp st(2),st                        ;x - 2^64
        fstp  st                              ;die cbase vom FPU-Stack
l501:   lds   bx,[bp+10]                      ;ds:bx = a
        fstp  QWORD PTR [bx]
        fstp  st                              ;die c1 vom FPU-Stack
        poping <ds,bp>
        ret 8
DOUBLE64 ENDP

; void _far _pascal inc64 ( int64 a, int b )
; a += b;
;-------------------------------------
PUBLIC   INC64
INC64   PROC  FAR
        push  bp
        mov   bp,sp
        push  ds
        mov   ax,[bp+06]                      ;ax = b
        lds   bx,[bp+08]                      ;ds:bx = a
        or    ax,ax
        js    l001
        xor   dx,dx                           ;falls b > 0
        add   [bx],ax
        i = 2
        REPT ilen - 1
          jnc   l000
          adc   [bx+i],dx
          i = i + 2
        ENDM
        poping <ds,bp>
        ret 6
l001:   add   [bx],ax                         ;falls b < 0
        mov   dx,-1
        i = 2
        REPT ilen - 1
          jc    l000
          adc   [bx+i],dx
          i = i + 2
        ENDM
l000:   poping <ds,bp>
        ret   6
INC64   ENDP

; void _far _pascal INC64L ( int64 a, long b )
; a += b
;-------------------------------------
PUBLIC   INC64L
INC64L  PROC  FAR
        push  bp
        mov   bp,sp
        push  ds
        mov   ax,[bp+06]                      ;ax = (int) b
        mov   dx,[bp+08]                      ;dx = (int) ( b >> 16 )
        lds   bx,[bp+10]                      ;ds:bx = a
        or    dx,dx
        js    l101
        add   [bx],ax                         ;falls b > 0
        adc   [bx+2],dx
        mov   dx,0
        i = 4
        REPT ilen - 2
          jnc   l100
          adc   [bx+i],dx
          i = i + 2
        ENDM
        poping <ds,bp>
        ret   8
l101:   add   [bx],ax                         ;falls b < 0
        adc   [bx+2],dx
        mov   dx,-1
        i = 4
        REPT ilen - 2
          jc    l100
          adc   [bx+i],dx
          i = i + 2
        ENDM
l100:   poping <ds,bp>
        ret   8
INC64L  ENDP

; void _far _pascal inc64x ( int64 a, const int64 b )
; a += b
;------------------------------------
PUBLIC   INC64X
INC64X  PROC FAR
        push  bp
        mov   bp,sp
        pushing <ds,si>
        lds   si,[bp+06]                      ;ds:si = b
        les   bx,[bp+10]                      ;es:bx = a
        mov   ax,[si]
        add   es:[bx],ax
        i = 2
        REPT ilen - 1
          mov  ax,[si+i]
          adc  es:[bx+i],ax                   ;a[j] += b[i] + Carry
          i = i + 2
        ENDM
        poping <si,ds,bp>
        ret 8
INC64X  ENDP

; void _far _pascal dec64x ( int64 a, const int64 b )
; a -= b
;------------------------------------
PUBLIC   DEC64X
DEC64X  PROC FAR
        push  bp
        mov   bp,sp
        pushing <ds,si>
        lds   si,[bp+06]                      ;ds:si = b
        les   bx,[bp+10]                      ;es:bx = a
        mov   ax,[si]
        sub   es:[bx],ax
        i = 2
        REPT ilen - 1
          mov  ax,[si+i]
          sbb  es:[bx+i],ax                   ;a[j] -= b[i] + Carry
          i = i + 2
        ENDM
        poping <si,ds,bp>
        ret 8
DEC64X  ENDP

; void _far _pascal neg64 ( int64 a )
; a = 0 - a
;------------------------------------
PUBLIC   NEG64
NEG64   PROC FAR
        push  bp
        mov   bp,sp
        push  ds
        lds   bx,[bp+06]                      ;ds:bx = a
        xor   cx,cx                           ;cx = 0
        neg   WORD PTR [bx]
        i = 2
        REPT ilen - 1
          mov   ax,[bx+i]
          mov   dx,cx
          sbb   dx,ax                         ;a[i] = 0 - a[i] - Carry
          mov   [bx+i],dx
          i = i + 2
        ENDM
        poping <ds,bp>
        ret 4
NEG64   ENDP

; void _far _pascal sub64
;              ( int64 a, const int64 b, const int64 c)
; a = b - c
;-------------------------------------
PUBLIC   SUB64
SUB64   PROC  FAR
        push  bp
        mov   bp,sp
        pushing <ds,si,di>
        lds   si,[bp+10]                      ;ds:si = b
        les   di,[bp+14]                      ;es:di = a
        mov   cx,ilen
        rep movsw                             ;a = b
        mov   di,[bp+14]                      ;es:di = a
        lds   si,[bp+06]                      ;ds:si = c
        mov   ax,[si]
        sub   es:[di],ax                      ;a -= c
        i = 2
        REPT ilen-1
          mov  ax,[si+i]
          sbb  es:[di+i],ax
          i = i + 2
        ENDM
        poping <di,si,ds,bp>
        ret   12
SUB64   ENDP

; int _far _pascal cmp cmp64 (const int64 a,const int64 b);
; return = 0    wenn a = b
;        < 0         a < b
;        > 0         a > b
;-------------------------------------
PUBLIC   CMP64
CMP64 PROC FAR
        push  bp
        mov   bp,sp
        pushing <ds,si,di>
        les   di,[bp+06]                      ;es:di = b
        lds   si,[bp+10]                      ;ds:si = a
        mov   cx,ilen
        mov   dx,ilen * 2 - 2
        add   di,dx                           ;es:di = b + ilen - 1
        add   si,dx                           ;si:di = a + ilen - 1
        std
        repe cmpsw                            ;gleiche Stellen übergehen
        mov   ax,ds:[si+2]
        sub   ax,es:[di+2]                    ;Diff. höchste unterschiedliche Stelle
        cld
        poping <di,si,ds,bp>
        ret   8
CMP64 ENDP

; void _far _pascal mul64 ( int64 a, const int64 b, int c )
; a = b * c
;-------------------------------------
PUBLIC   MUL64
MUL64   PROC  FAR
        push  bp
        mov   bp,sp
        pushing <ds,si,di>
        mov   cx,[bp+06]                      ;cx = c
        or    cx,cx
        jns   l200
        neg   cx                              ;cx = abs(c)
l200:   lds   si,[bp+08]                      ;ds:si = b
        les   di,[bp+12]                      ;es:di = a
        mov   ax,[si]
        mul   cx
        mov   es:[di],ax
        i = 2
        REPT ilen - 1
          mov   bx,dx
          mov   ax,[si+i]
          mul   cx
          add   ax,bx
          mov   es:[di+i],ax
          adc   dx,0
          i = i + 2
        ENDM
        cmp   WORD PTR [bp+06],0
        jge   l201
        push  es                              ;falls c < 0
        push  [bp+12]
        call  NEG64                           ;a = - ( b * abs(c) )
l201:   poping <di,si,ds,bp>
        ret   10
MUL64   ENDP

; void _far _pascal mul64l ( int64 a, const int64 b, long c )
; a = b * c
;-------------------------------------
PUBLIC   MUL64L
MUL64L  PROC  FAR
        push  bp
        mov   bp,sp
        xor   ax,ax
        push  ax                              ;lokale Variable, Vorzeichen
        pushing <ds,si,di>
        mov   cx,[bp+06]                      ;bx:cx = c
        mov   bx,[bp+08]
        or    bx,bx
        jns   l300
        xor   ax,ax                           ;falls c < 0
        neg   cx                              ;cx = (int) abs(c)
        sbb   ax,bx
        mov   [bp+08],ax
        mov   WORD PTR [bp-02],1              ;Vorzeichenwechsel merken
l300:   lds   si,[bp+10]                      ;ds:si = b
        les   di,[bp+14]                      ;es:di = a
        mov   ax,[si]
        mul   cx
        mov   es:[di],ax
        i = 2
        REPT ilen - 1
          mov   bx,dx                         ;bx = CARRY
          mov   ax,[si+i]
          mul   cx
          add   ax,bx
          mov   es:[di+i],ax
          adc   dx,0
          i = i + 2
        ENDM
        mov   cx,[bp+08]                      ;cx = (int) ( c >> 16 )
        jcxz  l302                            ;cx == 0 -> keine Mul. nötig
        mov   si,[bp+10]                      ;ds:si = b
        mov   di,[bp+14]                      ;es:di = a
        mov   ax,[si]
        mul   cx
        add   es:[di+2],ax
        i = 2
        REPT ilen - 2
          adc   dx,0
          mov   bx,dx                         ;bx = CARY
          mov   ax,[si+i]
          mul   cx
          add   ax,bx
          adc   dx,0
          add   es:[di+i+2],ax
          i = i + 2
        ENDM
l302:   cmp   WORD PTR [bp-02],0              ;Vorzeichen umgedreht ?
        je    l301
        push  es                              ;falls c < 0
        push  [bp+14]
        call  NEG64                           ;a = - ( b * abs(c) )
l301:   poping <di,si,ds,ax,bp>
        ret   12
MUL64L  ENDP

; void _far _pascal mul64ll ( int64 a, long b, long c )
; a = b * c
;-------------------------------------
PUBLIC   MUL64LL
MUL64LL PROC  FAR
        push  bp
        mov   bp,sp
        xor   ax,ax
        push  ax                              ;lokale Variable
        pushing <ds,si>
        mov   bx,[bp+08]                      ;bx:cx = c
        mov   cx,[bp+06]
        or    bx,bx
        jns   l400
        xor   dx,dx                           ;falls c < 0
        neg   cx
        sbb   dx,bx                           ;bx:cx = 0 - bx:cx
        mov   [bp+08],dx
        not   WORD PTR [bp-02]                ;Vorzeichentausch merken
l400:   mov   bx,[bp+12]
        mov   ax,[bp+10]                      ;bx:ax = b
        or    bx,bx
        jns   l401
        xor   dx,dx                           ;falls b < 0
        neg   ax
        sbb   dx,bx                           ;bx:ax = 0 - bx:ax
        mov   [bp+10],ax
        mov   [bp+12],dx
        not   WORD PTR [bp-02]                ;Vorzeichentausch merken
l401:   lds   si,[bp+14]                      ;ds:si = a
        mul   cx
        mov   [si],ax
        mov   [si+2],dx
        mov   ax,[bp+12]                      ;ax = (int) ( b >> 16 )
        mul   cx
        add   [si+2],ax
        adc   dx,0
        mov   [si+4],dx
        mov   cx,[bp+08]                      ;cx = (int) ( c >> 16 )
        mov   ax,[bp+10]                      ;ax = (int) b
        mul   cx
        add   [si+2],ax
        adc   dx,0
        mov   bx,dx                           ;bx = CARRY
        mov   ax,[bp+12]                      ;ax = (int) ( b >> 16 )
        mul   cx
        add   ax,bx
        adc   dx,0
        adc   [si+4],ax
        adc   dx,0
        mov   [si+6],dx
        cmp   WORD PTR [bp-02],0               ;Vorzeichentausch ?
        je    l402
        push  ds
        push  [bp+14]
        call  NEG64                           ;a = - ( b * c )
l402:   poping <si,ds,ax,bp>
        ret   12
MUL64LL ENDP

;  Variablen für Prozedur DIV64
;
vorzeichen DB (?)                             ;Ergebnis negativ ?
shift   DB (?)                                ;wurde normalisiert ?
        EVEN
        DW 0                                  ;Zugriff auf u[j+2] immer erlauben
u       DW ilen+1 DUP (?)
iu0     DW (?)                                ;Adresse erste Stelle von u
su      DW (?)                                ;Anzahl Stellen von u
        DW 0                                  ;Zugriff auf v[2] immer erlauben
v       DW ilen DUP (?)
iv0     DW (?)                                ;Adresse erste Stelle von v
sv      DW (?)                                ;Anzahl Stellen von v
q       DW ilen DUP (?)                       ;q = u / v
iq      DW (?)                                ;Adresse für nächstes q
qd      DW (?)                                ;q-Dach
rd      DW (?)                                ;r-Dach

; int _far _pascal div64
;                ( int64 c, const int64 b, const int64 a )
; return =! 0 im Fehlerfall
; c = b / a
;-------------------------------------
PUBLIC   DIV64
DIV64   PROC  FAR
        push  bp
        mov   bp,sp
        pushing <ds,si,di>
        lds   si,[bp+06]                      ;ds:si = a
        mov   ax,cs
        mov   es,ax
        mov   di,OFFSET v                     ;es:si = v
        mov   cx,ilen
        rep movsw                             ;v = a
        lds   si,[bp+10]                      ;ds:si = b
        mov   di,OFFSET u                     ;es:di = u[1]
        mov   cx,ilen
        rep movsw                             ;u[0..ilen-1] = b
        mov   ds,ax
        mov   es,ax
     ASSUME ds : INT64,  es : INT64
        mov   vorzeichen,0
        mov   shift,0                         ;Variablen belegen
        mov   iq,OFFSET q + ilen*2 -2
        cmp   [u+ilen*2-2],0                  ;Vorzeichen von u ?
        jge   l601
        push  ds                              ;falls u < 0
        lea   ax,[u]
        push  ax
        call  NEG64                           ;u = -u
        mov   vorzeichen,1
l601:   cmp   [v+ilen*2-2],0                  ;Vorzeichen von v ?
        jge   l602
        push  ds                              ;falls v < 0
        lea   ax,[v]
        push  ax
        call  NEG64                           ;v = -v
        neg vorzeichen
l602:   std
        lea   di,[v+ilen*2-2]                 ;erste Stelle von v suchen
        mov   cx,ilen
        xor   ax,ax
        repz scasw
        jnz   l603                            ;erstes v[i] != 0
        jmp   l6FF                            ;Division durch 0
l603:   INC2  di
        inc   cx
        mov   iv0,di                          ;erste Stelle von v
        mov   sv,cx                           ;Anzahl Stellen von v
        mov   ax,[di]
        or    ah,ah                           ;ist v[i] >= basis/2 ?
        jnz   l605                            ;  ja, alles ok
        mov   ah,al                           ;  nein, Normalisieren (D1)
        jmp   SHORT l606                      ;Multiplikation mit 100H
l607:   mov   bx,[di-2]
        mov   al,bh
        stosw
        mov   ah,bl
l606:   loop  l607
        xor   al,al
        mov   [di],ax
        mov   shift,1
l605:   lea   di,[u+ilen*2-2]                 ;erste Stelle von u suchen
        mov   cx,ilen
        xor   ax,ax
        repz scasw
        jnz   l609                            ;erstes u[i] != 0
        jmp   l6FE                            ;0 = 0 / a
l609:   add   di,4                            ;u um eine Stelle verlängern
        INC2  cx
        mov   iu0,di                          ;erste Stelle von u
        mov   su,cx                           ;Anzahl Stellen von u
        cmp   shift,0
        je    l60A                            ;keine Normalisierung
l60C:   mov   bx,[di-2]                       ;mit 100H multiplizieren
        mov   al,bh
        stosw
        mov   ah,bl
        loop  l60C
        xor   al,al
        mov   [di],ax

l60A:   cld                                ;*** Normalisierungen fertig
        mov   ax,su
        cmp   ax,sv                           ;Stellen u > Stellen v ?
        ja    l60E                            ;  ja
        jmp   l6FE                            ;  nein : u / v = 0
                                              ;  (u wurde um 1 Stelle erweitert)
l60E:   ; u mindestens 3 Stellen : 2 und die Pseudostelle
        ; v mindestens 2 Stellen : 1 und die Pseudostelle

l611:   mov   di,iu0                       ;*** Initialisierung (D2) erfolgt
                                              ;Berechne q' (D3)
l618:   mov   dx,[di]                         ;dx = u[j]
        mov   si,iv0
        mov   cx,[si]                         ;cx = v[1]
        cmp   cx,dx                           ;u[j] == v[1] ?
        jne   l60F
        mov   ax,0EFFFH                       ;  ja, ax ( = q') = basis - 1
        mov   dx,[di-2]                       ;dx ( = r' ) = u[j+1]
        jmp SHORT l610
l60F:   mov   ax,[di-2]                       ;dx:ax = u[j] * basis + u[j-1]
        div   cx
        jmp   SHORT l612
l613:   mov   ax,qd                           ;q' muß verkleinert werden
        dec   ax
        mov   qd,ax
        mov   dx,rd                           ;r' wird dann größer :
l610:   add   dx,[si]                         ;dx ( = r' ) += v[1]
        jc    l614                            ;falls Carry : r' > basis
l612:   mov   qd,ax                           ;q' = ( u[j] * b + h[j-1] ) / v[1]
        mov   rd,dx                           ;r' = u[j] * b + u[j-1] - q'*v[1]
        mul   WORD PTR [si-2]                 ;dx:ax = q' * v[2]
        cmp   dx,rd                           ;v[2]*q' < r'*b + u[j+2] ?
        jb    l614                            ;  ja
        ja    l613                            ;  nein, q' verkleinern
        cmp   ax,[di-4]
        ja    l613                            ;  nein, q' verkleinern
l614:                                      ;*** Multiplizieren und Subtrahieren (D4)
        mov   cx,sv                           ;cx = Anzahl Stellen v
        xor   bx,bx                           ;bx = CARRY = 0
        lea   si,v                            ;si auf v[0]
        sub   di,cx                           ;di auf u[j+n]
        sub   di,cx
l615:   lodsw                                 ;ax = v[i]
        mul   qd                              ;dx:ax = v[i] * q'
        add   ax,bx
        adc   dx,0
        sub   [di],ax                         ;u[j+i] -= v[i]*q' + CARRY
        adc   dx,0
        mov   bx,dx
        INC2  di
        loop  l615
        xor   dx,dx
        sub   [di],bx                         ;u[j] -= v[i]*q' - CARRY
        adc   dx,0                            ;letzte CARRY in dx halten
					   ;*** Teste Rest (D5)
        mov   ax,qd                           ;q[j] = q'
        mov   di,iq
        mov   [di],ax
        DEC2  di                              ;--j
        mov   iq,di
        or    dx,dx                           ;CARRY ?
        jz    l616                            ;  nein, alles ok
                                              ;  ja, Add Back (D6)
        dec   WORD PTR [di+2]                 ;q' war 1 zu groß
        lea   si,v
        mov   di,iu0
        mov   cx,sv                           ;cx = Anzahl Stellen von v
        sub   di,cx                           ;di auf u[j+n]
        ; ^ dort kann kein Carry auftreten
l617:   lodsw
        adc   [di],ax                         ;u[j+i] += v[i] + Carry
        INC2  di
        loop  l617
        adc   WORD PTR [di],0                 ;u[j] += Carry
l616:                                      ;*** Schleife über j (D7)
        mov   di,iu0
        DEC2  di                              ;nächste Stelle von u
        mov   iu0,di
        mov   ax,su
        dec   ax                              ;u hat jetzt eine Stelle weniger
        mov   su,ax
        cmp   ax,sv                           ;Stellen u <= Stellen v ?
        jbe   l619                            ;  fertig
        jmp   l618                            ;  sonst, nächste Stelle berechnen

l619:   lea   di,[q]                          ;di auf q[0], niederwertigste Stelle
        mov   si,iq
        INC2  si                              ;si auf letzte berechnete Stelle
l61A:   movsw                                 ;verschiebe die berechneten Stellen
        cmp   si,OFFSET q + ilen*2 - 2
        jbe   l61A
        xor   ax,ax
        jmp   SHORT l61B
l61D:   stosw                                 ;alle höheren Stellen = 0
l61B:   cmp   di,OFFSET q + ilen*2 - 2
        jbe   l61D
        cmp   vorzeichen,0                    ;Resultat negativ ?
        jz    l61E                            ;  nein
        push  ds
        lea   si,q
        push  si
        CALL  NEG64                           ;q = -q
l61E:   mov   cx,ilen
     ASSUME es : NOTHING
        les   di,[bp+14]                      ;es:di = c
        lea   si,q
        rep movsw                             ;c = q
        xor   ax,ax
        jmp SHORT l600
l6FE:   cld
        xor   ax,ax                           ;Ergebnis ist 0
        mov   cx,ilen
        les   di,[bp+14]                      ;es:di = c
        rep stosw                             ;c = 0
        jmp SHORT l600
l6FF:   cld                                   ;Fehlerausgang
        mov   ax,1
l600:   poping <di,si,ds,bp>
     ASSUME ds : NOTHING
        ret   12
DIV64   ENDP

; long _far _pascal mul32 ( int a, int b )
; return = a * b
;-------------------------------------
PUBLIC   MUL32
MUL32   PROC FAR
        push  bp
        mov   bp,sp
        mov   ax,[bp+06]                      ;ax = b
        mov   cx,[bp+08]                      ;cx = c
        imul  cx                              ;ax:dx = b * c
        pop   bp
        ret   4
MUL32   ENDP

; long _far _pascal gauss32 ( int a, int b )
; return a*a + b*b
;-------------------------------------
PUBLIC   GAUSS32
GAUSS32 PROC FAR
        push  bp
        mov   bp,sp
        mov   ax,[bp+06]                      ;ax = b
        imul  ax
        mov   bx,ax
        mov   cx,dx                           ;bx:cx = b * b
        mov   ax,[bp+08]                      ;ax = a
        imul  ax                              ;ax:dx = a * a
        add   ax,bx
        adc   dx,cx                           ;ax:dx = a*a + b*b
        pop   bp
        ret   4
GAUSS32 ENDP

int64   ENDS
        END
