;	.list
	.include	"define.inc"

	.ifndef	MAKE_NES
MAKE_NES	.equ	0
	.endif

	.ifndef	START_SONG
START_SONG	.equ	1
	.endif

	
	.if MAKE_NES
; INES header setup
	.inesprg	2	; 16k PRG bank
	.ineschr	1	; 8k CHR bank
	.inesmir	0	; Vertical map mirroring
	.inesmap	0	; Mapper
	.endif

    .bank	0
    .org	$8000
    .code

;NSF HEADER
	db	"NESM",$1A	;
	db	1		;Ver.
	db	TOTAL_SONGS	;Number of Songs
	db	START_SONG	;Start Song No.
	dw	LOAD		;Load
	dw	INIT		;Init
	dw	PLAY		;Play
  .org	$800E
	TITLE
;	db	"Title",$0
  .org	$802E
	COMPOSER
;	db	"Composer",$0
  .org	$804E
	MAKER
;	db	"Maker",$0
  .org	$806E
	dw	16666		;1000000 / (freq of NTSC) sec
	.if	(ALLOW_BANK_SWITCH)
	BANKSWITCH_INIT_MACRO
	.else
	db	0,0,0,0,0,0,0,0 ;Bankswitch Init Values
	.endif
	dw	20000		;1000000 / (freq of PAL)  sec
	db	%00
;                ||
;                |+-------------- PAL/NTSC
;                +--------------- dual PAL/NTSC tune or not
;	db	$02

__VRC6	=	%00000001
__VRC7	=	%00000010
__FDS	=	%00000100
__MMC5	=	%00001000
__N106	=	%00010000
__FME7	=	%00100000
	db	SOUND_GENERATOR
	db	0,0,0,0
LOAD:
INIT:
	jsr	sound_init
	rts
PLAY:
	jsr	sound_driver_start
	rts
;-------------------------------------------------------------------------------
	.include	"ppmck/sounddrv.h"
	.include	"ppmck/internal.h"
	.include	"ppmck/dpcm.h"
	.if	SOUND_GENERATOR & __FDS
	.include	"ppmck/fds.h"
	.endif
	.if	SOUND_GENERATOR & __VRC7
	.include	"ppmck/vrc7.h"
	.endif
	.if	SOUND_GENERATOR & __VRC6
	.include	"ppmck/vrc6.h"
	.endif
	.if	SOUND_GENERATOR & __N106
	.include	"ppmck/n106.h"
	.endif
	.if	SOUND_GENERATOR & __MMC5
	.include	"ppmck/mmc5.h"
	.endif
	.if	SOUND_GENERATOR & __FME7
	.include	"ppmck/fme7.h"
	.endif
	.include	"ppmck/freqdata.h"
	.include	"effect.h"

;-------------------------------------------------------------------------------
	.if MAKE_NES

songno    = $0100
pad_click = $0101
pad_press = $0102
nmi_flag  = $0103 ;$ff when play needed
wantinit  = $0104 ;$ff when init

 .bank 0

NMI:
	bit	wantinit
	bmi	song_init_1
	bit	nmi_flag
	bmi	do_rti		; 処理落ち
	dec	nmi_flag
IRQ:
do_rti:
	rti

song_init_1:
	ldx	#$ff
	txs
	inx
	jmp	song_init


RESET:				; このあたりのXレジスタの使い方はQuietustさんの方法を参考にしました
	sei
	cld
	ldx	#$00
	stx	$2000
	stx	$2001

	stx	pad_click
	stx	pad_press

	inx
.wv:	lda	$2002
	bpl	.wv
	dex
	bpl	.wv
	txs		;#$ff
	inx		;0
	txa
.ram:	sta	<$0000,x
	;sta	$0100,x
	sta	$0200,x
	sta	$0300,x
	sta	$0400,x
	sta	$0500,x
	sta	$0600,x
	sta	$0700,x
	inx
	bne	.ram

	lda	#START_SONG-1
	sta	songno
song_init:

	ldx	#$00			; X = 0 indicates NTSC
	stx	$2000
	stx	nmi_flag
	stx	wantinit
	stx	$4015
	lda	songno
	jsr	INIT
	
;.wv:	lda	$2002		; unneeded?
;	bpl	.wv
	
	lda	#$80		; PPU NMI on
	sta	$2000

mainloop:
	bit	nmi_flag	; wait NMI
	bpl	mainloop
	
main_routine:

read_pad:			;NESAudioRipping.TXTを参考にしました
	lda	pad_press
	sta	pad_click
	ldy	#$08
	ldx	#$01
	stx	$4016
	dex
	stx	$4016
.nextbit:
	lda	$4016	; A B Select Start Up Down Left Right
	ror	a	; bit0 into C
	txa		; 
	ror	a	; C into bit7
	tax		; X=A=C<<7|X>>1
	dey
	bne	.nextbit
	sta	pad_press
	eor	pad_click		;このあたりはkz-sさんのsnddrv3を参考にしました
	and	pad_press
	sta	pad_click		;今押されたボタン

check_pad:
	lda	pad_click
	tax			; X = pad_click
	beq	.check_pad_end
; Start
;	txa
	and	#$08
	bne	.set_wantinit

; Left
	txa
	and	#$40
	beq	.skipLeft
	
	dec	songno
	bpl	.set_wantinit
	inc	songno
	bpl	.check_pad_end
.set_wantinit:
	dec	wantinit
	bmi	.check_pad_end	;always

; Right	
.skipLeft:
	txa
	and	#$80
	beq	.skipRight
	inc	songno
	lda	#TOTAL_SONGS
	cmp	songno
	bne	.set_wantinit
	dec	songno
.skipRight:

.check_pad_end:

	jsr	PLAY

	inc	nmi_flag
	jmp	mainloop

 .bank 3
 .org $FFFA
	
	dw	NMI
	dw	RESET
	dw	IRQ

 .bank 4
	; NROM should have 8KB CHR-ROM
	
	.endif

