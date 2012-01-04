;Sunsoft 5B (Gimmick!)
;
;Sunsoft 5B supports noise and envelope generator.
;
;C000 :AY 3-8910 address(W)
;E000 :data(W)
;
;Reference: http://www.howell1964.freeserve.co.uk/parts/ay3891x_datasheet.htm
;           http://www.breezer.demon.co.uk/spec/tech/ay-3-8912.html etc...
;
;AY 3-8910 Registers
;00	Ch. A freq data lower bits
;	FFFFFFFF
;01	Ch. A freq data higher bits
;	----FFFF
;	freq=1.79/(F*16) Mhz
;
;	
;02	Ch. B freq data lower bits
;03	Ch. B freq data higher bits
;04	Ch. C freq data lower bits
;05	Ch. C freq data higher bits
;
;
;06	Noise pitch
;	---FFFFF
;	freq=1.79/(F*16) Mhz
;
;
;07	Mixer
;	0:Enable 1:Disable
;	IINNNTTT
;	||||||||
;	|||||||+-------- Ch A Tone
;	||||||+--------- Ch B Tone
;	|||||+---------- Ch C Tone
;	||||+----------- Ch A Noise
;	|||+------------ Ch B Noise
;	||+------------- Ch C Noise
;	|+-------------- 
;	+--------------- 
;
;
;08	Ch. A volume
;	---MVVVV
;	V: Volume
;	M: Mode (1: Use envelope, 0:Use fixed volume)
;
;
;09	Ch. B volume
;0a	Ch. C volume
;
;
;0b	Envelope duration lower bits
;	DDDDDDDD
;0c	Envelope duration higher bits
;	DDDDDDDD
;	freq = 1.79/(D*256) Mhz
;	(duration = D*256/1.79 sec)
;
;
;0d	Envelope shape
;	----CAAH
;	    ||||
;	    |||+-------- Hold
;	    ||+--------- Alternate
;	    |+---------- Atack
;	    +----------- Continue
;
;
;0e	
;0f	

FME7_ADDR	=	$C000
FME7_DATA	=	$E000

;----------------------------------------
fme7_sound_init:
	ldy	#$0A			;Volumeを0に
	lda	#$00
.loop:
	sty	FME7_ADDR
	sta	FME7_DATA
	dey
	cpy	#$07
	bne	.loop
	lda	#%11111000		;ToneをEnableに
	sty	FME7_ADDR
	sta	FME7_DATA
	sta	fme7_reg7
	rts

;----------------------------------------
fme7_dst_adr_set:
	lda	<channel_sel
	sec				
	sbc	#PTRFME7		;FME7の何チャンネル目か？
	sta	fme7_ch_sel
	asl	a
	sta	fme7_ch_selx2
	asl	a
	sta	fme7_ch_selx4
	lda	fme7_ch_sel
	clc
	adc	#$08
	sta	fme7_vol_regno
	rts

;-----------------------------------------------------------
sound_fme7:
	lda	<channel_sel
	cmp	#PTRFME7+3
	beq	.end1
	jsr	fme7_dst_adr_set
	ldx	<channel_selx2
	dec	sound_counter,x		;カウンタいっこ減らし
	beq	.sound_read_go		;ゼロならサウンド読み込み
	jsr	fme7_do_effect		;ゼロ以外ならエフェクトして
	rts				;おわり
.sound_read_go
	jsr	sound_fme7_read
	jsr	fme7_do_effect
	lda	rest_flag,x
	and	#%00000010		;キーオンフラグ
	beq	.end1			
	jsr	sound_fme7_write	;立っていたらデータ書き出し
	lda	rest_flag,x
	and	#%11111101		;キーオンフラグオフ
	sta	rest_flag,x
.end1
	rts

;-------
fme7_do_effect:
	lda	rest_flag,x
	and	#%00000001
	beq	.duty_write2
	rts				;休符なら終わり

.duty_write2:

.enve_write2:
	lda	effect_flag,x
	and	#%00000001
	beq	.lfo_write2
	jsr	sound_fme7_softenve

.lfo_write2:
	lda	effect_flag,x
	and	#%00010000
	beq	.pitchenve_write2
	jsr	sound_fme7_lfo

.pitchenve_write2:
	lda	effect_flag,x
	and	#%00000010
	beq	.arpeggio_write2
	jsr	sound_fme7_pitch_enve

.arpeggio_write2:
	lda	effect_flag,x
	and	#%00001000
	beq	.return7
	lda	rest_flag,x		;キーオンのときとそうでないときでアルペジオの挙動はちがう
	and	#%00000010		;キーオンフラグ
	bne	.arpe_key_on
	jsr	sound_fme7_note_enve	;キーオンじゃないとき通常はこれ
	jmp	.return7
.arpe_key_on				;キーオンも同時の場合
	jsr	note_enve_sub		;メモリ調整だけで、ここでは書き込みはしない
	jsr	fme7_freq_set
	jsr	arpeggio_address
.return7:
	rts
;------------------------------------------------
fme7_freq_set:
	ldx	<channel_selx2
	lda	fme7_tone,x
	cmp	#$02			;ノイズなら
	beq	fme7_noise_freq_set	;飛ぶ
	lda	sound_sel,x		;音階データ読み出し
	and	#%00001111		;下位4bitを取り出して
	asl	a
	tay

	lda	fme7_frequency_table,y	;Sun5B周波数テーブルからLowを読み出す
	sta	sound_freq_low,x	;書き込み
	lda	fme7_frequency_table+1,y	;Sun5B周波数テーブルからHighを読み出す
	sta	sound_freq_high,x	;書き込み

	lda	sound_sel,x		;音階データ読み出し
	lsr	a			;上位4bitを取り出し
	lsr	a			;
	lsr	a			;
	lsr	a			;
	beq	fme7_freq_end		;ゼロならそのまま終わり
	tay

fme7_oct_set2:

	lsr	sound_freq_high,x	;右シフト　末尾はCへ
	ror	sound_freq_low,x	;Cから持ってくるでよ　右ローテイト
	dey				;
	bne	fme7_oct_set2		;オクターブ分繰り返す

FREQ_ROUND = 0
	.if	FREQ_ROUND
	lda	sound_freq_low,x
	adc	#$00			;最後に切り捨てたCを足す(四捨五入)
	sta	sound_freq_low,x
	bcc	fme7_freq_end
	inc	sound_freq_high,x
	.endif

fme7_freq_end:
	jsr	detune_write_sub
	rts

;----
fme7_noise_freq_set:
	lda	sound_sel,x		;音階データ読み出し
	sta	sound_freq_low,x	;そのまま
	rts
;------------------------------------------------
fme7_frequency_table:
	dw	$0D5C,$0C9D,$0BE7,$0B3C	; o0c  c+ d  d+
	dw	$0A9B,$0A02,$0973,$08EB	;   e  f  f+ g
	dw	$086B,$07F2,$0780,$0714	;   g+ a  a+ b
	dw	$0000,$0FE4,$0EFF,$0E28	; o-1  a  a+ b
; 再生周波数 = 1789772.5 / (n*32) [Hz]

;------------------------------------------------
sound_fme7_read:
	ldx	<channel_selx2
	
	lda	sound_bank,x
	jsr	change_bank
	
	lda	[sound_add_low,x]
;----------
;ループ処理1
fme7_loop_program:
	cmp	#$a0
	bne	fme7_loop_program2
	jsr	loop_sub
	jmp	sound_fme7_read
;----------
;ループ処理2(分岐)
fme7_loop_program2:
	cmp	#$a1
	bne	fme7_bank_command
	jsr	loop_sub2
	jmp	sound_fme7_read
;----------
;バンク切り替え
fme7_bank_command:
	cmp	#$ee
	bne	fme7_wave_set
	jsr	data_bank_addr
	jmp	sound_fme7_read
;----------
;データエンド設定
;fme7_data_end:
;	cmp	#$ff
;	bne	fme7_wave_set
;	jsr	data_end_sub
;	jmp	sound_fme7_read
;----------
;音色設定
fme7_wave_set:
	cmp	#$fe
	bne	fme7_volume_set

	jsr	sound_data_address
	ldy	fme7_ch_selx4
	lda	fme7_enable_bit_tbl,y	;まずノイズビット、トーンビットの両方を0にする
	eor	#$FF
	and	fme7_reg7
	sta	fme7_reg7
	lda	[sound_add_low,x]	;音色データ読み出し
	and	#%00000011
	sta	fme7_tone,x
	clc
	adc	fme7_ch_selx4
	tay
	lda	fme7_enable_bit_tbl,y	;ビット読み出し
	ora	fme7_reg7		;1のを1にする(Disable)
	sta	fme7_reg7		;
	ldy	#$07
	sty	FME7_ADDR
	sta	FME7_DATA
	jsr	sound_data_address
	jmp	sound_fme7_read

fme7_enable_bit_tbl:
;		       @0   @1(tone)  @2(noise)   @3(both)
	db	%00001001, %00001000, %00000001, %00000000	; ch A
	db	%00010010, %00010000, %00000010, %00000000	; ch B
	db	%00100100, %00100000, %00000100, %00000000	; ch C
;----------
;音量設定
fme7_volume_set:
	cmp	#$fd
	bne	fme7_rest_set
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	temporary
	bpl	fme7_softenve_part	;ソフトエンベ処理へ

fme7_volume_part:
	lda	effect_flag,x
	and	#%11111110
	sta	effect_flag,x		;ソフトエンベ無効指定

	lda	temporary
	and	#%00011111		;bit 4はハードエンベロープフラグ
	sta	fme7_volume,x
	tay
	jsr	fme7_volume_write_sub

	jsr	sound_data_address
	jmp	sound_fme7_read

fme7_softenve_part:
	jsr	volume_sub
	jmp	sound_fme7_read
;----------
fme7_rest_set:
	cmp	#$fc
	bne	fme7_lfo_set

	lda	rest_flag,x
	ora	#%00000001
	sta	rest_flag,x

	jsr	sound_data_address
	lda	[sound_add_low,x]
	sta	sound_counter,x

	ldy	#$00			;ボリューム0を書き込み
	jsr	fme7_volume_write_sub

	jsr	sound_data_address
	rts
;----------
fme7_lfo_set:
	cmp	#$fb
	bne	fme7_detune_set
	jsr	lfo_set_sub
	jmp	sound_fme7_read
;----------
fme7_detune_set:
	cmp	#$fa
	bne	fme7_pitch_set
	jsr	detune_sub
	jmp	sound_fme7_read
;----------
;ピッチエンベロープ設定
fme7_pitch_set:
	cmp	#$f8
	bne	fme7_arpeggio_set
	jsr	pitch_set_sub
	jmp	sound_fme7_read
;----------
;ノートエンベロープ設定
fme7_arpeggio_set:
	cmp	#$f7
	bne	fme7_freq_direct_set
	jsr	arpeggio_set_sub
	jmp	sound_fme7_read
;----------
;再生周波数直接設定
fme7_freq_direct_set:
	cmp	#$f6
	bne	fme7_y_command_set
	jsr	direct_freq_sub
	rts
;----------
;ｙコマンド設定
fme7_y_command_set:
	cmp	#$f5
	bne	fme7_wait_set
	jsr	y_sub
	jmp	sound_fme7_read
;----------
;ウェイト設定
fme7_wait_set:
	cmp	#$f4
	bne	fme7_hard_speed_set
	jsr	wait_sub
	rts
;----------
;ハードウェアエンベロープ速度設定
fme7_hard_speed_set:
	cmp	#$f2
	bne	fme7_noise_set
	jsr	sound_data_address
	ldy	#$0B
	lda	[sound_add_low,x]
	sty	FME7_ADDR
	sta	FME7_DATA
	iny
	jsr	sound_data_address
	lda	[sound_add_low,x]
	sty	FME7_ADDR
	sta	FME7_DATA
	jsr	sound_data_address
	jmp	sound_fme7_read
;----------
;ノイズ周波数設定
fme7_noise_set:
	cmp	#$f1
	bne	fme7_oto_set
	jsr	sound_data_address
	ldy	#$06
	lda	[sound_add_low,x]
	sty	FME7_ADDR
	sta	FME7_DATA
	jsr	sound_data_address
	jmp	sound_fme7_read
;----------
fme7_oto_set:
	sta	sound_sel,x		;処理はまた後で
	jsr	sound_data_address
	lda	[sound_add_low,x]	;音長読み出し
	sta	sound_counter,x		;実際のカウント値となります
	jsr	sound_data_address
	jsr	fme7_freq_set		;周波数セットへ
	jsr	effect_init
	rts
;-------------------------------------------------------------------------------
sound_fme7_write:
	ldy	fme7_volume,x
	jsr	fme7_volume_write_sub

fme7_write:
	lda	fme7_tone,x
	cmp	#$02
	beq	fme7_noise_write	;ノイズ時はノイズ周波数を出力

	ldy	fme7_ch_selx2		;周波数レジスタ番号

	lda	sound_freq_low,x
	sty	FME7_ADDR
	sta	FME7_DATA
	iny
	lda	sound_freq_high,x
	sty	FME7_ADDR
	sta	FME7_DATA
	rts

fme7_noise_write:
	ldy	#$06
	lda	sound_freq_low,x
	and	#%00011111
	sty	FME7_ADDR
	sta	FME7_DATA
	rts
;----------------------------------------
;ボリューム書き込み、エンベロープシェイプ
; input: Y
fme7_volume_write_sub:
	cpy	#$10
	bcc	.write_vol
	tya				;ハードエンベロープのときは
	and	#%00001111
	ldy	#$0D			;エンベロープシェイプも
	sty	FME7_ADDR
	sta	FME7_DATA
	ldy	#$10
.write_vol:				;通常ボリュームのときはここだけ
	lda	fme7_vol_regno
	sta	FME7_ADDR
	sty	FME7_DATA
	rts
;-----------------------------------------------------
sound_fme7_softenve:
	jsr	volume_enve_sub
	sta	fme7_volume,x
	tay
	jsr	fme7_volume_write_sub
	jmp	enverope_address
;-------------------------------------------------------------------------------
sound_fme7_lfo:
	jsr	lfo_sub
	jmp	fme7_write
;-------------------------------------------------------------------------------
sound_fme7_pitch_enve:
	jsr	pitch_sub
	jsr	fme7_write
	jmp	pitch_enverope_address
;-------------------------------------------------------------------------------
sound_fme7_note_enve
	jsr	note_enve_sub
	bcs	.end4			;0なので書かなくてよし
	jsr	fme7_freq_set
	jsr	fme7_write
.end4
	jmp	arpeggio_address
;-------------------------------------------------------------------------------
