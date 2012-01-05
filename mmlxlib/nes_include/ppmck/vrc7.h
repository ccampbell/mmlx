VRC7_ADRS = $9010
VRC7_DATA = $9030

FNUM_LOW = $10
FNUM_HI  = $20
INST_VOL = $30

sound_vrc7:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	vrc7_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go
	jsr	sound_vrc7_read
	jsr	vrc7_do_effect
	lda	rest_flag,x
	and	#%00000010		;キーオンフラグ
	beq	.end1			
	jsr	sound_vrc7_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#%11111101		;キーオンフラグオフ
	sta	rest_flag,x
.end1
	rts

;-------
vrc7_do_effect:
	lda	rest_flag,x
	and	#%00000001
	beq	.duty_write2
	rts				;休符なら終わり

.duty_write2:

.enve_write2:
	lda	effect_flag,x
	and	#%00000001
	beq	.lfo_write2
	jsr	sound_vrc7_softenve

.lfo_write2:
	lda	effect_flag,x
	and	#%00010000
	beq	.pitchenve_write2
	jsr	sound_vrc7_lfo

.pitchenve_write2:
	lda	effect_flag,x
	and	#%00000010
	beq	.arpeggio_write2
	jsr	sound_vrc7_pitch_enve

.arpeggio_write2:
	lda	effect_flag,x
	and	#%00001000
	beq	.return7
	lda	rest_flag,x		;キーオンのときとそうでないときでアルペジオの挙動はちがう
	and	#%00000010		;キーオンフラグ
	bne	.arpe_key_on
	jsr	sound_vrc7_note_enve	;キーオンじゃないとき通常はこれ
	jmp	.return7
.arpe_key_on				;キーオンも同時の場合
	jsr	note_enve_sub		;メモリ調整だけで、ここでは書き込みはしない
	jsr	vrc7_freq_set
	jsr	arpeggio_address
.return7:
	rts
;------------------------------------------------
vrc7_freq_set: ; 2004-0426 VRC7
	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して

	asl	a
	tay

	lda	vrc7_freq_table,y	;vrc7周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	vrc7_freq_table+1,y	;vrc7周波数テーブルからMidleを読み出す
	sta	sound_freq_high,x	;書き込み

vrc7_oct_set1:

	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	sta	temporary
	lda	vrc7_key_stat,x
	and	#%11110001
	ora	#%00010000

	sta	vrc7_key_stat,x
	lda	temporary
	asl	a
	ora	vrc7_key_stat,x
	sta	vrc7_key_stat,x

; 2004/06/15 improve too strong detune.

	jsr	detune_write_sub
	rts

;------------------
vrc7_freq_table:
	dw	$00AC	; o4 :   C : 3
	dw	$00B6	; o4 :  C# : 4
	dw	$00C1	; o4 :   D : 5
	dw	$00CD	; o4 :  D# : 6
	dw	$00D9	; o4 :   E : 7
	dw	$00E6	; o4 :   F : 8
	dw	$00F3	; o4 :  F# : 9
	dw	$0102	; o4 :   G : 10
	dw	$0111	; o4 :  G# : 11
	dw	$0121	; o4 :   A : 12
	dw	$0133	; o4 :  A# : 13
	dw	$0145	; o4 :   B : 14

;---------------------------------------------------------------
sound_vrc7_read:
	ldx	<channel_selx2

	lda	sound_bank,x
	jsr	change_bank

	lda	[sound_add_low,x]
;----------
;ループ処理1
vrc7_loop_program
	cmp	#$a0
	bne	vrc7_loop_program2
	jsr	loop_sub
	jmp	sound_vrc7_read
;----------
;ループ処理2(分岐)
vrc7_loop_program2
	cmp	#$a1
	bne	vrc7_bank_set
	jsr	loop_sub2
	jmp	sound_vrc7_read
;----------
;バンクを切り替えます～
vrc7_bank_set
	cmp	#$ee
	bne	vrc7_wave_set
	jsr	data_bank_addr
	jmp	sound_vrc7_read
;----------
;データエンド設定
;vrc7_data_end:
;	cmp	#$ff
;	bne	vrc7_wave_set
;	jsr	data_end_sub
;	jmp	sound_vrc7_read
;----------
;音色設定
vrc7_wave_set:
	cmp	#$fe
	bne	vrc7_volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]

	sta	temporary
	and	#%01000000		;if over 64, set user tone data
	cmp	#%01000000
	bne	vrc7_set_tone

	lda	temporary
	and	#$3F
	
	asl	a
	tax

	lda	vrc7_data_table,x
	sta	<temp_data_add
	inx
	lda	vrc7_data_table,x
	sta	<temp_data_add+1

	ldy	#$8
	sty	temporary

	ldy	#$00
loop_set_fmdata:
	sty	VRC7_ADRS
	jsr	vrc7_write_reg_wait2
	lda	[temp_data_add],y
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait

	iny
	cpy	temporary
	bmi	loop_set_fmdata

	jmp	end_tone_set

vrc7_set_tone:
	lda	temporary

	asl	a
	asl	a
	asl	a
	asl	a

	sta	temporary
	lda	vrc7_volume,x
	and	#%00001111
	ora	temporary
	sta	vrc7_volume,x

	lda	#INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	eor	#$0f
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait
end_tone_set:
	ldx	<channel_selx2
	jsr	sound_data_address
	jmp	sound_vrc7_read
;----------
;音量設定
vrc7_volume_set:
	cmp	#$fd
	bne	vrc7_rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]

	sta	temporary
	bpl	vrc7_softenve_part	;ソフトエンベ処理へ

vrc7_volume_part:
	lda	effect_flag,x
	and	#%11111110
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	temporary
	and	#%00001111
	sta	temporary

	lda	vrc7_volume,x
	and	#%11110000
	ora	temporary
	sta	vrc7_volume,x

	lda	#INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	eor	#$0f
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait

	jsr	sound_data_address
	jmp	sound_vrc7_read

vrc7_softenve_part:
	jsr	volume_sub
	jmp	sound_vrc7_read
;----------
vrc7_rest_set:
	cmp	#$fc
	bne	vrc7_lfo_set

	lda	rest_flag,x
	ora	#%00000001
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	jsr	vrc7_key_off

	jsr	sound_data_address
	rts
;-------------
vrc7_key_off
	lda	vrc7_key_stat,x
	and	#%11101111
	sta	temporary

	lda	#FNUM_HI
	jsr	vrc7_adrs_ch
	lda	sound_freq_high,x
	ora	temporary
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait

	rts

;----------
vrc7_lfo_set:
	cmp	#$fb
	bne	vrc7_detune_set
	jsr	lfo_set_sub
	jmp	sound_vrc7_read
;----------
vrc7_detune_set:
	cmp	#$fa
	bne	vrc7_pitch_set
	jsr	detune_sub
	jmp	sound_vrc7_read
;----------
;ピッチエンベロープ設定
vrc7_pitch_set:
	cmp	#$f8
	bne	vrc7_arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_vrc7_read
;----------
;ノートエンベロープ設定
vrc7_arpeggio_set:
	cmp	#$f7
	bne	vrc7_freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_vrc7_read
;----------
;再生周波数直接設定
vrc7_freq_direct_set:
	cmp	#$f6
	bne	vrc7_y_command_set
	jsr	direct_freq_sub
	rts
;----------
;ｙコマンド設定
vrc7_y_command_set:
	cmp	#$f5
	bne	vrc7_wait_set
	jsr	y_sub
	jmp	sound_vrc7_read
;----------
;ウェイト設定
vrc7_wait_set:
	cmp	#$f4
	bne	vrc7_slur
	jsr	wait_sub
	rts
;----------
;スラー
vrc7_slur:
	cmp	#$e9
	bne	vrc7_oto_set
	lda	effect2_flags,x
	ora	#%00000001
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_vrc7_read


;----------
vrc7_oto_set:
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	vrc7_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#%00000001
	beq	no_slur_vrc7

	lda	effect2_flags,x
	and	#%11111110
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

no_slur_vrc7:
;volume
	lda	#INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	eor	#$0f
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait
	jsr	vrc7_key_off
	jsr	effect_init
	rts
;-------------------------------------------------------------------------------
sound_vrc7_write:
	ldx	<channel_selx2

	lda	#FNUM_LOW
	jsr	vrc7_adrs_ch

	lda	sound_freq_low,x
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait

	lda	#FNUM_HI
	jsr	vrc7_adrs_ch
	lda	sound_freq_high,x
	ora	vrc7_key_stat,x
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait

	rts
;-------------------------------------------------------------------------------
vrc7_adrs_ch
	sta	<t0
	lda	<channel_sel
	sec
	sbc	#PTRVRC7
	ora	<t0
	sta	VRC7_ADRS
	nop
	nop
	nop
	rts

vrc7_write_reg_wait:
	pha
	lda	#$01
.wait:
	asl	a
	bcc	.wait
	pla
vrc7_write_reg_wait2:
	rts
;-----------------------------------------------------
sound_vrc7_softenve:
	jsr	volume_enve_sub
	sta	temporary

	lda	#INST_VOL
	jsr	vrc7_adrs_ch
	lda	vrc7_volume,x
	and	#%11110000
	ora	temporary
	eor	#$0f
	sta	VRC7_DATA
	jsr	vrc7_write_reg_wait
	jmp	enverope_address
;-------------------------------------------------------------------------------
sound_vrc7_lfo:
	jsr	lfo_sub
	jmp	sound_vrc7_write
;-------------------------------------------------------------------------------
sound_vrc7_pitch_enve:
	jsr	pitch_sub
	jsr	sound_vrc7_write
	jmp	pitch_enverope_address
;-------------------------------------------------------------------------------
sound_vrc7_note_enve
	jsr	note_enve_sub
	bcs	.end4			;0なので書かなくてよし
	jsr	vrc7_freq_set
	jsr	sound_vrc7_write
	jsr	arpeggio_address
	rts
.end4
;	jsr	vrc7_freq_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
