MMC5_REG_CTRL	=	$5000	; コントロールレジスタ
MMC5_REG_FREQ_L	=	$5002	; 周波数(L)レジスタ
MMC5_REG_FREQ_H	=	$5003	; 周波数(H)レジスタ
MMC5_START_CH	=	PTRMMC5	; 開始ch
;----------------------------------------


mmc5_sound_init:
	lda	#$03
	sta	$5015
	rts
;-----------------------------------------------------------
sound_mmc5:
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	mmc5_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go
	jsr	sound_mmc5_read
	jsr	mmc5_do_effect
	lda	rest_flag,x
	and	#%00000010		;キーオンフラグ
	beq	.end1			
	jsr	sound_mmc5_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#%11111101		;キーオンフラグオフ
	sta	rest_flag,x
.end1
	rts

;-------
mmc5_do_effect:
	lda	rest_flag,x
	and	#%00000001
	beq	.duty_write2
	rts				;休符なら終わり

.duty_write2:
	lda	effect_flag,x
	and	#%00000100
	beq	.enve_write2
	jsr	sound_mmc5_dutyenve

.enve_write2:
	lda	effect_flag,x
	and	#%00000001
	beq	.lfo_write2
	jsr	sound_mmc5_softenve

.lfo_write2:
	lda	effect_flag,x
	and	#%00010000
	beq	.pitchenve_write2
	jsr	sound_mmc5_lfo

.pitchenve_write2:
	lda	effect_flag,x
	and	#%00000010
	beq	.arpeggio_write2
	jsr	sound_mmc5_pitch_enve

.arpeggio_write2:
	lda	effect_flag,x
	and	#%00001000
	beq	.return7
	lda	rest_flag,x		;キーオンのときとそうでないときでアルペジオの挙動はちがう
	and	#%00000010		;キーオンフラグ
	bne	.arpe_key_on
	jsr	sound_mmc5_note_enve	;キーオンじゃないとき通常はこれ
	jmp	.return7
.arpe_key_on				;キーオンも同時の場合
	jsr	note_enve_sub		;メモリ調整だけで、ここでは書き込みはしない
	jsr	mmc5_freq_set
	jsr	arpeggio_address
.return7:
	rts

;------------------------------------------------
mmc5_freq_set:
	ldx	<channel_selx2
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay

	lda	psg_frequency_table,y	;PSG周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	psg_frequency_table+1,y	;PSG周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み

mmc5_oct_set1:

	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;

	sec				;オクターブ下げる
	sbc	#$02

	beq	mmc5_freq_end		;ゼロならそのまま終わり
	tay

mmc5_oct_set2:

	lsr	sound_freq_high,x	;右シフト　末尾はCへ
	ror	sound_freq_low,x	;Cから持ってくるでよ　右ローテイト
	dey				;
	bne	mmc5_oct_set2		;オクターブ分繰り返す

mmc5_freq_end:
	jsr	detune_write_sub
	rts
;---------------------------------------------------------------
sound_mmc5_read:
	ldx	<channel_selx2
	
	lda	sound_bank,x
	jsr	change_bank
	
	lda	[sound_add_low,x]
;----------
;ループ処理1
mmc5_loop_program
	cmp	#$a0
	bne	mmc5_loop_program2
	jsr	loop_sub
	jmp	sound_mmc5_read
;----------
;ループ処理2(分岐)
mmc5_loop_program2
	cmp	#$a1
	bne	mmc5_bank_command
	jsr	loop_sub2
	jmp	sound_mmc5_read
;----------
;バンク切り替え
mmc5_bank_command
	cmp	#$ee
	bne	mmc5_wave_set
	jsr	data_bank_addr
	jmp	sound_mmc5_read
;----------
;データエンド設定
;mmc5_data_end:
;	cmp	#$ff
;	bne	mmc5_wave_set
;	jsr	data_end_sub
;	jmp	sound_mmc5_read
;----------
;音色設定
mmc5_wave_set:
	cmp	#$fe
	bne	mmc5_volume_set
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音色データ読み出し
	pha
	bpl	mmc5_duty_enverope_part	;ヂューティエンベ処理へ

mmc5_duty_select_part:
	lda	effect_flag,x
	and	#%11111011
	sta	effect_flag,x		;デューティエンベロープ無効指定

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

;;;;	ora	#%00110000		;waveform hold on & hardware envelope off

	ora	register_high,x
	sta	register_high,x		;書き込み
	ora	register_low,x
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y
	jsr	sound_data_address
	jmp	sound_mmc5_read

mmc5_duty_enverope_part:
	lda	effect_flag,x
	ora	#%00000100
	sta	effect_flag,x		;デューティエンベロープ有効指定
	pla
	sta	duty_sel,x
	asl	a
	tay
	lda	dutyenve_table,y	;デューティエンベロープアドレス設定
	sta	duty_add_low,x
	lda	dutyenve_table+1,y
	sta	duty_add_high,x
	jsr	sound_data_address
	jmp	sound_mmc5_read

;----------
;音量設定
mmc5_volume_set:
	cmp	#$fd
	bne	mmc5_rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	bpl	mmc5_softenve_part		;ソフトエンベ処理へ

mmc5_volume_part:
	lda	effect_flag,x
	and	#%11111110
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	temporary
	and	#%00001111
	sta	register_low,x
	ora	register_high,x
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y			;ボリューム書き込み
	jsr	sound_data_address
	jmp	sound_mmc5_read

mmc5_softenve_part:
	jsr	volume_sub
	jmp	sound_mmc5_read

;----------
mmc5_rest_set:
	cmp	#$fc
	bne	mmc5_lfo_set

	lda	rest_flag,x
	ora	#%00000001
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	lda	register_high,x
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y
	jsr	sound_data_address
	rts
;----------
mmc5_lfo_set:
	cmp	#$fb
	bne	mmc5_detune_set
	jsr	lfo_set_sub
	jmp	sound_mmc5_read
;----------
mmc5_detune_set:
	cmp	#$fa
	bne	mmc5_pitch_set
	jsr	detune_sub
	jmp	sound_mmc5_read
;----------
;ピッチエンベロープ設定
mmc5_pitch_set:
	cmp	#$f8
	bne	mmc5_arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_mmc5_read
;----------
;ノートエンベロープ設定
mmc5_arpeggio_set:
	cmp	#$f7
	bne	mmc5_freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_mmc5_read
;----------
;再生周波数直接設定
mmc5_freq_direct_set:
	cmp	#$f6
	bne	mmc5_y_command_set
	jsr	direct_freq_sub
	rts
;----------
;ｙコマンド設定
mmc5_y_command_set:
	cmp	#$f5
	bne	mmc5_wait_set
	jsr	y_sub
	jmp	sound_mmc5_read
;----------
;ウェイト設定
mmc5_wait_set:
	cmp	#$f4
	bne	mmc5_hwenv
	jsr	wait_sub
	rts


;----------
;ハードエンベロープ
mmc5_hwenv:
	cmp	#$f0
	bne	mmc5_slur

	jsr	sound_data_address
	lda	effect2_flags,x
	and	#%11001111
	ora	[sound_add_low,x]
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_mmc5_read


;----------
;スラー
mmc5_slur:
	cmp	#$e9
	bne	mmc5_oto_set
	lda	effect2_flags,x
	ora	#%00000001
	sta	effect2_flags,x
	jsr	sound_data_address
	jmp	sound_mmc5_read

;----------
mmc5_oto_set:
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	mmc5_freq_set		;周波数セットへ

	lda	effect2_flags,x		;スラーフラグのチェック
	and	#%00000001
	beq	no_slur_mmc5

	lda	effect2_flags,x
	and	#%11111110
	sta	effect2_flags,x		;スラーフラグのクリア
	jmp	sound_flag_clear_key_on

no_slur_mmc5:
	jmp	effect_init

;-------------------------------------------------------------------------------
sound_mmc5_write:
	ldx	<channel_selx2
	ldy	<channel_selx4

	lda	register_low,x		;音量保持
	ora	register_high,x
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y

	lda	sound_freq_low,x	;Low Write
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x	;High Write
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
	rts
;-----------------------------------------------------
sound_mmc5_softenve:
	jsr	volume_enve_sub
	sta	register_low,x
	ora	register_high,x		;音色データ（上位4bit）と下位4bitで足し算
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y			;書き込み～
	jsr	enverope_address	;アドレス一個増やして
	rts				;おしまい
;-------------------------------------------------------------------------------
sound_mmc5_lfo:
	lda	sound_freq_high,x
	sta	temporary
	jsr	lfo_sub
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	mmc5_end4
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
mmc5_end4:
	rts
;-------------------------------------------------------------------------------
sound_mmc5_pitch_enve:
	lda	sound_freq_high,x
	sta	temporary
	jsr	pitch_sub
mmc5_pitch_write:
	lda	sound_freq_low,x
	ldy	<channel_selx4
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x
	cmp	temporary
	beq	mmc5_end3
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
mmc5_end3:
	jsr	pitch_enverope_address
	rts
;-------------------------------------------------------------------------------
sound_mmc5_note_enve
;	lda	sound_freq_high,x
;	sta	temporary2
	jsr	note_enve_sub
	bcs	.end4			;0なので書かなくてよし
	jsr	mmc5_freq_set
;.mmc5_note_freq_write:
	ldx	<channel_selx2
	ldy	<channel_selx4

	lda	sound_freq_low,x
	sta	MMC5_REG_FREQ_L-MMC5_START_CH*4,y
	lda	sound_freq_high,x
;	cmp	temporary2
;	beq	.mmc5_end2
	sta	MMC5_REG_FREQ_H-MMC5_START_CH*4,y
;.mmc5_end2:
	jsr	arpeggio_address
	rts
.end4
;	jsr	mmc5_freq_set
	jsr	arpeggio_address
	rts
;-------------------------------------------------------------------------------
sound_mmc5_dutyenve:
	ldx	<channel_selx2

	indirect_lda	duty_add_low		;エンベロープデータ読み込み
	cmp	#$ff			;最後かどーか
	beq	mmc5_return22		;最後ならそのままおしまい
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	asl	a
	ora	#%00110000		;waveform hold on & hardware envelope off
	sta	register_high,x
	ora	register_low,x		;音色データ（上位4bit）と下位4bitで足し算
	ldy	<channel_selx4
	sta	MMC5_REG_CTRL-MMC5_START_CH*4,y			;書き込み～
	jsr	duty_enverope_address	;アドレス一個増やして
	rts				;おしまい

mmc5_return22:
	lda	duty_sel,x
	asl	a
	tay
	lda	dutyenve_lp_table,y
	sta	duty_add_low,x
	lda	dutyenve_lp_table+1,y
	sta	duty_add_high,x
	jmp	sound_mmc5_dutyenve
;-------------------------------------------------------------------------------
