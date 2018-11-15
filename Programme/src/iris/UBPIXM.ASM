        PAGE 60,132
;---------------------------------------------------------------------
;  MS-Macro-Assembler
;
; ubpixm.asm entstanden aus dem disassemblieren Modul ispixm.obj
; aus Bibliothek  dtisclli.lib
;
;  29.09.1992  Beginn
;---------------------------------------------------------------------

_data   SEGMENT WORD PUBLIC 'DATA'

        EXTRN i$flags : WORD
        EXTRN i$blkmove1_gdt : WORD
        EXTRN i$blkmove2_gdt : WORD
        EXTRN i$sub_frame : BYTE

_data   ENDS

dtiris_text   SEGMENT BYTE PUBLIC 'CODE'
dgroup  GROUP _data

        ASSUME CS : dtiris_text, DS : dgroup

        EXTRN i$errp : FAR
        EXTRN i$findfr : FAR
        EXTRN i$wait : FAR

        PUBLIC _isb_put_pixel,_isb_get_pixel

;---------------------------------------------------------------------
; int _far _cdecl
;  isb_get_pixel ( int frb, int zeile, unsigned count, char _far* buffer )
;
; Parameter : frb = framebuffer nummer
;             zeile = Zeile ab der Pixel gespeichert werten
;             count = Anzahl der Pixel / 2 = Anzahl der Words
;             buffer = hierhin werden die Daten geschrieben
; Rückgabe  : Fehlercode
;
; entstanden aus _is_get_pixel
; Veränderungen:
;       - Parameter Spalte weggelassen immer 0 == Anfang der Spalte
;       - 'count' gibt Anzahl der Words = 2 Pixel
;       - weggelassen Überprüfung der Parameter 'zeile', 'count'
;       - normalisieren des Zeigers buffer
;---------------------------------------------------------------------
_isb_get_pixel PROC FAR
        sub   sp,2                            ;2 bytes für rückgabe wert zwischenspeichern
        push  bp                              ;frame pointer sichern
        mov   bp,sp                           ;mit bp lokale varibalen adressieren
        mov   WORD PTR 2[bp],0                ;rückgabewert = 0 == ise_normal
        push  di                              ;register sichern
        push  si

        test  i$flags,1                       ;ist modul initialisiert ?
        jne   l2bc                            ;  ja
        jmp   l321                            ;  nein, fehlerausgang
l2bc:   mov   bx,8[bp]                        ;bx = frb
        or    bx,bx                           ;frb < 0
        jl    l326                            ;  ja, ungültiger wert
        cmp   bx,7FH                          ;frb > 127
        jg    l326                            ;  ja, ungültiger wert
        call  i$findfr                        ;frame buffer selektieren
        jb    l32b                            ;  fehler, buffer not found
        mov   dl,bl
        mov   dh,bl
        call  i$wait
        mov   di,OFFSET dgroup:i$blkmove1_gdt
        mov   al,1[si]
        mov   14H[di],al                      ;bits 16-23 der start-segment adresse
        mov   ax,2[si]
        mov   12H[di],ax                      ;bits 0-15 der start-segment adresse

        xor   dx,dx
        mov   ax,10H[bp]                      ;ax = buffer-segment

        shl   ax,1                            ;aus dx:ax segment
        rcl   dx,1                            ;  mach dx:ax absolut
        shl   ax,1
        rcl   dx,1
        shl   ax,1
        rcl   dx,1
        shl   ax,1
        rcl   dx,1

        add   ax,0EH[bp]                      ;buffer-offset addieren
        adc   dx,0

        mov   1CH[di],dl                      ;bits 16-23 der ziel-segment adresse
        mov   1AH[di],ax                      ;bits 0-15 der ziel-segment adresse

        mov   ax,0AH[bp]                      ;ax = zeile
        mov   dx,200H
        mul   dx                              ;dx:ax = zeile * 512
        add   12H[di],ax                      ;zeile im start-segment berücksichtigen
        adc   14H[di],dl
        jmp   SHORT l336

l321:   mov   ax,65H                          ;65H == 101 == ise_not_initialized
        jmp   SHORT l32e
l326:   mov   ax,64H                          ;64H == 100 == ise_argument
        jmp   SHORT l32e
l32b:   mov   ax,6BH                          ;6BH == 107 == ise_buffer_not_found
l32e:   call  i$errp
        jmp   l3d4

l336:   mov   cx,0CH[bp]                      ;cx = count

        push  10H[di]                         ;start-segmentlänge sichern
        push  18H[di]                         ;ziel-segmentlänge sichern

        mov   dx,cx
        shl   dx,1
        mov   10H[di],dx                      ;start-segmentlänge = count
        mov   18H[di],dx                      ;ziel-segmentlänge = count

        mov   ax,ds
        mov   es,ax                           ;es:si auf gdt
        mov   si,di
        mov   ah,87H                          ;speicherbereich verschieben
        int   15H                             ;at-bios call

        pop   18H[di]                         ;ziel-segmentlänge zurück
        pop   10H[di]                         ;start-segmentlänge zurück

l3d4:   pop   si                              ;register wiederherstellen
        pop   di
        pop   bp                              ;frame pointer wiederherstellen
        pop   ax                              ;rückgabewert in ax übergeben
        ret
_isb_get_pixel ENDP

;---------------------------------------------------------------------
; int _far _cedcl
;  isb_put_pixel ( int frb, int zeile, unsigned count, char _far* buffer )
;
; Parameter : frb = frame buffer number
;             zeile = Zeile ab der die Pixel übertragen werden
;             count = Anzahl der Pixel / 2 = Anzahl der Words
;             buffer = Speicher aus dem die Pixel kommen
; Rückgabe  : Fehlercode
;
; entstanden aus isb_get_pixel, nur die Richtung umgekehrt
;---------------------------------------------------------------------
_isb_put_pixel PROC FAR
        sub   sp,2                            ;2 bytes für rückgabe wert zwischenspeichern
        push  bp                              ;frame pointer sichern
        mov   bp,sp                           ;mit bp lokale varibalen adressieren
        mov   WORD PTR 2[bp],0                ;rückgabewert = 0 == ise_normal
        push  di                              ;register sichern
        push  si

        test  i$flags,1                       ;ist modul initialisiert ?
        jne   M2bc                            ;  ja
        jmp   M321                            ;  nein, fehlerausgang
M2bc:   mov   bx,8[bp]                        ;bx = frb
        or    bx,bx                           ;frb < 0
        jl    M326                            ;  ja, ungültiger wert
        cmp   bx,07FH                         ;frb > 127
        jg    M326                            ;  ja, ungültiger wert
        call  i$findfr                        ;frame buffer selektieren
        jb    M32b                            ;  fehler, buffer not found
        mov   dl,bl
        mov   dh,bl
        call  i$wait
        mov   di,OFFSET dgroup:i$blkmove1_gdt
        mov   al,1[si]
        mov   1CH[di],al                      ;bits 16-23 der ziel-segment adresse
        mov   ax,2[si]
        mov   1AH[di],ax                      ;bits 0-15 der ziel-segment adresse

        xor   dx,dx
        mov   ax,10H[bp]                      ;ax = buffer-segment

        shl   ax,1                            ;aus dx:ax segment
        rcl   dx,1                            ;  mach dx:ax absolut
        shl   ax,1
        rcl   dx,1
        shl   ax,1
        rcl   dx,1
        shl   ax,1
        rcl   dx,1

        add   ax,0EH[bp]                      ;buffer-offset addieren
        adc   dx,0

        mov   14H[di],dl                      ;bits 16-23 der start-segment adresse
        mov   12H[di],ax                      ;bits 0-15 der start-segment adresse

        mov   ax,0AH[bp]                      ;ax = zeile
        mov   dx,200H
        mul   dx                              ;dx:ax = zeile * 512
        add   1AH[di],ax                      ;zeile im zeil-segment berücksichtigen
        adc   1CH[di],dl
        jmp   SHORT M336

M321:   mov   ax,65H                          ;65H == 101 == ise_not_initialized
        jmp   SHORT M32e
M326:   mov   ax,64H                          ;64H == 100 == ise_argument
        jmp   SHORT M32e
M32b:   mov   ax,6BH                          ;6BH == 107 == ise_buffer_not_found
M32e:   call  i$errp
        jmp   M3d4

M336:   mov   cx,0CH[bp]                      ;cx = count

        push  10H[di]                         ;start-segmentlänge sichern
        push  18H[di]                         ;ziel-segmentlänge sichern

        mov   dx,cx
        shl   dx,1
        mov   10H[di],dx                      ;start-segmentlänge = count
        mov   18H[di],dx                      ;ziel-segmentlänge = count

        mov   ax,ds
        mov   es,ax                           ;es:si auf gdt
        mov   si,di
        mov   ah,087H                         ;speicherbereich verschieben
        int   015H                            ;at-bios call

        pop   18H[di]                         ;ziel-segmentlänge zurück
        pop   10H[di]                         ;start-segmentlänge zurück

M3d4:   pop   si                              ;register wiederherstellen
        pop   di
        pop   bp                              ;frame pointer wiederherstellen
        pop   ax                              ;rückgabewert in ax übergeben
        ret
_isb_put_pixel ENDP

;---------------------------------------------------------------------
dtiris_text   ENDS
        END
