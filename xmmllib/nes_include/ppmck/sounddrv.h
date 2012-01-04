;-------------------------------------------------------------------------------
;internal 4 dpcm 1 fds 1 vrc7 6 vrc6 3 n106 8 fme7 3 mmc5 3(?)
;4+1+1+6+3+8+3+3=29ch
MAX_CH		=	29
CH_COUNT	=	PTR_TRACK_END
	.if	CH_COUNT > MAX_CH
	.fail	"memory out of range"
	.endif

;-------------------------------------------------------------------------------
;memory definition
;-------------------------------------------------------------------------------
;ゼロページメモリ定義
	.zp
	.org	$00
	
t0			.ds	1		; temp 
t1			.ds	1		;
t2			.ds	1		;
t3			.ds	1		;

sound_add_low		.ds	1		;command address
sound_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2

channel_sel		.ds	1		;
channel_selx2		.ds	1		;
channel_selx4		.ds	1		;

drvtmp0			.ds	1		; 各音源のドライバ内で使用する
drvtmp1			.ds	1		;
drvtmp2			.ds	1		;
drvtmp3			.ds	1		;

ind_lda_add		.ds	2		; for indirect_lda

temp_data_add		.ds	2		;

VRC6_DST_REG_LOW	.ds	1		; for vrc6.h
VRC6_DST_REG_HIGH	.ds	1		;

t4			.ds	1 ; for divider
t5			.ds	1
t6			.ds	1
t7			.ds	1

ps_temp		.ds	1

;-----------------------------------
;非ゼロページのメモリ定義

	.bss
BSS_BASE	=	$0200
	.org	BSS_BASE

;各チャンネルに必要なメモリ
;	ldx	<channel_selx2
;	してから
;	lda	memory,x
;	するので1バイトおきにデータがならぶ

;_add_lowと_add_highは近接している必要がある

soft_add_low		.ds	1		;software envelope(@v) address
soft_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
pitch_add_low		.ds	1		;pitch envelope (EP) address
pitch_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
duty_add_low		.ds	1		;duty envelope (@@) address
duty_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
arpe_add_low		.ds	1		;note envelope (EN) address
arpe_add_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_reverse_counter	.ds	1		;
lfo_adc_sbc_counter	.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_start_counter	.ds	1		;
lfo_start_time		.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_adc_sbc_time	.ds	1		;
lfo_depth		.ds	1		;
			.ds	CH_COUNT*2 - 2
lfo_reverse_time	.ds	1		;
lfo_sel			.ds	1		;vibrato (MP) no
			.ds	CH_COUNT*2 - 2
detune_dat		.ds	1		;detune value
fme7_tone
register_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
fme7_volume
register_low		.ds	1		;
duty_sel		.ds	1		;duty envelope no
			.ds	CH_COUNT*2 - 2
channel_loop		.ds	1		;|: :| loop counter
rest_flag		.ds	1		;
			.ds	CH_COUNT*2 - 2
softenve_sel		.ds	1		;software envelope(@v) no
pitch_sel		.ds	1		;pitch envelope (EP) no
			.ds	CH_COUNT*2 - 2
arpeggio_sel		.ds	1		;note envelope (EN) no
effect_flag		.ds	1		;
			.ds	CH_COUNT*2 - 2
sound_sel		.ds	1		;note no.
sound_counter		.ds	1		;wait counter
			.ds	CH_COUNT*2 - 2
sound_freq_low		.ds	1		;
sound_freq_high		.ds	1		;
			.ds	CH_COUNT*2 - 2
sound_freq_n106		.ds	1		;n106じゃないchでも使ってる
sound_bank		.ds	1		;
			.ds	CH_COUNT*2 - 2
pitch_shift_amount	.ds	1		;
n106_volume
vrc7_key_stat
			.ds	1		;
			.ds	CH_COUNT*2 - 2
n106_7c
vrc7_volume
			.ds	1		;
extra_mem2		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

ps_step			.ds	1		;
ps_count		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

ps_nextnote		.ds	1		;
ps_dummy		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

ps_addfreq_l		.ds	1		;
ps_addfreq_h		.ds	1		;
			.ds	CH_COUNT*2 - 2	;

effect2_flags		.ds	1
sound_lasthigh		.ds	1
			.ds	CH_COUNT*2 - 2	;

;-------------
;その他

temporary		.ds	1		;
temporary2		.ds	1		;

fds_hard_select		.ds	1		;
fds_volume		.ds	1		;

n106_7f			.ds	1		;

fds_hard_count_1	.ds	1		;
fds_hard_count_2	.ds	1		;

initial_wait		.ds	1		;

fme7_ch_sel		.ds	1		;
fme7_ch_selx2		.ds	1		;
fme7_ch_selx4		.ds	1		;
fme7_reg7		.ds	1		;R7現在値
fme7_vol_regno		.ds	1		;

DMC_NMI:
ram_nmi			.ds	3		;
DMC_RESET:
ram_reset		.ds	3		;
DMC_IRQ:
ram_irq			.ds	3		;

;effect_flag: DLLLadpv
;+------ detune flag
;l+----- software LFOスピード可変フラグ（予約）
;ll+---- software LFO方向フラグ0=- 1=+
;lll+--- software LFO flag
;llll+---- note enverope flag
;lllll+--- duty enverope flag / FDS hardware effect flag
;llllll+-- pitch enverope flag
;lllllll+- software enverope flag
;llllllll
;DLLLadpv


;rest_flag
;xxxxxxor
;|||||||+- rest
;||||||+-- key on (if set, must do sound_data_write)
;

	.code


;-------------------------------------------------------------------------------
;macros and misc sub routines
;-------------------------------------------------------------------------------
;indirect_lda
;statement
;	indirect_lda	hoge_add_low	;hoge_add_low is not zero page address
;is same as:
;	lda	[hoge_add_low,x]
indirect_lda	.macro
	lda	\1,x		;hoge_add_low
	sta	<ind_lda_add
	lda	\1+1,x		;hoge_add_high
	sta	<ind_lda_add+1
	ldx	#$00
	lda	[ind_lda_add,x]
	ldx	<channel_selx2
	.endm

;--------------------------------------
channel_sel_inc:
	inc	<channel_sel
	lda	<channel_sel
	asl	a
	sta	<channel_selx2
	asl	a
	sta	<channel_selx4
	rts

;-------------------------------------------------------------------------------
;initialize routine
;-------------------------------------------------------------------------------
INITIAL_WAIT_FRM = $00 ;最初にこのフレーム数だけウェイト
;初期化ルーチン
sound_init:
	.if TOTAL_SONGS > 1
	pha
	.endif
	lda	#$00
	ldx	#$00
.memclear
	sta	$0000,x
	sta	$0200,x
	sta	$0300,x
	sta	$0400,x
	sta	$0500,x
	sta	$0600,x
	sta	$0700,x
	inx
	bne	.memclear

	lda	#INITIAL_WAIT_FRM
	sta	initial_wait

	lda	#$0f		;内蔵音源初期化
	sta	$4015		;チャンネル使用フラグ
	lda	#$08		
	sta	$4001		;矩形波o2a以下対策
	sta	$4005

	.if (DPCM_BANKSWITCH)
	ldx	#$4C		; X = "JMP Absolute"
	stx	ram_nmi
	lda	$FFFA		; NMI low
	sta	ram_nmi+1
	lda	$FFFB		; NMI high
	sta	ram_nmi+2

	stx	ram_reset
	lda	$FFFC		; RESET low
	sta	ram_reset+1
	lda	$FFFD		; RESET high
	sta	ram_reset+2

	stx	ram_irq
	lda	$FFFE		; IRQ/BRK low
	sta	ram_irq+1
	lda	$FFFF		; IRQ/BRK high
	sta	ram_irq+2
	.endif


	.if	SOUND_GENERATOR & __FME7
	jsr	fme7_sound_init
	.endif

	.if	SOUND_GENERATOR & __MMC5
	jsr	mmc5_sound_init
	.endif

	.if	SOUND_GENERATOR & __FDS
	jsr	fds_sound_init
	.endif
	
	.if	SOUND_GENERATOR & __N106
	jsr	n106_sound_init
	.endif

	.if	SOUND_GENERATOR & __VRC6
	jsr	vrc6_sound_init
	.endif

; use t0, t1, t2, t3
.start_add_lsb	=	t0
.start_add_lsb_hi=	t1
.start_bank	=	t2
.start_bank_hi	=	t3

	.if TOTAL_SONGS > 1
		pla
		asl	a
		tax
		
		lda	song_addr_table,x
		sta	<.start_add_lsb
		lda	song_addr_table+1,x
		sta	<.start_add_lsb+1
		
		.if (ALLOW_BANK_SWITCH)
			lda	song_bank_table,x
			sta	<.start_bank
			lda	song_bank_table+1,x
			sta	<.start_bank+1
		.endif
	
	.endif
	
	lda	#$00
	sta	<channel_sel
	sta	<channel_selx2
	sta	<channel_selx4
.sound_channel_set:
	lda	<channel_sel
	cmp	#PTR_TRACK_END		;終わり？
	beq	.sound_init_end
	
	
	.if TOTAL_SONGS > 1
		.if (ALLOW_BANK_SWITCH)
			ldy	<channel_sel		; y = ch; x = ch<<1;
			ldx	<channel_selx2
			
			lda	[.start_bank],y
			sta	sound_bank,x
			
			ldy	<channel_selx2		; x = y = ch<<1;
		.else
			ldx	<channel_selx2		; x = y = ch<<1;
			ldy	<channel_selx2
		.endif
				
		lda	[.start_add_lsb],y
		sta	<sound_add_low,x	;データ開始位置書き込み
		iny
		lda	[.start_add_lsb],y
		sta	<sound_add_low+1,x	;データ開始位置書き込み
	.else
		
		ldy	<channel_sel		; y = ch; x = ch<<1;
		ldx	<channel_selx2

		.if (ALLOW_BANK_SWITCH)
			lda	song_000_bank_table,y
			sta	sound_bank,x
		.endif
		
		lda	song_000_track_table,x
		sta	<sound_add_low,x	;データ開始位置書き込み
		lda	song_000_track_table+1,x
		sta	<sound_add_low+1,x	;データ開始位置書き込み

	.endif
	; x = ch<<1; y = ?
	
	lda	#$ff
	sta	sound_lasthigh,x
	lda	#$00
	sta	effect_flag,x
	lda	#$01
	sta	sound_counter,x
	
	jsr	channel_sel_inc
	jmp	.sound_channel_set
.sound_init_end:
	rts

;-------------------------------------------------------------------------------
;main routine
;-------------------------------------------------------------------------------
sound_driver_start:

	lda	initial_wait
	beq	.gogo
	dec	initial_wait
	rts
.gogo

	lda	#$00
	sta	<channel_sel
	sta	<channel_selx2
	sta	<channel_selx4

internal_return:
	jsr	sound_internal
	jsr	channel_sel_inc
	lda	<channel_sel
	cmp	#$04
	bne	internal_return		;戻す

;	.if	DPCMON
sound_dpcm_part:
	jsr	sound_dpcm
;	.endif
	jsr	channel_sel_inc

	.if	SOUND_GENERATOR & __FDS
	jsr	sound_fds		;FDS行ってこい
	jsr	channel_sel_inc
	.endif

	.if	SOUND_GENERATOR & __VRC7
vrc7_return:
	jsr	sound_vrc7		;vrc7行ってこい
	jsr	channel_sel_inc
	lda	<channel_sel
	cmp	#PTRVRC7+$06		;vrc7は終わりか？
	bne	vrc7_return		;まだなら戻れ
	.endif

	.if	SOUND_GENERATOR & __VRC6
vrc6_return:
	jsr	sound_vrc6		;vrc6行ってこい
	jsr	channel_sel_inc
	lda	<channel_sel
	cmp	#PTRVRC6+$03		;vrc6は終わりか？
	bne	vrc6_return		;まだなら戻れ
	.endif

	.if	SOUND_GENERATOR & __N106
.rept:
	jsr	sound_n106		;n106行ってこい
	jsr	channel_sel_inc
	lda	<channel_sel
	sec
	sbc	#PTRN106
	cmp	n106_channel		;n106は終わりか？
	bne	.rept			;まだなら戻れ
	.endif

	.if	SOUND_GENERATOR & __FME7
fme7_return:
	jsr	sound_fme7		;fme7行ってこい
	jsr	channel_sel_inc
	lda	<channel_sel
	cmp	#PTRFME7+$03		;fme7は終わりか？
	bne	fme7_return		;まだなら戻れ
	.endif

	.if	SOUND_GENERATOR & __MMC5
mmc5_return:
	jsr	sound_mmc5		;mmc5行ってこい
	jsr	channel_sel_inc
	lda	<channel_sel
	cmp	#PTRMMC5+$02		;mmc5は終わりか？
	bne	mmc5_return		;まだなら戻れ
	.endif

	rts

;------------------------------------------------------------------------------
;command read sub routines
;------------------------------------------------------------------------------
sound_data_address:
	inc	<sound_add_low,x	;データアドレス＋１
	bne	return2			;位が上がったら
sound_data_address_inc_high
	inc	<sound_add_high,x	;データアドレス百の位（違）＋１
return2:
	rts

sound_data_address_add_a:
	clc
	adc	<sound_add_low,x
	sta	<sound_add_low,x
	bcs	sound_data_address_inc_high
	rts
;-------------------------------------------------------------------------------
change_bank:
;バンクをReg.Aに変えます～
;変更されるバンクアドレスはバンクコントローラによる
;現在はNSFのみ。
	if (ALLOW_BANK_SWITCH)
;バンク切り替えできるcondition: A <= BANK_MAX_IN_4KB
;i.e. A < BANK_MAX_IN_4KB + 1
;i.e. A - (BANK_MAX_IN_4KB+1) < 0
;i.e. NOT ( A - (BANK_MAX_IN_4KB+1) >= 0 )
;skipするcondition: A - (BANK_MAX_IN_4KB+1) >= 0
	cmp	#BANK_MAX_IN_4KB+1
	bcs	.avoidbankswitch
	sta	$5ffa ; A000h-AFFFh
	clc
	adc	#$01
	cmp	#BANK_MAX_IN_4KB+1
	bcs	.avoidbankswitch
	sta	$5ffb ; B000h-BFFFh
.avoidbankswitch
	endif
	rts

;-------------------------------------------------------------------------------
; リピート終了コマンド
;
; channel_loop++;
; if (channel_loop == <num>) {
;   channel_loop = 0;
;   残りのパラメータ無視してadrを次に進める;
; } else {
;   0xeeコマンドと同じ処理;
; }
loop_sub:
	jsr	sound_data_address
	inc	channel_loop,x
	lda	channel_loop,x
	cmp	[sound_add_low,x]	;繰り返し回数
	beq	loop_end
	jsr	sound_data_address
	jmp	bank_address_change
loop_end:
	lda	#$00
	sta	channel_loop,x
loop_esc_through			;loop_sub2から飛んでくる
	lda	#$04
	jsr	sound_data_address_add_a
	rts				;おちまい
;-----------
; リピート途中抜け
;
; channel_loop++;
; if (channel_loop == <num>) {
;   channel_loop = 0;
;   0xeeコマンドと同じ処理;
; } else {
;   残りのパラメータ無視してadrを次に進める;
; }

loop_sub2:
	jsr	sound_data_address
	inc	channel_loop,x
	lda	channel_loop,x
	cmp	[sound_add_low,x]	;繰り返し回数
	bne	loop_esc_through
	lda	#$00
	sta	channel_loop,x
	jsr	sound_data_address
	jmp	bank_address_change
;-------------------------------------------------------------------------------
;バンクセット (gotoコマンド。bank, adr_low, adr_high)
data_bank_addr:
	jsr	sound_data_address
bank_address_change:
	if (ALLOW_BANK_SWITCH)
	lda	[sound_add_low,x]
	sta	sound_bank,x
	endif

	jsr	sound_data_address
	lda	[sound_add_low,x]
	pha
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<sound_add_high,x
	pla
	sta	<sound_add_low,x	;新しいアドレス書込み

	rts
;-------------------------------------------------------------------------------
;data_end_sub:
;	ldy	<channel_sel
;	
;	if (ALLOW_BANK_SWITCH)
;	lda	loop_point_bank,y
;	sta	sound_bank,x
;	endif
;	
;	lda	loop_point_table,x
;	sta	<sound_add_low,x	;ループ開始位置書き込み Low
;	inx
;	lda	loop_point_table,x
;	sta	<sound_add_low,x	;ループ開始位置書き込み High
;	rts
;-------------------------------------------------------------------------------
volume_sub:
	lda	effect_flag,x
	ora	#%00000001
	sta	effect_flag,x		;ソフトエンベ有効指定

	lda	temporary
	sta	softenve_sel,x
	asl	a
	tay
	lda	softenve_table,y	;ソフトエンベデータアドレス設定
	sta	soft_add_low,x
	lda	softenve_table+1,y
	sta	soft_add_high,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
lfo_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	lfo_data_set

	lda	effect_flag,x
	and	#%10001111		;LFO無効処理
	sta	effect_flag,x
	jsr	sound_data_address
	rts
lfo_data_set:
	asl	a
	asl	a
	sta	lfo_sel,x

	tay
	ldx	<channel_selx2
	lda	lfo_data,y
	sta	lfo_start_time,x		;ディレイセット
	sta	lfo_start_counter,x
	lda	lfo_data+1,y
	sta	lfo_reverse_time,x		;スピードセット
	sta	lfo_reverse_counter,x
	lda	lfo_data+2,y
	sta	lfo_depth,x			;仮デプスセット(後でwarizan_startにより書き換わる)
;	lda	lfo_data+3,y
;	sta	lfo_harf_time,x
;	sta	lfo_harf_count,x		;1/2カウンタセット

	jsr	warizan_start

	.if PITCH_CORRECTION
		lda	effect_flag,x
		ora	#%00010000		;LFO有効フラグセット
		sta	effect_flag,x
		jsr	lfo_initial_vector
	.else
		lda	<channel_sel		;なぜこの処理を入れているかというと
		sec				;内蔵音源と拡張音源で+-が逆だからである
		sbc	#$05
		bcc	urararara2

		lda	effect_flag,x
		ora	#%00110000
		sta	effect_flag,x
		jmp	ittoke2
urararara2:
		lda	effect_flag,x
		and	#%11011111		;波形－処理
		ora	#%00010000		;LFO有効フラグセット
		sta	effect_flag,x
ittoke2:
	.endif
	jsr	sound_data_address
	rts

	.if PITCH_CORRECTION
; チャンネルによるピッチの方向性
lfo_initial_vector:
	lda	freq_vector_table,x
	bmi	.increasing_function
; 2A03など
.decreasing_function:
	lda	effect_flag,x
	and	#%11011111		;LFOは最初減算
	jmp	.ittoke2
; FDSなど
.increasing_function:
	lda	effect_flag,x
	ora	#%00100000		;LFOは最初加算
.ittoke2:
	sta	effect_flag,x
	rts
	.endif
;-------------------------------------------------------------------------------
detune_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	detune_data_set

	lda	effect_flag,x
	and	#%01111111		;detune無効処理
	sta	effect_flag,x
	jsr	sound_data_address
	rts
detune_data_set:
	tay
	sta	detune_dat,x
	lda	effect_flag,x
	ora	#%10000000		;detune有効処理
	sta	effect_flag,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
pitch_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	pitch_enverope_part
	lda	effect_flag,x
	and	#%11111101
	sta	effect_flag,x
	jsr	sound_data_address
	rts

pitch_enverope_part:
	sta	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_table,y
	sta	pitch_add_low,x
	lda	pitchenve_table+1,y
	sta	pitch_add_high,x
	lda	effect_flag,x
	ora	#%00000010
	sta	effect_flag,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
arpeggio_set_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	cmp	#$ff
	bne	arpeggio_part

	lda	effect_flag,x
	and	#%11110111
	sta	effect_flag,x
	jsr	sound_data_address
	rts

arpeggio_part:
	sta	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_table,y
	sta	arpe_add_low,x
	lda	arpeggio_table+1,y
	sta	arpe_add_high,x

	lda	effect_flag,x
	ora	#%00001000
	sta	effect_flag,x
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
direct_freq_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_freq_low,x		;Low
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_freq_high,x		;High
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x			;Counter
	jsr	sound_data_address
	jsr	effect_init
	rts
;-------------------------------------------------------------------------------
y_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<t0
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	<t1
	jsr	sound_data_address
	lda	[sound_add_low,x]
	ldx	#$00
	sta	[t0,x]
	ldx	<channel_selx2
	jsr	sound_data_address
	rts
;-------------------------------------------------------------------------------
wait_sub:
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x
	jsr	sound_data_address

	rts
;-------------------------------------------------------------------------------


;-------------------------------------------------------------------------------
;effect sub routines
;-------------------------------------------------------------------------------
detune_write_sub:
	lda	effect_flag,x
	and	#%10000000
	bne	detune_part
	rts

detune_part:
	lda	detune_dat,x

; freqにAを加減算する
; Aの$80が立っていなかったらそのまま加算
; Aの$80が立っていたらand $7Fして減算
; input: A
freq_add_mcknumber:
	.if PITCH_CORRECTION
	eor	freq_vector_table,x
	.else
	eor	#0			;set N flag
	.endif
	bmi	detune_minus

; freqにAを加算する
; input: A
detune_plus:
	eor	#0			;set Z flag
	beq	.endo			;プラス0なら終了
	
	ldy	pitch_shift_amount,x
	bne	detune_plus_with_asl	;シフトあり
	
	clc
	adc	sound_freq_low,x
	sta	sound_freq_low,x
	bcs	.mid_plus
.endo:	rts
.mid_plus:
	inc	sound_freq_high,x
	bne	.n106_high_through
	inc	sound_freq_n106,x
.n106_high_through:
	rts

detune_minus:
	and	#%01111111
; freqからAを減算する
; input: A
detune_minus_nomask:
	eor	#0			;set Z flag
	beq	.endo			;プラス0なら終了
	
	ldy	pitch_shift_amount,x
	bne	detune_minus_nomask_with_asl	;シフトあり
	
	sta	<t0
	lda	sound_freq_low,x
	sec
	sbc	<t0
	sta	sound_freq_low,x
	bcc	.mid_minus
.endo:	rts
.mid_minus:
	lda	sound_freq_high,x
	beq	.borrow
.no_borrow:
	dec	sound_freq_high,x
	rts
.borrow:
	dec	sound_freq_high,x
	dec	sound_freq_n106,x
	rts

;---------------------------------
; 何回か左シフトするバージョン
; ここは直接呼び出さず、
; freq_add_mcknumber, detune_plus, detune_minus_nomaskを経由すること
; A = 足し算引き算する値
; Y = シフト量 (0は禁止)
detune_plus_with_asl:
	sta	<t0
	lda	#0
	sta	<t1
	sta	<t2
	beq	freq_add_mcknumber_shift	;always

detune_minus_nomask_with_asl:
	eor	#$ff
	sta	<t0
	inc	<t0
	beq	detune_through		;0
	
	lda	#$ff
	sta	<t1
	sta	<t2

freq_add_mcknumber_shift:
.lp
		asl	<t0
		rol	<t1
		rol	<t2
		dey
		bne	.lp
.add
	clc
	lda	<t0
	adc	sound_freq_low,x
	sta	sound_freq_low,x
	lda	<t1
	adc	sound_freq_high,x
	sta	sound_freq_high,x
	lda	<t2
	adc	sound_freq_n106,x
	sta	sound_freq_n106,x
detune_through:
	rts


;-----------------------------------------------------------------------------
sound_software_enverope:
	jsr	volume_enve_sub
	sta	register_low,x
	ora	register_high,x		;音色データ（上位4bit）と下位4bitで足し算
	ldy	<channel_selx4
	sta	$4000,y			;書き込み～
	jsr	enverope_address	;アドレス一個増やして
	rts				;おしまい

volume_enve_sub:
	ldx	<channel_selx2

	indirect_lda	soft_add_low		;エンベロープデータ読み込み
	cmp	#$ff			;最後かどーか
	beq	return3			;最後ならループ処理へ
	rts

return3:
	lda	softenve_sel,x
	asl	a
	tay
	lda	softenve_lp_table,y
	sta	soft_add_low,x
	lda	softenve_lp_table+1,y
	sta	soft_add_high,x
	jmp	volume_enve_sub
;------------------------------------------------
enverope_address:
	inc	soft_add_low,x
	bne	return5
	inc	soft_add_high,x
return5:
	rts
;-------------------------------------------------------------------------------
sound_duty_enverope:
	ldx	<channel_selx2

	lda	<channel_sel
	cmp	#$02
	beq	return21		;三角波なら飛ばし～

	indirect_lda	duty_add_low		;エンベロープデータ読み込み
	cmp	#$ff			;最後かどーか
	beq	return22		;最後ならそのままおしまい
	pha

	lda	effect2_flags,x         ; hw_envelope
	and	#%00110000
	eor	#%00110000
	sta	register_high,x

	pla
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
;	ora	#%00110000		;hardware envelope & ... disable
	ora	register_high,x		;hw_envelope
	sta	register_high,x

	ora	register_low,x		;音色データ（上位4bit）と下位4bitで足し算
	ldy	<channel_selx4
	sta	$4000,y			;書き込み～
	jsr	duty_enverope_address	;アドレス一個増やして
return21:
	rts				;おしまい

return22:
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	lda	dutyenve_lp_table+1,y
	sta	duty_add_high,x
	jmp	sound_duty_enverope

;--------------------------------------------------
duty_enverope_address:
	inc	duty_add_low,x
	bne	return23
	inc	duty_add_high,x
return23:
	rts
;--------------------------------------	
sound_lfo:
	lda	sound_freq_high,x
	sta	temporary

	jsr	lfo_sub

	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y			;　　現在値をレジスタにセット
	lda	sound_freq_high,x
	cmp	temporary
	beq	end4
	sta	$4003,y
end4:
	rts				;ここまで
;-------------------------------------------------------------------------------
lfo_sub:
	ldx	<channel_selx2
	lda	lfo_start_counter,x
	beq	.lfo_start
	dec	lfo_start_counter,x
	rts

.lfo_start:
	asl	lfo_reverse_time,x	;2倍する(LFOの1/2周期になる)
	lda	lfo_reverse_counter,x	;反転用カウンタ読み込み
	cmp	lfo_reverse_time,x	;LFOの周期の1/2ごとに反転する
	bne	.lfo_depth_set		;規定数に達していなければデプス処理へ
.lfo_revers_set:				;規定数に達していたら方向反転処理
		lda	#$00			;
		sta	lfo_reverse_counter,x	;反転カウンタ初期化
		lda	effect_flag,x		;方向ビットを反転
		eor	#%00100000		;
		sta	effect_flag,x		;

.lfo_depth_set:
	lsr	lfo_reverse_time,x	;1/2にする(LFOの1/4周期になる)
	lda	lfo_adc_sbc_counter,x	;デプス用カウンタ読み込み
	cmp	lfo_adc_sbc_time,x	;lfo_adc_sbc_timeごとにデプス処理する
	bne	.lfo_count_inc		;まだならカウンタプラスへ
.lfo_depth_work:				;一致していればデプス処理
		lda	#$00			;
		sta	lfo_adc_sbc_counter,x	;デプスカウンタ初期化
		lda	effect_flag,x		;＋か－か
		and	#%00100000		;このビットが
		bne	.lfo_depth_plus		;立っていたら加算
.lfo_depth_minus:
			lda	lfo_depth,x
			jsr	detune_minus_nomask
			jmp	.lfo_count_inc
.lfo_depth_plus:
			lda	lfo_depth,x
			jsr	detune_plus

.lfo_count_inc:
	inc	lfo_reverse_counter,x	;カウンタ足してお終い
	inc	lfo_adc_sbc_counter,x
	rts

;-------------------------------------------------------------------------------
warizan_start:
.quotient = t0
.divisor = t1
	lda	#$00
	sta	<.quotient
	lda	lfo_reverse_time,x	;1/4周期と
	cmp	lfo_depth,x		;Y軸ピーク指定
	beq	.plus_one		;同じなら1:1
	bmi	.depth_wari		;Y軸ピークのほうが大きい場合

.revers_wari:				;1/4周期のほうが大きい場合
	lda	lfo_depth,x
	sta	<.divisor
	lda	lfo_reverse_time,x
	jsr	.warizan
	lda	<.quotient
	sta	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	lda	#$01
	sta	lfo_depth,x
	rts

.depth_wari:
	lda	lfo_reverse_time,x
	sta	<.divisor
	lda	lfo_depth,x
	jsr	.warizan
	lda	<.quotient
	sta	lfo_depth,x
	lda	#$01
	sta	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	rts

.plus_one:				;1フレームごとに1
	lda	#$01
	sta	lfo_depth,x
	sta	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	rts

.warizan:
	inc	<.quotient
	sec
	sbc	<.divisor
	beq	.warizan_end
	bcc	.warizan_end
	bcs	.warizan			;always
.warizan_end:
	rts

;-------------------------------------------------------------------------------
sound_pitch_enverope:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub
pitch_write:
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	end3
	sta	$4003,y
end3:
	jsr	pitch_enverope_address
	rts
;-------------------------------------------------------------------------------
pitch_sub:
	ldx	<channel_selx2
	indirect_lda	pitch_add_low	
	cmp	#$ff
	beq	return62

	jmp	freq_add_mcknumber

;--------------------------------------------------
return62:
	indirect_lda	pitch_add_low	
	lda	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_lp_table,y
	sta	pitch_add_low,x
	lda	pitchenve_lp_table+1,y
	sta	pitch_add_high,x
	jmp	pitch_sub
;--------------------------------------------------
pitch_enverope_address:
	inc	pitch_add_low,x
	bne	return63
	inc	pitch_add_high,x
return63:
	rts
;-------------------------------------------------------------------------------
sound_high_speed_arpeggio:		;note enverope
ARPEGGIO_RETRIG = 0			; 1だとsound_freq_highが変化しなくても書き込む
	.if !ARPEGGIO_RETRIG
	lda	sound_freq_high,x
	sta	temporary2
	.endif
	jsr	note_enve_sub
	bcs	.end			; 0なので書かなくてよし
	jsr	frequency_set
;.note_freq_write:
	ldx	<channel_selx2
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y
	lda	sound_freq_high,x
	.if !ARPEGGIO_RETRIG
	cmp	temporary2
	beq	.end
	.endif
	sta	$4003,y
	sta	sound_lasthigh,x
	
.end:
	jsr	arpeggio_address
	rts
;--------------------------------------------------
note_add_set:
	lda	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_lp_table,y
	sta	arpe_add_low,x
	lda	arpeggio_lp_table+1,y
	sta	arpe_add_high,x
	jmp	note_enve_sub
;--------------------------------------------------
arpeggio_address:
	inc	arpe_add_low,x
	bne	return83
	inc	arpe_add_high,x
return83:
	rts
;-------------------------------------------------------------------------------
;Output 
;	C=0(読み込んだ値は0じゃないので発音処理しろ)
;	C=1(読み込んだ値は0なので発音処理しなくていいよ)
;
note_enve_sub:

	ldx	<channel_selx2
	indirect_lda	arpe_add_low		;ノートエンベデータ読み出し
	cmp	#$ff			;$ff（お終い）か？
	beq	note_add_set
	cmp	#$00			;ゼロか？(Zフラグ再セット)
	beq	.note_enve_zero_end	;ゼロならC立ててお終い
	cmp	#$80
	beq	.note_enve_zero_end	;ゼロならC立ててお終い
	bne	.arpeggio_sign_check	;always
.note_enve_zero_end
	sec				;発音処理は不要
	rts
.arpeggio_sign_check
	eor	#0			;N flag確認
	bmi	arpeggio_minus		;－処理へ

arpeggio_plus:
	sta	<t0			;テンポラリに置く（ループ回数）
arpeggio_plus2:
	lda	sound_sel,x		;音階データ読み出し
	and	#$0f			;下位4bit抽出
	cmp	#$0b			;もしbなら
	beq	oct_plus		;オクターブ＋処理へ
	inc	sound_sel,x		;でなければ音階＋１
	jmp	loop_1			;ループ処理１へ
oct_plus:
	lda	sound_sel,x		;音階データ読み出し
	and	#$f0			;上位4bit取り出し＆下位4bitゼロ
	clc
	adc	#$10			;オクターブ＋１
	sta	sound_sel,x		;音階データ書き出し
loop_1:
	dec	<t0			;ループ回数－１
	lda	<t0			;んで読み出し
	beq	note_enve_end		;ゼロならループ処理終わり
	bne	arpeggio_plus2		;でなければまだ続く

arpeggio_minus:
	and	#%01111111
	sta	<t0
arpeggio_minus2:
	lda	sound_sel,x		;音階データ読み出し
	and	#$0f			;下位4bit抽出
	beq	oct_minus		;ゼロなら－処理へ
	dec	sound_sel,x		;でなければ音階－１
	jmp	loop_2			;ループ処理２へ
oct_minus:
	lda	sound_sel,x		;音階データ読み出し
	clc
	adc	#$0b			;+b
	sec
	sbc	#$10			;-10
	sta	sound_sel,x		;音階データ書き出し
loop_2:
	dec	<t0			;ループ回数－１
	lda	<t0			;んで読み出し
	bne	arpeggio_minus2		;ゼロならループ処理終わり
note_enve_end:
	clc				;発音処理は必要
	rts				;
;-------------------------------------------------------------------------------
;oto_setで呼ばれる
effect_init:
;ソフトウェアエンベロープ読み込みアドレス初期化
	lda	softenve_sel,x
	asl	a
	tay
	lda	softenve_table,y
	sta	soft_add_low,x
	lda	softenve_table+1,y
	sta	soft_add_high,x

;ピッチエンベロープ読み込みアドレス初期化
	lda	pitch_sel,x
	asl	a
	tay
	lda	pitchenve_table,y
	sta	pitch_add_low,x
	lda	pitchenve_table+1,y
	sta	pitch_add_high,x

;デューティエンベロープ読み込みアドレス初期化
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x

;ノートエンベロープ読み込みアドレス初期化
	lda	arpeggio_sel,x
	asl	a
	tay
	lda	arpeggio_table,y
	sta	arpe_add_low,x
	lda	arpeggio_table+1,y
	sta	arpe_add_high,x
;ソフトウェアLFO初期化
	lda	lfo_start_time,x
	sta	lfo_start_counter,x
	lda	lfo_adc_sbc_time,x
	sta	lfo_adc_sbc_counter,x
	lda	lfo_reverse_time,x
	sta	lfo_reverse_counter,x

	.if PITCH_CORRECTION
		jsr	lfo_initial_vector
	.else
		lda	<channel_sel
		sec
		sbc	#$04
		bmi	urararara

		lda	effect_flag,x
		and	#%10111111
		ora	#%00100000
		sta	effect_flag,x
		jmp	ittoke
urararara:
		lda	effect_flag,x
		and	#%10011111
		sta	effect_flag,x
ittoke:	
	.endif
;休符フラグクリア&Key Onフラグ書き込み
sound_flag_clear_key_on
	lda	#%00000010
	sta	rest_flag,x
	rts
;
;-------------------------
;
; curfreq = freq
; call frequency_set
; oldfreq = freq
;
; t_note = note
;
; read note
; read count
;
; call frequency_set
;
; if (oldfreq < freq)
;  { nega = 0, diff = freq - oldfreq }
; else
;  { nega = 1, diff = oldfreq - freq }
;
; note = t_note
; freq = curfreq
;
; if (diff < count)
; {
;   step = count / diff
;   addfreq = 1
; }
; else
; {
;   step = 1
;   addfreq = diff / count
; }
;
; if (nega) addfreq = 0 - addfreq
; count = step
;

; proc_addfreq
; if (!step) return
; count--
; if (count) return
; count = step
; freq += addfreq
; return
; 
;
;-------------------------
;pitchshift_setup
;
; t4 = curfreq , t2 = oldfreq
; t6 = t_note
;
; dest : t0,t1
; Note that t0 and t1 might be used in a subroutine
;

pitchshift_setup:
	; curfreq = freq
	ldx	<channel_selx2
	lda	sound_freq_low,x
	sta	<t4
	lda	sound_freq_high,x
	sta	<t5

	jsr	frequency_set
	ldx	<channel_selx2

	; oldfreq = freq
	lda	sound_freq_low,x
	sta	<t2
	lda	sound_freq_high,x
	sta	<t3

	lda	sound_sel,x
	sta	<t6
	
	; read note
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_sel,x
	sta	ps_nextnote,x

	; read count
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	ps_count,x
	sta	sound_counter,x

	jsr	sound_data_address

	lda	sound_sel,x
	cmp	<t6
	bne	ps_calc_freq

	; next = current
	lda	#$00
	sta	ps_step,x
	rts
	

ps_calc_freq:
	jsr	frequency_set
	ldx	<channel_selx2

	; if ( oldfreq < freq ) goto posi else goto nega
	lda	<t3
	cmp	sound_freq_high,x
	bcc	ps_diff_posi  ; (A < B)
	bne	ps_diff_nega  ; (A > B)
	lda	<t2 ; (A == B)
	cmp	sound_freq_low,x
	bcs	ps_diff_nega  ; (A >= B)

ps_diff_posi:
	; nega = 0
	lda	#$00
	sta	<ps_temp
	
	; diff = freq - oldfreq
	sec
	lda	sound_freq_low,x
	sbc	<t2
	sta	<t0

	lda	sound_freq_high,x
	sbc	<t3
	sta	<t1
	jmp	ps_restore_note
ps_diff_nega:
	; nega = 1
	lda	#$01
	sta	<ps_temp
	
	; diff = oldfreq - freq
	sec
	lda	<t2
	sbc	sound_freq_low,x
	sta	<t0

	lda	<t3
	sbc	sound_freq_high,x
	sta	<t1

ps_restore_note:

	; note = t_note
	lda	<t6
	sta	sound_sel,x

	; freq = curfreq
	lda	<t4
	sta	sound_freq_low,x
	lda	<t5
	sta	sound_freq_high,x


	; if ( diff < count ) goto step else goto freq
	lda	<t1
	cmp	#$00
	bcc	ps_step_base  ; (A < B)
	bne	ps_freq_base  ; (A > B)
	lda	<t0 ; (A == B)
	cmp	ps_count,x
	bcs	ps_freq_base  ; (A >= B)

ps_step_base:

	; addfreq = 1
	lda	#$01
	sta	ps_addfreq_l,x
	lda	#$00
	sta	ps_addfreq_h,x

	; step = count / diff
	; t2 = t0
	lda	<t0
	sta	<t2
	lda	<t1
	sta	<t3

	; t0 = count
	lda	ps_count,x
	sta	<t0
	lda	#$00
	sta	<t1
	
	jsr	b_div

	; step = t6
	lda	<t6
	sta	ps_step,x

	jmp	ps_nega_chk
ps_freq_base:
	lda	#$01
	sta	ps_step,x

	; addfreq = diff / count
	lda	ps_count,x
	sta	<t2
	lda	#$00
	sta	<t3

	jsr	b_div

	; addfreq = t6
	lda	<t6
	sta	ps_addfreq_l,x
	lda	<t7
	sta	ps_addfreq_h,x

ps_nega_chk:
	lda	<ps_temp
	beq	ps_posi
	; addfreq = 0 - addfreq

	sec
	lda	#$00
	sbc	ps_addfreq_l,x
	sta	ps_addfreq_l,x

	lda	#$00
	sbc	ps_addfreq_h,x
	sta	ps_addfreq_h,x
	
ps_posi:
	lda	ps_step,x
	sta	ps_count,x

	rts


;--------------------------
;process_ps
;called from effect part
;
process_ps:
	lda	ps_step,x
	beq	process_ps_fin ; if (!step) return
	dec	ps_count,x
	bne	process_ps_fin ; if (count > 0) return
	lda	ps_step,x
	sta	ps_count,x

	lda	sound_freq_high,x
	sta	temporary

	; freq += ps_addfreq
	clc
	lda	sound_freq_low,x
	adc	ps_addfreq_l,x
	sta	sound_freq_low,x

	lda	sound_freq_high,x
	adc	ps_addfreq_h,x
	sta	sound_freq_high,x

	; write to sound regs
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	$4002,y
	lda	sound_freq_high,x

	cmp	temporary
	beq	process_ps_fin
	sta	$4003,y
	sta	sound_lasthigh,x


process_ps_fin:
	rts


; -------------------------
; b_div
; 16bit binary divider
; C = A / B
;
; in   :
;   t0,t1 = dividend(A)  t2,t3 = divider(B) 
; temp :
;   t4,t5 = temp
; out  :
;   t6,t7 = quotient(C)
;
; dest : almost all params
;
b_div:
	lda  #$00
	sta  <t6
	sta  <t7

	lda  #$01
	sta  <t4
	lda  #$00
	sta  <t5

	lda  <t2
	ora  <t3
	bne  b_div_lp1
	rts  ; divided by zero

b_div_lp1:
	lda  <t3
	and  #$80
	bne  b_div_sub
	asl  <t2 ; B <<=1
	rol  <t3

	; if ( A < B ) goto fin_ls else next_ls
	lda	<t1
	cmp	<t3
	bcc	b_div_fin_ls  ; (A < B)
	bne	b_div_next_ls ; (A > B)
	lda	<t0 ; (A == B)
	cmp	<t2
	bcc	b_div_fin_ls ; (A < B)

b_div_next_ls:
	asl  <t4	; temp <<= 1
	rol  <t5
	jmp  b_div_lp1
b_div_fin_ls:
	lsr  <t3 ; B>>=1
	ror  <t2
;	lsr  <t5 ; temp >>= 1
;	ror  <t4 ;
b_div_sub:
	lda  <t0 ; A-=B
	sec
	sbc  <t2
	sta  <t0

	lda  <t1
	sbc  <t3
	sta  <t1

	; C += temp
	clc
	lda  <t6
	adc  <t4
	sta  <t6

	lda  <t7
	adc  <t5
	sta  <t7

b_div_shift:
	; B >>= 1 
	lsr  <t3
	ror  <t2

	; temp >>= 1 
	lsr  <t5
	ror  <t4
	bcs  b_div_fin

	; if ( A(t0) < B(t2) ) goto shift else sub
	lda	<t1		  
	cmp	<t3		  
	bcc	b_div_shift       ; (t0 < t2)
	bne	b_div_sub	  ; (t0 > t2)
	lda	<t0
	cmp	<t2
	bcc	b_div_shift	  ; (t0 < t2)
	jmp	b_div_sub


b_div_fin:
	rts



