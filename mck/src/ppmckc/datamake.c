#include	<stddef.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"mckc.h"

extern char	*mml_names[MML_MAX];
int		mml_idx = 0;
extern	int	mml_num;
char	songlabel[64];
extern int	debug_flag;
extern char ef_name[256];
extern char inc_name[256];
extern char out_name[256];
extern int	warning_flag;
extern int	message_flag;
extern int	include_flag;


extern int	getFileSize( char *ptr );
extern int  Asc2Int( char *ptr, int *cnt );
extern void strupper( char *ptr );
extern char *readTextFile( char *filename );
extern FILE *openDmc(char *name);
extern char *skipSpaceOld( char *ptr );
extern char *skipSpace( char *ptr );

extern char *skipQuote( char *ptr );
extern char *skipComment( char *ptr );
extern int  isComment( char *ptr );


void	putBankOrigin(FILE *fp, int bank);
int checkBankRange(int bank);
int double2int(double d);
#define arraysizeof(x) ( sizeof(x) / sizeof(x[0]) )


int		error_flag;					// エラーが発生していると0以外に
int		octave;						// 変換中のオクターブ
double	length;						// 変換中の音長
int		octave_flag = 0;			// オクターブスイッチ ("<" ">" の処理)
int		gate_denom = 8;				//qコマンドの分母
int		pitch_correction = 0;			//拡張音源のディチューン、ピッチエンベロープ、LFOの方向修正

int		loop_flag;					// チャンネルループがあると0以外に
int		putAsm_pos;					//

char		*mml_file_name;				//現在のmmlファイル名(アセンブラ出力時に使用)
int		mml_line_pos;				//
int		mml_trk;				//

int		nest;						// リピートのネスト数
LEN		track_count[MML_MAX][_TRACK_MAX][2];			// 音長トータル保管場所(音長/フレーム/ループ音長/ループフレーム)

int		volume_flag;				// 音量の状態
double		tbase = 0.625;						// 変換中の[frame/count]割合

int		transpose;					// 現在のトランスポーズ値

int		sndgen_flag = 0;			// 拡張音源フラグ
// トラック許可フラグ
unsigned long	track_allow_flag = (TRACK(0)|TRACK(1)|TRACK(2)|NOISETRACK|DPCMTRACK);
//実際に使ったトラック
unsigned long	actual_track_flag = 0;
int		dpcm_track_num = 1;			// DPCMトラック
int		fds_track_num = 0;			// FDSトラック
int		vrc7_track_num = 0;			// VRC7トラック
int		vrc6_track_num = 0;			// VRC6トラック
int		n106_track_num = 0;			// 拡張音源(namco106)使用トラック数
int		fme7_track_num = 0;			// FME7トラック
int		mmc5_track_num = 0;			// MMC5トラック


int		bank_sel[_TRACK_MAX];	// 0 ～ 127 = バンク切り替え , 0xFF = 変更無し
int		allow_bankswitching = 1;
int		dpcm_bankswitch = 0;
int		auto_bankswitch = 0;
int		curr_bank = 0x00;
int		bank_usage[128];		//bank_usage[0]は今のところ無意味
int		bank_maximum = 0;		//8KB
int		dpcm_extra_bank_num = 0;	//8KB

int tone_tbl[          _TONE_MAX][1024];	// Tone
int envelope_tbl[  _ENVELOPE_MAX][1024];	// Envelope
int pitch_env_tbl[_PITCH_ENV_MAX][1024];	// Pitch Envelope
int pitch_mod_tbl[_PITCH_MOD_MAX][   5];	// LFO
int arpeggio_tbl[  _ARPEGGIO_MAX][1024];	// Arpeggio
int fm_tone_tbl[    _FM_TONE_MAX][2+64];	// FM Tone
int vrc7_tone_tbl[_VRC7_TONE_MAX][2+64];	// VRC7 Tone(配列数は使用関数の関係)
int n106_tone_tbl[_N106_TONE_MAX][2+64];	// NAMCO106 Tone
int hard_effect_tbl[_HARD_EFFECT_MAX][5];	// FDS Hardware Effect
int effect_wave_tbl[_EFFECT_WAVE_MAX][33];	// Effect Wave (4088) Data

DPCMTBL	dpcm_tbl[_DPCM_MAX];				// DPCM
unsigned char	*dpcm_data;	// DPCM展開データ
int	dpcm_size = 0;
int	dpcm_reststop = 0;

char	song_name[1024] = "Song Name\0";
char	composer[1024] = "Artist\0";
char	maker[1024] = "Maker\0";
char	programer_buf[1024] = "";
char	*programer = NULL;

const	char	str_track[] = _TRACK_STR;

// エラー番号
enum {
	COMMAND_NOT_DEFINED = 0,
	DATA_ENDED_BY_LOOP_DEPTH_EXCEPT_0,
	DEFINITION_IS_WRONG,
	TONE_DEFINITION_IS_WRONG,
	ENVELOPE_DEFINITION_IS_WRONG,
	PITCH_ENVELOPE_DEFINITION_IS_WRONG,
	NOTE_ENVELOPE_DEFINITION_IS_WRONG,
	LFO_DEFINITION_IS_WRONG,
	DPCM_DEFINITION_IS_WRONG,
	DPCM_PARAMETER_IS_LACKING,
	FM_TONE_DEFINITION_IS_WRONG,
	ABNORMAL_PARAMETERS_OF_FM_TONE,
	N106_TONE_DEFINITION_IS_WRONG,
	ABNORMAL_PARAMETERS_OF_N106_TONE,
	ABNORMAL_VALUE_OF_REPEAT_COUNT,
	ABNORMAL_TONE_NUMBER,
	ABNORMAL_ENVELOPE_NUMBER,
	ABNORMAL_ENVELOPE_VALUE,
	ABNORMAL_PITCH_ENVELOPE_NUMBER,
	ABNORMAL_NOTE_ENVELOPE_NUMBER,
	ABNORMAL_LFO_NUMBER,
	ABNORMAL_PITCH_VALUE,
	ABNORMAL_VOLUME_VALUE,
	ABNORMAL_TEMPO_VALUE,
	ABNORMAL_QUANTIZE_VALUE,
	ABNORMAL_SHUFFLE_QUANTIZE_VALUE,
	ABNORMAL_SWEEP_VALUE,
	ABNORMAL_DETUNE_VALUE,
	ABNORMAL_SHIFT_AMOUNT,
	ABNORMAL_NOTE_AFTER_COMMAND,
	RELATIVE_VOLUME_WAS_USED_WITHOUT_SPECIFYING_VOLUME,
	VOLUME_RANGE_OVER_OF_RELATIVE_VOLUME,
	VOLUME_RANGE_UNDER_OF_RELATIVE_VOLUME,
	DATA_ENDED_BY_CONTINUATION_NOTE,
	DPCM_FILE_NOT_FOUND,
	DPCM_FILE_SIZE_OVER,
	DPCM_FILE_TOTAL_SIZE_OVER,
	INVALID_TRACK_HEADER,
	HARD_EFFECT_DEFINITION_IS_WRONG,
	EFFECT_WAVE_DEFINITION_IS_WRONG,
	ABNORMAL_HARD_EFFECT_NUMBER,
	ABNORMAL_TRANSPOSE_VALUE,
	TUPLET_BRACE_EMPTY,
	BANK_IDX_OUT_OF_RANGE,
	FRAME_LENGTH_LESSTHAN_0,
	ABNORMAL_NOTE_LENGTH_VALUE,
	PARAMETER_IS_LACKING,
	ABNORMAL_SELFDELAY_VALUE,
	CANT_USE_BANK_2_OR_3_WITH_DPCMBANKSWITCH,
	CANT_USE_SHIFT_AMOUNT_WITHOUT_PITCH_CORRECTION,
	UNUSE_COMMAND_IN_THIS_TRACK,
};

// エラー文字列
const	char	*ErrorlMessage[] = {
	"指定のコマンドはありません",							"Command not defined",
	"ループ深度が0以外でデータが終了しました",				"Data ended by loop depth except 0",
	"設定に誤りがあります",									"Definition is wrong",
	"PSG音色設定に誤りがあります",							"PSG Tone definition is wrong",
	"エンベロープ設定に誤りがあります",						"Envelope definition is wrong",
	"ピッチエンベロープ設定に誤りがあります",				"Pitch envelope definition is wrong",
	"ノートエンベロープ設定に誤りがあります",				"Note envelope definition is wrong",
	"LFO設定に誤りがあります",								"LFO definition is wrong",
	"DPCM設定に誤りがあります",								"DPCM definition is wrong",
	"DPCM設定のパラメータが足りません",						"DPCM parameter is lacking",
	"FM音色設定に誤りがあります",							"FM tone definition is wrong",
	"FM用音色のパラメータが異常です",						"Abnormal parameters of FM tone",
	"namco106音色設定に誤りがあります",						"namco106 tone definition is wrong",
	"namco106用音色のパラメータが異常です",					"Abnormal parameters of namco106 tone",
	"繰り返し回数の値が異常です",							"Abnormal value of repeat count",
	"音色番号が異常です",									"Abnormal tone number",
	"エンベロープ番号が異常です",							"Abnormal envelope number",
	"エンベロープの値が異常です",							"Abnormal envelope value",
	"ピッチエンベロープ番号の値が異常です",					"Abnormal pitch envelope number",
	"ノートエンベロープ番号の値が異常です",					"Abnormal note envelope number",
	"LFO番号の値が異常です",								"Abnormal LFO number",
	"音程の値が異常です",									"Abnormal pitch value",
	"音量の値が異常です",									"Abnormal volume value",
	"テンポの値が異常です",									"Abnormal tempo value",
	"クォンタイズの値が異常です",							"Abnormal quantize value",
	"シャッフルクォンタイズの値が異常です",							"Abnormal shuffle quantize value",
	"スイープの値が異常です",								"Abnormal sweep value",
	"ディチューンの値が異常です",							"Abnormal detune value",
	"ピッチシフト量の値が異常です",							"Abnormal pitch shift amount value",
	"コマンド後のノートが異常です",							"Abnormal note after command",
	"音量が指定されていない状態で相対音量を使用しました",	"Relative volume was used without specifying volume",
	"相対音量(+)で音量の範囲を超えました",					"Volume range over(+) of relative volume",
	"相対音量(-)で音量の範囲を超えました",					"Volume range under(-) of relative volume",
	"連符処理の途中でデータが終了しました",					"Data ended by Continuation note",
	"DPCMファイルがありません",								"DPCM file not found",
	"DPCMデータのサイズが4081byteを超えました",				"DPCM file size over",
	"DPCMデータのサイズが規定のサイズを超えました",			"DPCM file total size over",
	"指定のトラックヘッダは無効です",						"Invalid track header",
	"ハードウェアエフェクト設定に誤りがあります。",			"Hardware effect definition is wrong",
	"エフェクト波形設定に誤りがあります。",					"Effect wavetable definition is wrong",
	"ハードウェアエフェクト番号の値が異常です。",			"Abnormal hardware effect number",
	"トランスポーズの値が異常です",							"Abnormal transpose value",
	"連符の{}の中に音符がありません",						"Tuplet {} empty",
	"バンクが範囲を超えました",						"Bank index out of range",
	"音長が負の値です(unexpected error)",			"Frame length is negative value (unexpected error)",
	"音長の値が異常です",					"Abnormal note length value",
	"設定のパラメータが足りません",						"Parameter is lacking",
	"セルフディレイの値が異常です",						"Abnormal self-delay value",
	"DPCMサイズが0x4000を超える場合はバンク2と3は使用できません",		"Cannot use bank 2 or 3 if DPCM size is greater than 0x4000",
	"#PITCH-CORRECTIONを指定しない限りピッチシフト量コマンドは使用できません",		"Cannot use SA<num> without #PITCH-CORRECTION",
	"このトラックでは使用できないコマンドです",				"Unuse command in this track",
};



enum {
	TOO_MANY_INCLUDE_FILES = 0,
	FRAME_LENGTH_IS_0,
	REPEAT2_FRAME_ERROR_OVER_3,
	IGNORE_PMCKC_BANK_CHANGE,
	THIS_NUMBER_IS_ALREADY_USED,
	DPCM_FILE_SIZE_ERROR,
};

const	char	*WarningMessage[] = {
	"インクルードファイルの数が多すぎます",					"Too many include files",
	"フレーム音長が0になりました。",						"frame length is 0",
	"リピート2のフレーム誤差が3フレームを超えています。",	"Repeat2 frame error over 3 frames",
	"#BANK-CHANGE使用時は#SETBANK, NBは無視します",		"Ignoring #SETBANK and NB if #BANK-CHANGE used",
	"定義番号が重複しています",				"This definition number is already used",
	"DPCMサイズ mod 16 が1ではありません",			"DPCM size mod 16 is not 1",
};



/*--------------------------------------------------------------
	エラー表示
 Input:
	
 Output:
	none
--------------------------------------------------------------*/
void dispError( int no, char *file, int line )
{
	no = no*2;
	if( message_flag != 0 ) {
		no++;
	}
	if( file != NULL ) {
		printf( "Error  : %s %6d: %s\n", file, line, ErrorlMessage[no] );
	} else {
		printf( "Error  : %s\n", ErrorlMessage[no] );
	}
	error_flag = 1;
}



/*--------------------------------------------------------------
	ワーニング表示
 Input:
	
 Output:
	none
--------------------------------------------------------------*/
void dispWarning( int no, char *file, int line )
{
	if( warning_flag != 0 ) {
		no = no*2;
		if( message_flag != 0 ) {
			no++;
		}
		if( file != NULL ) {
			printf( "Warning: %s %6d: %s\n", file, line, WarningMessage[no] );
		} else {
			printf( "Warning: %s\n", WarningMessage[no] );
		}
	}
}



/*--------------------------------------------------------------
	C言語タイプのリマークの削除
 Input:
	char	*ptr		:データ格納ポインタ
 Output:
	none
--------------------------------------------------------------*/
void deleteCRemark( char *ptr )
{
	int within_com = 0;
	while ( *ptr != '\0' ) {
		if ( *ptr == '/' && *(ptr + 1) == '*' ) {
			within_com = 1;
			*ptr++ = ' ';
			*ptr++ = ' ';
			while (*ptr) {
				if (*ptr == '*' && *(ptr + 1) == '/') {
					*ptr++ = ' ';
					*ptr++ = ' ';
					within_com = 0;
					break;
				}
				if ( *ptr != '\n' ) {
					*ptr = ' ';
				}
				ptr++;
			}
		} else {
			++ptr;
		}
	}
	if (within_com) {
		printf("Warning :");
		printf( message_flag 	? "Reached EOF in comment"
					: "コメントが閉じられないままファイル終端に達しました");
		printf("\n");
	}
}


//不要
/*--------------------------------------------------------------
	リマークの削除
 Input:
	char	*ptr	:データ格納ポインタ
 Output:
	無し
--------------------------------------------------------------*/
void deleteRemark( char *ptr )
{
	while( *ptr != '\0' ) {
		if ( *ptr == '/' || *ptr == ';' ) {
			while( *ptr != '\0' ) {
				if( *ptr != '\n' ) {
					*ptr++ = ' ';
				} else {
					ptr++;
					break;
				}
			}
		} else {
			ptr++;
		}
	}
}



/*----------------------------------------------------------*/
/*	ファイル行数を求める								    */
/* Input:												    */
/*	char	*data		:データ格納ポインタ				    */
/* Output:												    */
/*	none												    */
/*----------------------------------------------------------*/
int getLineCount( char *ptr )
{
	int	line;

	line = 0;

	while( *ptr != '\0' ) {
		if( *ptr == '\n' ) {
			line++;
		}
		ptr++;
	}
	if( *(ptr-1) != '\n' ) {
		line++;
	}
	return line;
}


/*--------------------------------------------------------------
--------------------------------------------------------------*/
LINE *readMmlFile(char *fname)
{
	LINE *lptr;
	int line_count;
	int i;
	char *filestr;
	filestr = readTextFile(fname);
	
	if (filestr == NULL) {
		error_flag = 1;
		return NULL;
	}
	
	deleteCRemark(filestr);
	
	//skipSpaceに組み込み
	//deleteRemark(filestr);

	line_count = getLineCount(filestr);
	lptr = (LINE *)malloc( (line_count+1)*sizeof(LINE) );	/* ラインバッファを確保 */

	lptr[0].status = _HEADER;		/* LINEステータス[0]はmallocされた	*/
	lptr[0].str    = filestr;		/* ポインタとサイズが格納されている */
	lptr[0].line   = line_count;
	lptr[0].filename = fname;
	for( i = 1; i <= line_count; i++ ) {
		lptr[i].filename = fname;
		lptr[i].line = i;
	}
	return lptr;
}




/*--------------------------------------------------------------
	改行/EOFを0(NULL)にする(バッファを行単位で切り分け)
 Input:
	char	*ptr	:データ格納ポインタ
 Output:
	無し
--------------------------------------------------------------*/
char *changeNULL( char *ptr )
{
	while( *ptr != '\n' ) {
		if( *ptr == '\0' ) break;
		ptr++;
	}
	*ptr = '\0';
	ptr++;

	return ptr;
}


/*---------------------------------------------------------
  @hoge123 = { ag ae aeag g} の処理
  
  @HOGE\s*(\d+)\s*(=|)\s*{.*?(}.*|)$
-----------------------------------------------------------*/
int setEffectSub(LINE *lptr, int line, int *ptr_status_end_flag, int min, int max, const int error_no)
{
	int param, cnt;
	char *temp;
	temp = skipSpace( lptr[line].str );
	param = Asc2Int( temp, &cnt );
	
	if (cnt == 0)
		goto on_error;
	if (param < min || max <= param)
		goto on_error;
	
	lptr[line].param = param;
	temp = skipSpace( temp+cnt );
	
	if ( *temp == '=' ) {
		temp++;
		temp = skipSpace( temp );
	}
	
	if ( *temp != '{' )
		goto on_error;
	
	lptr[line].str = temp;
	*ptr_status_end_flag = 1;
	
	while ( *temp != '\0' ) {
		if( *temp == '}' ) {
			*ptr_status_end_flag = 0;
		}
		if ( *temp == '\"' )
			temp = skipQuote( temp );
		else
		if ( isComment( temp ) )
			temp = skipComment( temp );
		else
			temp++;

	}
	return 1;
on_error:
	lptr[line].status = 0;
	dispError( error_no, lptr[line].filename, line );
	return 0;
}




/*--------------------------------------------------------------
	ヘッダーを求める
 Input:
	char	*ptr	:データ格納ポインタ
 Output:
	無し
--------------------------------------------------------------*/
void getLineStatus(LINE *lptr, int inc_nest )
{
	const HEAD head[] = {
		{ "#TITLE",          _TITLE          },
		{ "#COMPOSER",       _COMPOSER       },
		{ "#MAKER",          _MAKER          },
		{ "#PROGRAMER",      _PROGRAMER      },
		{ "#OCTAVE-REV",     _OCTAVE_REV     },
		{ "#GATE-DENOM",        _GATE_DENOM  },
		{ "#INCLUDE",        _INCLUDE        },
		{ "#EX-DISKFM",      _EX_DISKFM      },
		{ "#EX-NAMCO106",    _EX_NAMCO106    },
		{ "#EX-VRC7",		 _EX_VRC7		 },
		{ "#EX-VRC6",		 _EX_VRC6		 },
		{ "#EX-FME7",		 _EX_FME7		 },
		{ "#EX-MMC5",		 _EX_MMC5		 },
		{ "#NO-BANKSWITCH",    _NO_BANKSWITCH    },
		{ "#AUTO-BANKSWITCH",    _AUTO_BANKSWITCH    },
		{ "#PITCH-CORRECTION",       _PITCH_CORRECTION    },
		{ "#BANK-CHANGE",    _BANK_CHANGE    },
		{ "#SETBANK",    	 _SET_SBANK	     },
		{ "#EFFECT-INCLUDE", _EFFECT_INCLUDE },
		{ "#DPCM-RESTSTOP", _DPCM_RESTSTOP },
		{ "@DPCM",           _SET_DPCM_DATA  },
		{ "@MP",             _SET_PITCH_MOD  },
		{ "@EN",             _SET_ARPEGGIO   },
		{ "@EP",             _SET_PITCH_ENV  },
		{ "@FM",             _SET_FM_TONE    },
		{ "@MH",             _SET_HARD_EFFECT},
		{ "@MW",             _SET_EFFECT_WAVE},
		{ "@OP",             _SET_VRC7_TONE  },
		{ "@N",              _SET_N106_TONE  },			
		{ "@V",              _SET_ENVELOPE   },
		{ "@",               _SET_TONE       },
		{ "",                -1              },
	};

	int	line, i, param, cnt, track_flag, status_end_flag, bank,bank_ch;
	char	*temp, *temp2;
	char *ptr;
	ptr = lptr[0].str;

	status_end_flag = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		ptr = skipSpace( ptr );
		/* 前の行がエフェクト定義処理だった？ */
		if( ((lptr[line-1].status&_SET_EFFECT) != 0) && (status_end_flag != 0) ) {
			lptr[line].status = lptr[line-1].status|_SAME_LINE;
			lptr[line].param  = lptr[line-1].param;
			lptr[line].str = ptr;
			temp = ptr;
			ptr = changeNULL( ptr );
			status_end_flag = 1;
			while( *temp != '\0' ) {
				if( *temp == '}' ) {
					status_end_flag = 0;
				}
				if ( *temp == '\"' )
					temp = skipQuote( temp );
				else
				if ( isComment( temp ) )
					temp = skipComment( temp );
				else
					temp++;
			}
		/* 行の先頭に何も無い時は無効な行とする */
		} else if( *ptr == '\n' || *ptr == '\0' ) {
			lptr[line].status = 0;
			lptr[line].str = ptr;
			ptr = changeNULL( ptr );
		} else {
			/* #/@付きヘッダーの時はヘッダーの文字列を大文字にする */
			if( *ptr == '#' || *ptr == '@' ) {
				i = 1;
				while( (*(ptr+i) != ' ') && (*(ptr+i) != '\t') && (*(ptr+i) != '\n') ) {
					*(ptr+i) = (char)toupper( *(ptr+i) );
					i++;
				}
				/* ヘッダーも字列をテーブル文字列と比較 */
				for( i = 0; head[i].status != -1; i++ ) {
					if( strncmp( ptr, head[i].str, strlen(head[i].str) ) == 0 ) {
						break;
					}
				}
				lptr[line].status = head[i].status;
				lptr[line].str = skipSpaceOld( ptr+strlen(head[i].str) );	/* ヘッダー＋空白を飛ばしたところを先頭に */
			} else if( strchr( str_track, *ptr ) ) {
				track_flag = 0;
				while( strchr( str_track, *ptr ) ) {
					temp = strchr( str_track, *ptr );
					if( temp == NULL ) {
						dispError( INVALID_TRACK_HEADER, lptr[line].filename, line );
					} else {
						track_flag |= (1<<(temp-str_track));
					}
					ptr++;
				}
				// トラック許可のチェック
				for (i = 0; i < _TRACK_MAX; i++) {
					if( (TRACK(i) & track_flag) && !(TRACK(i) & track_allow_flag) ) {
						dispError( INVALID_TRACK_HEADER, lptr[line].filename, line );
						track_flag &= ~TRACK(i);
					}
				}
				
				if( track_flag != 0 ) {
					lptr[line].status = _TRACK;
					lptr[line].param = track_flag;
					lptr[line].str = skipSpace( ptr );
					actual_track_flag |= track_flag;				
				} else {
					lptr[line].status = 0;
					lptr[line].param = 0;
				}
			} else {
				lptr[line].status = -1;
				lptr[line].str = skipSpace( ptr );
			}
			ptr = changeNULL( ptr );

			switch( lptr[line].status ) {
			/* Includeコマンドの処理 */
			  case _INCLUDE:
				if( inc_nest > 16 ) {				/* ネストは16段まで(再帰で呼ばれると終了しないので) */
					dispWarning( TOO_MANY_INCLUDE_FILES, lptr[line].filename, line );
					lptr[line].status = 0;
				} else {
					LINE *ltemp;
					temp = skipSpaceOld( lptr[line].str ); /* /をとばさないようにしてみる */
					ltemp = readMmlFile(temp);
					if( ltemp != NULL ) {
						lptr[line].inc_ptr = ltemp;
						++inc_nest;
						getLineStatus(lptr[line].inc_ptr, inc_nest);
						--inc_nest;
					} else {
						lptr[line].status = 0;					/* ファイル読み込み失敗に付きエラー */
						error_flag = 1;
					}
				}
				break;
			/* LFOコマンド */
			  case _SET_PITCH_MOD:
				setEffectSub(lptr, line, &status_end_flag, 0, _PITCH_MOD_MAX, LFO_DEFINITION_IS_WRONG);
				break;
			/* ピッチエンベロープコマンド */
			  case _SET_PITCH_ENV:
				setEffectSub(lptr, line, &status_end_flag, 0, _PITCH_ENV_MAX, PITCH_ENVELOPE_DEFINITION_IS_WRONG);
				break;
			/* 音量エンベロープコマンド */
			  case _SET_ENVELOPE:
				setEffectSub(lptr, line, &status_end_flag, 0, _ENVELOPE_MAX, ENVELOPE_DEFINITION_IS_WRONG);
				break;
			/* 自作音色 */
			  case _SET_TONE:
				setEffectSub(lptr, line, &status_end_flag, 0, _TONE_MAX, TONE_DEFINITION_IS_WRONG);
				break;
			/* アルペジオ */
			  case _SET_ARPEGGIO:
				setEffectSub(lptr, line, &status_end_flag, 0, _ARPEGGIO_MAX, NOTE_ENVELOPE_DEFINITION_IS_WRONG);
				break;
			/* DPCM登録コマンド */
			  case _SET_DPCM_DATA:
				setEffectSub(lptr, line, &status_end_flag, 0, _DPCM_MAX, DPCM_DEFINITION_IS_WRONG);
				break;
			  /* VRC7 Tone */
			  case _SET_VRC7_TONE:
				setEffectSub(lptr, line, &status_end_flag, 0, _VRC7_TONE_MAX, FM_TONE_DEFINITION_IS_WRONG);
				break;
			/* FM音色 */
			  case _SET_FM_TONE:
				setEffectSub(lptr, line, &status_end_flag, 0, _FM_TONE_MAX, FM_TONE_DEFINITION_IS_WRONG);
				break;
			/* namco106音源音色 */
			  case _SET_N106_TONE:
				setEffectSub(lptr, line, &status_end_flag, 0, _N106_TONE_MAX, N106_TONE_DEFINITION_IS_WRONG);
				break;
			/* ハードウェアエフェクト */
			  case _SET_HARD_EFFECT:
				setEffectSub(lptr, line, &status_end_flag, 0, _HARD_EFFECT_MAX, HARD_EFFECT_DEFINITION_IS_WRONG);
				break;
			/* エフェクト波形 */
			  case _SET_EFFECT_WAVE:
				setEffectSub(lptr, line, &status_end_flag, 0, _EFFECT_WAVE_MAX, EFFECT_WAVE_DEFINITION_IS_WRONG);
				break;
			/* DISKSYSTEM FM音源使用フラグ */
			  case _EX_DISKFM:
				sndgen_flag |= BDISKFM;
				track_allow_flag |= FMTRACK;
				fds_track_num = 1;
				break;
			/* VRC7 FM音源使用フラグ */
			  case _EX_VRC7:
				sndgen_flag |= BVRC7;
				track_allow_flag |= VRC7TRACK;
				vrc7_track_num = 6;
				break;
			/* VRC6 音源使用フラグ */
			  case _EX_VRC6:
				sndgen_flag |= BVRC6;
				track_allow_flag |= VRC6TRACK;
				vrc6_track_num = 3;
				break;
			/* FME7 音源使用フラグ */
			  case _EX_FME7:
				sndgen_flag |= BFME7;
				track_allow_flag |= FME7TRACK;
				fme7_track_num = 3;
				break;
			/* MMC5 音源使用フラグ */
			  case _EX_MMC5:
				sndgen_flag |= BMMC5;
				track_allow_flag |= MMC5TRACK;
				mmc5_track_num = 2;
				break;
			/* namco106 拡張音源使用フラグ */
			  case _EX_NAMCO106:
				temp = skipSpace( lptr[line].str );
				param = Asc2Int( temp, &cnt );
				if( cnt != 0 && (0 <= param && param <= 8) ) {
					if( param == 0 ) {
						param = 1;
					}
					lptr[line].param = param;
					n106_track_num = param;
					sndgen_flag |= BNAMCO106;
					for (i = 0; i < param; i++) {
						track_allow_flag |= TRACK(BN106TRACK+i);
					}
				} else {
					dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
					lptr[line].status = 0;
				}
				break;
			/* DPCM sound stops on 'r' command */
			  case _DPCM_RESTSTOP:
				dpcm_reststop = 1;
				break;
			/* NSF mapper の bankswitching 禁止 */
			  case _NO_BANKSWITCH:
				allow_bankswitching = 0;
				break;
			/* 自動バンク切り替え */
			  case _AUTO_BANKSWITCH:
				temp = skipSpace( lptr[line].str );
				param = Asc2Int( temp, &cnt );
				if ( cnt != 0 && (0 <= param && param <= 8192)) {
					// 最初の一回しか有効にしない
					if (!auto_bankswitch) {
						bank_usage[0] = 8192 - param;
					}
					auto_bankswitch = 1;
				} else {
					dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
				}
				break;
			/* バンク切り替え埋め込み(暫定処理の互換措置) */
			  case _BANK_CHANGE:
				/*
					#BANK-CHANGE <num0>,<num1>
					上記バンク切り替えの拡張書式です。<num0>はバンク番号で0～2の値が
					入ります。<num1>はトラック番号で1～14の数値が入り、1がAトラックに
					対応しており以下2=B、3=C、…P=7となっています。
					ちなみに以下は同じことをしています。
					#BANK-CHANGE	n
					#BANK-CHANGE	0,n
					
					#BANK-CHANGEで同じバンクにトラックを持っていった場合、
					最後に指定したものだけが有効。という仕様はあまり理解されていなかった。
					ppmckでは全て有効とするため、その点は非互換。
					
					mckc用の古いMMLをコンパイルするためには
					最後のもの以外消す。
				
				*/
				/*
					数字とトラックの対応は非互換。
				
					mckc
					A B C D E | F | P Q R  S  T  U  V  W
					1 2 3 4 5 | 6 | 7 8 9 10 11 12 13 14
					pmckc
					A B C D E | F | G H I  J  K  L |  P  Q  R  S  T  U  V  W
					1 2 3 4 5 | 6 | 7 8 9 10 11 12 | 13 14 15 16 17 18 19 20
					ppmckc
					A B C D E | F | G H I  J  K  L |  M  N  O |  P  Q  R  S  T  U  V  W |  X  Y  Z |  a  b
					1 2 3 4 5 | 6 | 7 8 9 10 11 12 | 13 14 15 | 16 17 18 19 20 21 22 23 | 24 25 26 | 27 28
				
					mckc用の古いMMLをコンパイルするためには
					P以降は 手動で 9 を足せばOK。(自動にはしないほうがよいでしょう)
					てかこんな表を見なきゃいけないことが間違って(ry
				*/
				temp = skipSpace( lptr[line].str );
				param = Asc2Int( temp, &cnt );
				if( cnt != 0 ) {
					temp += cnt;
					temp = skipSpace( temp );
					if( *temp == ',' ) {
						/* 拡張書式 */
						temp++;
						if( (0 <= param) && (param <= 2) ) {
							bank = param; /* 0,1,2が1,2,3に対応 */
							//printf( "bank: %d\n", bank );
							temp = skipSpace( temp );
							param = Asc2Int( temp, &cnt ); /* 1,2,3 がABCに対応 だから 0,1,2に対応 */
							if( cnt != 0 && (1 <= param && param <= _TRACK_MAX) ) {
								//bank_change[bank] = param-1;
								bank_sel[param-1] = bank+1;
							} else {
								dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
								lptr[line].status = 0;
								//bank_change[bank] = 0xff;
							}
						} else {
							dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
							lptr[line].status = 0;
						}
					} else {
						/* 非拡張書式 bank 1に入れる */
						if( cnt != 0 && (1 <= param && param <= _TRACK_MAX) ) {
							//bank_change[0] = param-1;
							bank_sel[param-1] = 1;
						} else {
							dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
							lptr[line].status = 0;
							//bank_change[0] = 0xff;
						}
					}
				} else {
					dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
					lptr[line].status = 0;
				}
				break;
			/* バンク切り替え */
			  case _SET_SBANK:
				temp = skipSpace( lptr[line].str );
				
				if ((temp2 = strchr(str_track, *temp)) != NULL) {
					/* ABC..によるトラック指定 */
					param = (temp2 - str_track) + 1;
					temp++;
				} else {
					/* 数字によるトラック指定 */
					param = Asc2Int( temp, &cnt );
					if (cnt == 0) {
						dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
						lptr[line].status = 0;
						break;
					} else {
						temp += cnt;
					}
				}
				
				temp = skipSpace( temp );
				if( *temp == ',' ) {		/* バンク拡張 */
					temp++;
					if( (1 <= param) && (param <= _TRACK_MAX) ) {
						bank_ch = param;
						// printf( "bank: %d\n", bank );
						temp = skipSpace( temp );
						param = Asc2Int( temp, &cnt );
						if( cnt != 0) {
							if (checkBankRange(param) == 0) {
								dispError( BANK_IDX_OUT_OF_RANGE, lptr[line].filename, line );
								break;
							}
							bank_sel[bank_ch-1] = param;
						} else {
							dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
							lptr[line].status = 0;
						}
					} else {
						dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
						lptr[line].status = 0;
					}
				} else {
					dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
					lptr[line].status = 0;
				}
				break;

			/*  */
			  case _EFFECT_INCLUDE:
				include_flag = 1;
				break;
			/* タイトル */
			  case _TITLE:
				temp = skipSpaceOld( lptr[line].str );
				strncpy( song_name, temp, 1023 );
				break;
			/* 作曲者 */
			  case _COMPOSER:
				temp = skipSpaceOld( lptr[line].str );
				strncpy( composer, temp, 1023 );
				break;
			/* メーカー */
			  case _MAKER:
				temp = skipSpaceOld( lptr[line].str );
				strncpy( maker, temp, 1023 );
				break;
			/* 打ち込み者 */
			  case _PROGRAMER:
				temp = skipSpaceOld( lptr[line].str );
				strncpy( programer_buf, temp, 1023 );
				programer = programer_buf;
				break;
			/* オクターブ記号の反転 */
			  case _OCTAVE_REV:
				temp = skipSpace( lptr[line].str );
				param = Asc2Int( temp, &cnt );
				if( cnt != 0) {
					if( param == 0 ) {
						octave_flag = 0;
					} else {
						octave_flag = 1;
					}
				} else {
					octave_flag = 1;
				}
				break;
			/* qコマンド分母変更 */
			  case _GATE_DENOM:
				temp = skipSpace( lptr[line].str );
				param = Asc2Int( temp, &cnt );
				if( cnt != 0 && param > 0) {
					gate_denom = param;
				} else {
					dispError( DEFINITION_IS_WRONG, lptr[line].filename, line );
					lptr[line].status = 0;
				}
				break;
			/*ディチューン、ピッチエンベロープ、LFOの方向修正 */
			  case _PITCH_CORRECTION:
				pitch_correction = 1;
				break;
			/* ヘッダ無し */
			  case -1:
				if( (lptr[line-1].status&_SET_EFFECT) != 0 ) {
					lptr[line].status = lptr[line-1].status|_SAME_LINE;
					lptr[line].str = ptr;
				} else {
					/* エラーチェック */
					dispError( COMMAND_NOT_DEFINED, lptr[line].filename, line );
					lptr[line].status = 0;
					lptr[line].str = ptr;
				}
				break;
			}
		}
	}
}



/*--------------------------------------------------------------
	音色の取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getTone( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_TONE ) {
			no = lptr[line].param;				/* 音色番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (tone_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			tone_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (tone_tbl[no][0] >= 1) {
						tone_tbl[no][i] = EFTBL_END;
						tone_tbl[no][0]++;
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						tone_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '|':
					tone_tbl[no][i] = EFTBL_LOOP;
					tone_tbl[no][0]++;
					i++;
					ptr++;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( TONE_DEFINITION_IS_WRONG, lptr[line].filename, line );
						tone_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					//vrc6用に制限を外す(内蔵矩形波、MMC5は3まで)
					//if( cnt != 0 && (0 <= num && num <= 3) ) {
					if( cnt != 0 && (0 <= num && num <= 7) ) {
						tone_tbl[no][i] = num;
						tone_tbl[no][0]++;
						ptr += cnt;
						i++;
					} else {
						dispError( TONE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						tone_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_TONE|_SAME_LINE) ) {
			dispError( TONE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getTone( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	エンベロープの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getEnvelope( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* エンベロープデータ発見？ */
		if( lptr[line].status == _SET_ENVELOPE ) {
			no = lptr[line].param;				/* エンベロープ番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (envelope_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			envelope_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (envelope_tbl[no][0] >= 1) {
						envelope_tbl[no][i] = EFTBL_END;
						envelope_tbl[no][0]++;
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						envelope_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '|':
					envelope_tbl[no][i] = EFTBL_LOOP;
					envelope_tbl[no][0]++;
					i++;
					ptr++;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( ENVELOPE_DEFINITION_IS_WRONG, lptr[line].filename, line );
						envelope_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 && (0 <= num && num <= 63) ) {
						envelope_tbl[no][i] = num;
						envelope_tbl[no][0]++;
						ptr += cnt;
						i++;
					} else {
						dispError( ENVELOPE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						envelope_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* エンベロープ定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_ENVELOPE|_SAME_LINE) ) {
			dispError( ENVELOPE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getEnvelope( lptr[line].inc_ptr );
		}
	}
}

/*--------------------------------------------------------------
	ピッチエンベロープの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getPitchEnv( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* ピッチエンベロープデータ発見？ */
		if( lptr[line].status == _SET_PITCH_ENV ) {
			no = lptr[line].param;				/* ピッチエンベロープ番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (pitch_env_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			pitch_env_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (pitch_env_tbl[no][0] >= 1) {
						pitch_env_tbl[no][i] = EFTBL_END;
						pitch_env_tbl[no][0]++;
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						pitch_env_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '|':
					pitch_env_tbl[no][i] = EFTBL_LOOP;
					pitch_env_tbl[no][0]++;
					i++;
					ptr++;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( PITCH_ENVELOPE_DEFINITION_IS_WRONG, lptr[line].filename, line );
						pitch_env_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 && (-127 <= num && num <= 126) ) {
						pitch_env_tbl[no][i] = num;
						pitch_env_tbl[no][0]++;
						ptr += cnt;
						i++;
					} else {
						dispError( PITCH_ENVELOPE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						pitch_env_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* ピッチエンベロープ定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_PITCH_ENV|_SAME_LINE) ) {
			dispError( PITCH_ENVELOPE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getPitchEnv( lptr[line].inc_ptr );
		}
	}
}

/*--------------------------------------------------------------
	ピッチモジュレーションの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getPitchMod( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_PITCH_MOD ) {
			no = lptr[line].param;				/* LFO番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (pitch_mod_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			pitch_mod_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (pitch_mod_tbl[no][0] >= 3) {
						//OK.
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						pitch_mod_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( LFO_DEFINITION_IS_WRONG, lptr[line].filename, line );
						pitch_mod_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 ) {
						switch( i ) {
						  case 1:
						  case 2:
						  case 3:
							if( 0 <= num && num <= 255 ) {
								pitch_mod_tbl[no][i] = num;
								pitch_mod_tbl[no][0]++;
								ptr += cnt;
								i++;
							} else {
								dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
								pitch_mod_tbl[no][0] = 0;
								end_flag = 1;
							}
							break;
						  case 4:
							if( 0 <= num && num <= 255 ) {
								pitch_mod_tbl[no][i] = num;
								pitch_mod_tbl[no][0]++;
								ptr += cnt;
								i++;
							} else {
								dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
								pitch_mod_tbl[no][0] = 0;
								end_flag = 1;
							}
							break;
						  default:
							dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
							pitch_mod_tbl[no][0] = 0;
							end_flag = 1;
							break;
						}
					} else {
						dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						pitch_mod_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_PITCH_MOD|_SAME_LINE) ) {
			dispError( LFO_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getPitchMod( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	ノートエンベロープの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getArpeggio( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* アルペジオデータ発見？ */
		if( lptr[line].status == _SET_ARPEGGIO ) {
			no = lptr[line].param;				/* エンベロープ番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (arpeggio_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			arpeggio_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (arpeggio_tbl[no][0] >= 1) {
						arpeggio_tbl[no][i] = EFTBL_END;
						arpeggio_tbl[no][0]++;
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						arpeggio_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '|':
					arpeggio_tbl[no][i] = EFTBL_LOOP;
					arpeggio_tbl[no][0]++;
					i++;
					ptr++;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( NOTE_ENVELOPE_DEFINITION_IS_WRONG, lptr[line].filename, line );
						arpeggio_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 ) {
						if( num >= 0 ) {
							arpeggio_tbl[no][i] = num;
						} else {
							arpeggio_tbl[no][i] = (-num)|0x80;
						}	
						arpeggio_tbl[no][0]++;
						ptr += cnt;
						i++;
					} else {
						dispError( NOTE_ENVELOPE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						arpeggio_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* アルペジオ定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_ARPEGGIO|_SAME_LINE) ) {
			dispError( NOTE_ENVELOPE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getArpeggio( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	DPCMの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getDPCM( LINE *lptr )
{
	int		line, i, no, offset, end_flag, num, cnt;
	char	*ptr;
	FILE	*fp;
	DPCMTBL	*tbl;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		// DPCMデータ発見？
		if( lptr[line].status == _SET_DPCM_DATA ) {
			no = lptr[line].param;				// DPCM番号取得
			ptr = lptr[line].str;
			ptr++;								// '{'の分を飛ばす
			tbl = &dpcm_tbl[no];
			if (tbl->flag != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			tbl->flag = 1;						// フラグを使用中に
			tbl->index = -1;
			tbl->fname = NULL;
			tbl->freq = 0;
			tbl->size = 0;
			tbl->delta_init = 0;
			offset = 0;
			i = 0;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				// データ終了
				  case '}':
					switch( i ) {
					  case 0:
					  case 1:
						dispError( DPCM_PARAMETER_IS_LACKING, lptr[line].filename, line );
						tbl->flag = 0;
						break;
					  default:
						line += offset;
						break;
					}
					end_flag = 1;
					break;
				// 改行
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( DPCM_DEFINITION_IS_WRONG, lptr[line].filename, line );
						tbl->flag = 0;
						end_flag = 1;
					}
					break;
				  default:
					switch( i ) {
					// ファイル名を登録
					  case 0:
						// ファイル名は"..."で囲まれている？
						if( *ptr == '\"' ) {
							ptr++;
							//ptr = skipSpace( ptr );
							//"file.dmc"はOK. " file.dmc"はNG.
							tbl->fname = ptr;
							while( *ptr != '\"' && *ptr != '\0' ) {
								ptr++;
							}
						} else {
							tbl->fname = ptr;
							//空白があるところまではファイル名
							// '/'';'はとばさない
							while( *ptr != ' ' && *ptr != '\t' && *ptr != '\0' ) {
								ptr++;
							}
						}
						*ptr = '\0';
						ptr++;
						// ファイル存在チェック/サイズチェック
						if( (fp = openDmc( tbl->fname )) == NULL ) {
							dispError( DPCM_FILE_NOT_FOUND, lptr[line+offset].filename, line );
							tbl->flag = 0;
							end_flag = 1;
						} else {
							fseek( fp, 0, SEEK_END );
							tbl->size = ftell( fp );
							fseek( fp, 0, SEEK_SET );
							fclose( fp );
						}
						i++;
						break;
					// 再生周波数を登録
					  case 1:
						num = Asc2Int( ptr, &cnt );
						if( cnt != 0 && (0 <= num && num <= 15) ) {
								tbl->freq = num;
						} else {
							dispError( DPCM_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
							tbl->flag = 0;
							end_flag = 1;
						}
						ptr += cnt;
						i++;
						break;
					// 再生サイズを登録
					  case 2:
						num = Asc2Int( ptr, &cnt );
						if (cnt != 0 && num == 0) {
							//値が0のときは省略と同じ
							ptr += cnt;
							i++;
							break;
						}
						if( cnt != 0 && (0 < num && num < 16384) ) {
							tbl->size = num;
						} else {
							dispError( DPCM_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
							tbl->flag = 0;
							end_flag = 1;
						}
						ptr += cnt;
						i++;
						break;
					// デルタカウンタ($4011)初期値を登録
					  case 3:
						num = Asc2Int(ptr, &cnt);
						if (cnt != 0 && ((0 <= num && num <= 0x7f) || num == 0xff)) {
							tbl->delta_init = num;
						} else {
							dispError( DPCM_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
							tbl->flag = 0;
							end_flag = 1;
						}
						ptr += cnt;
						i++;
						break;
					// ループ情報($4010のbit7,6)を登録
					  case 4:
						num = Asc2Int(ptr, &cnt);
						if (cnt != 0 && (0 <= num && num <= 2)) {
							tbl->freq |= (num<<6);
						} else {
							dispError( DPCM_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
							tbl->flag = 0;
							end_flag = 1;
						}
						ptr += cnt;
						i++;
						break;
					  default:
						dispError( DPCM_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						tbl->flag = 0;
						end_flag = 1;
						break;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
			if( tbl->size > (0xff)*16+1 ) {
				dispError( DPCM_FILE_SIZE_OVER, lptr[line+offset].filename, line );
				tbl->flag = 0;
			} else if ((tbl->size % 16) != 1) {
				dispWarning( DPCM_FILE_SIZE_ERROR, lptr[line+offset].filename, line );
			}
		// DPCM定義だけど_SAME_LINEの時はエラー
		} else if( lptr[line].status == (_SET_DPCM_DATA|_SAME_LINE) ) {
			dispError( DPCM_DEFINITION_IS_WRONG, lptr[line].filename, line );
		// インクルードファイル処理
		} else if( lptr[line].status == _INCLUDE ) {
			getDPCM( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	FDS FM音色の取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getFMTone( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_FM_TONE ) {
			no = lptr[line].param;				/* 音色番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (fm_tone_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			fm_tone_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (fm_tone_tbl[no][0] == 64) {
						//OK.
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						fm_tone_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( FM_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line+offset );
						fm_tone_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 && (0 <= num && num <= 0x3f) ) {
						fm_tone_tbl[no][i] = num;
						fm_tone_tbl[no][0]++;
						ptr += cnt;
						i++;
						if( i > 65 ) {
							dispError( ABNORMAL_PARAMETERS_OF_FM_TONE, lptr[line+offset].filename, line+offset );
							fm_tone_tbl[no][0] = 0;
							line += offset;
							end_flag = 1;
						}
					} else {
						dispError( FM_TONE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line+offset );
						fm_tone_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_FM_TONE|_SAME_LINE) ) {
			dispError( FM_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getFMTone( lptr[line].inc_ptr );
		}
	}
}


/*--------------------------------------------------------------
	VRC7音色の取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getVRC7Tone( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_VRC7_TONE ) {
			no = lptr[line].param;				/* 音色番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (vrc7_tone_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			vrc7_tone_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (vrc7_tone_tbl[no][0] == 8) {
						//OK.
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						vrc7_tone_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( FM_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line+offset );
						vrc7_tone_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 && (0 <= num && num <= 0xff) ) {
						vrc7_tone_tbl[no][i] = num;
						vrc7_tone_tbl[no][0]++;
						ptr += cnt;
						i++;
						if( i > 9 ) {
							dispError( ABNORMAL_PARAMETERS_OF_FM_TONE, lptr[line+offset].filename, line+offset );
							vrc7_tone_tbl[no][0] = 0;
							line += offset;
							end_flag = 1;
						}
					} else {
						dispError( FM_TONE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line+offset );
						vrc7_tone_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
			if( i != 9 ) {
				if (!error_flag) {
					dispError( ABNORMAL_PARAMETERS_OF_FM_TONE, lptr[line].filename, line);
					vrc7_tone_tbl[no][0] = 0;
				}
			}


		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_VRC7_TONE|_SAME_LINE) ) {
			dispError( FM_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getVRC7Tone( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	namco106音色の取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getN106Tone( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;
					//	   16 14 12 10  8  6  4  2 			
	int	n106_tone_max[] = { 4, 4, 5, 6, 8,10,16,32 }; 
	int	n106_tone_num;

	cnt = 0;
	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_N106_TONE ) {
			no = lptr[line].param;				/* 音色番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (n106_tone_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			n106_tone_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					//要素の数はwhileを抜けた後でチェック
					end_flag = 1;
					line += offset;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( N106_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line+offset );
						n106_tone_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( i == 1 ) {						// 登録バッファ(0～5)			
						if( cnt != 0 && (0 <= num && num <= 32) ) {
							n106_tone_tbl[no][1] = num;
							n106_tone_tbl[no][0]++;
							ptr += cnt;
							i++;
						} else {
							dispError( N106_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line+offset );
							n106_tone_tbl[no][0] = 0;
							line += offset;
							end_flag = 1;
						}
					} else {
						if( cnt != 0 && (0 <= num && num <= 15) ) {
							n106_tone_tbl[no][i] = num;
							n106_tone_tbl[no][0]++;
							ptr += cnt;
							i++;
							if( i > 2+32 ) {
								dispError( ABNORMAL_PARAMETERS_OF_N106_TONE, lptr[line+offset].filename, line+offset );
								n106_tone_tbl[no][0] = 0;
								line += offset;
								end_flag = 1;
							}
						} else {
							dispError( N106_TONE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line+offset );
							n106_tone_tbl[no][0] = 0;
							line += offset;
							end_flag = 1;
						}
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
			switch( n106_tone_tbl[no][0] ) {
			  case 16*2+1: n106_tone_num =  0; break;
			  case 14*2+1: n106_tone_num =  1; break;
			  case 12*2+1: n106_tone_num =  2; break;
			  case 10*2+1: n106_tone_num =  3; break;
			  case  8*2+1: n106_tone_num =  4; break;
			  case  6*2+1: n106_tone_num =  5; break;
			  case  4*2+1: n106_tone_num =  6; break;
			  case  2*2+1: n106_tone_num =  7; break;
			  default:     n106_tone_num = -1; break;
			}
			if( n106_tone_num == -1 ) {
				dispError( ABNORMAL_PARAMETERS_OF_N106_TONE, lptr[line].filename, line );
				n106_tone_tbl[no][0] = 0;
			}
			if( n106_tone_tbl[no][1] >= n106_tone_max[n106_tone_num] ) {
				dispError( N106_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line );
				n106_tone_tbl[no][0] = 0;
			}
		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_N106_TONE|_SAME_LINE) ) {
			dispError( N106_TONE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getN106Tone( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	ハードウェアエフェクトの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getHardEffect( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_HARD_EFFECT ) {
			no = lptr[line].param;				/* エフェクト番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (hard_effect_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			hard_effect_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (hard_effect_tbl[no][0] == 4) {
						//OK.
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						hard_effect_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( LFO_DEFINITION_IS_WRONG, lptr[line].filename, line );
						hard_effect_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 ) {
						switch( i ) {
						  case 1:
							if( 0 <= num && num <= 255 ) {
								hard_effect_tbl[no][i] = num;
								hard_effect_tbl[no][0]++;
								ptr += cnt;
								i++;
							} else {
								dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
								hard_effect_tbl[no][0] = 0;
								end_flag = 1;
							}
							break;
						  case 2:
							if( 0 <= num && num <= 4095 ) {
								hard_effect_tbl[no][i] = num;
								hard_effect_tbl[no][0]++;
								ptr += cnt;
								i++;
							} else {
								dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
								hard_effect_tbl[no][0] = 0;
								end_flag = 1;
							}
							break;
						  case 3:
							if( 0 <= num && num <= 255 ) {
								hard_effect_tbl[no][i] = num;
								hard_effect_tbl[no][0]++;
								ptr += cnt;
								i++;
							} else {
								dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
								hard_effect_tbl[no][0] = 0;
								end_flag = 1;
							}
							break;
						  case 4:
							if( 0 <= num && num <= 7 ) {
								hard_effect_tbl[no][i] = num;
								hard_effect_tbl[no][0]++;
								ptr += cnt;
								i++;
							} else {
								dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
								hard_effect_tbl[no][0] = 0;
								end_flag = 1;
							}
							break;
						  default:
							dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
							hard_effect_tbl[no][0] = 0;
							end_flag = 1;
							break;
						}
					} else {
						dispError( LFO_DEFINITION_IS_WRONG, lptr[line+offset].filename, line );
						hard_effect_tbl[no][0] = 0;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_HARD_EFFECT|_SAME_LINE) ) {
			dispError( LFO_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getHardEffect( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	エフェクト波形の取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void getEffectWave( LINE *lptr )
{
	int		line, i, no, end_flag, offset, num, cnt;
	char	*ptr;

	cnt = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		/* 音色データ発見？ */
		if( lptr[line].status == _SET_EFFECT_WAVE ) {
			no = lptr[line].param;				/* 波形番号取得 */
			ptr = lptr[line].str;
			ptr++;								/* '{'の分を飛ばす */
			if (effect_wave_tbl[no][0] != 0) {
				dispWarning( THIS_NUMBER_IS_ALREADY_USED, lptr[line].filename, line );
			}
			effect_wave_tbl[no][0] = 0;
			offset = 0;
			i = 1;
			end_flag = 0;
			while( end_flag == 0 ) {
				ptr = skipSpace( ptr );
				switch( *ptr ) {
				  case '}':
					if (effect_wave_tbl[no][0] == 32) {
						//OK.
					} else {
						dispError( PARAMETER_IS_LACKING, lptr[line].filename, line );
						effect_wave_tbl[no][0] = 0;
					}
					end_flag = 1;
					line += offset;
					break;
				  case '\0':
					offset++;
					if( line+offset <= lptr->line ) {
						if( (lptr[line+offset].status&_SAME_LINE) == _SAME_LINE ) {
							ptr = lptr[line+offset].str;
						}
					} else {
						dispError( EFFECT_WAVE_DEFINITION_IS_WRONG, lptr[line].filename, line+offset );
						effect_wave_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				  default:
					num = Asc2Int( ptr, &cnt );
					if( cnt != 0 && (0 <= num && num <= 7) ) {
						effect_wave_tbl[no][i] = num;
						effect_wave_tbl[no][0]++;
						ptr += cnt;
						i++;
						if( i > 33 ) {
							dispError( EFFECT_WAVE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line+offset );
							effect_wave_tbl[no][0] = 0;
							line += offset;
							end_flag = 1;
						}
					} else {
						dispError( EFFECT_WAVE_DEFINITION_IS_WRONG, lptr[line+offset].filename, line+offset );
						effect_wave_tbl[no][0] = 0;
						line += offset;
						end_flag = 1;
					}
					break;
				}
				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
				}
			}
		/* 音色定義だけど_SAME_LINEの時はエラー */
		} else if( lptr[line].status == (_SET_EFFECT_WAVE|_SAME_LINE) ) {
			dispError( EFFECT_WAVE_DEFINITION_IS_WRONG, lptr[line].filename, line );
		/* インクルードファイル処理 */
		} else if( lptr[line].status == _INCLUDE ) {
			getEffectWave( lptr[line].inc_ptr );
		}
	}
}



/*--------------------------------------------------------------
	DPCMデータのダブりを削除
 Input:
	無し
 Output:
	無し
--------------------------------------------------------------*/
void sortDPCM( DPCMTBL dpcm_tbl[_DPCM_MAX] )
{
	int	i, j;

	for( i = 0; i < _DPCM_MAX; i++ ) {
		if( dpcm_tbl[i].flag == 0 || dpcm_tbl[i].index != -1 ) continue;
		for( j = 0; j < _DPCM_MAX; j++ ) {
			if( i == j ) continue;
			if( dpcm_tbl[j].flag == 0 ) continue;
			// ファイル名が同じ？
			if( strcmp( dpcm_tbl[i].fname, dpcm_tbl[j].fname ) == 0
			 && dpcm_tbl[i].size >= dpcm_tbl[j].size ) {
				dpcm_tbl[j].index = i;
			}
		}
	}
}



/*--------------------------------------------------------------
	DPCMサイズ修正(16byteバウンダリに修正)
 Input:
	
 Output:
 	int: DPCMサイズ
--------------------------------------------------------------*/
int checkDPCMSize( DPCMTBL dpcm_tbl[_DPCM_MAX] )
{
	int	i;
	int	adr = 0;
	int	size = 0;
	int	bank = 0; //0x4000ごとに増加
	for( i = 0; i < _DPCM_MAX; i++ ) {
		if( dpcm_tbl[i].flag != 0 ) {
			/*
			   $4013 * 16 + 1 = size
			   $4013 = (size - 1) / 16 
			
			   newsize % 16 == 1が成立するように調整
			   size%16	(size-1)%16	diff(floor)	diff(ceil)
			   1		0		0		0
			   2		1		-1		+15
			   3		2		-2		+14
			   4		3		-3		+13
			   15		14		-14		+2
			   0		15		-15		+1
			*/
			//printf("%s size $%x\n", dpcm_tbl[i].fname, dpcm_tbl[i].size);
			if ((dpcm_tbl[i].size % 16) != 1) {
				int diff;
				diff = (16 - ((dpcm_tbl[i].size - 1) % 16)) % 16; //ceil
				//diff =    - ((dpcm_tbl[i].size - 1) % 16); //floor
				dpcm_tbl[i].size += diff;
			}
			//printf("%s fixed size $%x\n", dpcm_tbl[i].fname, dpcm_tbl[i].size);
			// スタートアドレスを設定
			if( dpcm_tbl[i].index == -1 ) {
				if ( ((adr%0x4000 + dpcm_tbl[i].size) > 0x4000) || (adr % 0x4000 == 0 && adr != 0) ) {
					/* 16KB境界をまたがる場合・または前回のアドレス切り上げで新しい16KB領域に乗った場合 */
					adr += (0x4000 - (adr % 0x4000)) % 0x4000;
					bank++;
					dpcm_bankswitch = 1;
				}
				//printf("%s bank %d a %x s %x\n", dpcm_tbl[i].fname, bank, adr, size);
				
				dpcm_tbl[i].start_adr = adr;
				dpcm_tbl[i].bank_ofs = bank;
				adr += dpcm_tbl[i].size;
				size = adr;
				// adr % 64 == 0が成立するように切り上げ
				adr += (64 - (adr % 64)) % 64;
			}
		}
	}
	return size;
}





/*--------------------------------------------------------------
	DPCMデータ読み込み
 Input:
	
 Output:
--------------------------------------------------------------*/
void readDPCM( DPCMTBL dpcm_tbl[_DPCM_MAX] )
{
	int	i;
	FILE	*fp;

	for( i = 0; i < dpcm_size; i++ ) {
		dpcm_data[i] = 0xaa; 
	}

	for( i = 0; i < _DPCM_MAX; i++ ) {
		if( dpcm_tbl[i].flag != 0 && dpcm_tbl[i].index == -1 ) {
			fp = openDmc( dpcm_tbl[i].fname );
			if( fp == NULL ) {
//				disperror( DPCM_FILE_NOT_FOUND, 0 );
			} else {
				fread( &dpcm_data[dpcm_tbl[i].start_adr], 1, dpcm_tbl[i].size, fp );
				fclose( fp );
			}
		}
	}
#if DEBUG
	for( i = 0; i < _DPCM_TOTAL_SIZE; i++ ) {
		if( (i&0x0f) != 0x0f ) {
			printf( "%02x,", dpcm_data[i] );
		} else {
			printf( "%02x\n", dpcm_data[i] );
		}
	}
#endif
}



/*--------------------------------------------------------------
	音色/エンベロープのループチェック
 Input:
	
 Output:
	int	: 一番大きい音色番号
--------------------------------------------------------------*/
int checkLoop( int ptr[128][1024], int max )
{
	int		i, j, lp_flag, ret;

	ret = 0;

	for( i = 0; i < max; i++ ) {
		if( ptr[i][0] != 0 ) {
			lp_flag = 0;
			for( j = 1; j <= ptr[i][0]; j++ ) {
				if( ptr[i][j] == EFTBL_LOOP ) lp_flag = 1;
			}
			if( lp_flag == 0 ) {
				j = ptr[i][0];
				ptr[i][j+1] = ptr[i][j  ]; 
				ptr[i][j  ] = ptr[i][j-1]; 
				ptr[i][j-1] = EFTBL_LOOP;
				ptr[i][0]++;
			}
			ret = i+1;
		}
	}
	return ret;
}



/*--------------------------------------------------------------
	音色の使用個数を返す
 Input:
	
 Output:
	int	: 一番大きい音色番号
--------------------------------------------------------------*/
int getMaxTone( int ptr[128][66], int max )
{
	int		i, ret;

	ret = 0;

	for( i = 0; i < max; i++ ) {
		if( ptr[i][0] != 0 ) {
			ret = i+1;
		}
	}
	return ret;
}



/*--------------------------------------------------------------
	LFOの使用個数を返す
 Input:
	
 Output:
	int	: 一番大きいLFO番号
--------------------------------------------------------------*/
int getMaxLFO( int ptr[_PITCH_MOD_MAX][5], int max )
{
	int		i, ret;

	ret = 0;
	for( i = 0; i < max; i++ ) {
		if( ptr[i][0] != 0 ) {
			ret = i+1;
		}
	}
	return ret;
}



/*--------------------------------------------------------------
	DPCMの使用個数を返す
 Input:
	
 Output:
	int	: 一番大きい音色番号
--------------------------------------------------------------*/
int getMaxDPCM( DPCMTBL	dpcm_tbl[_DPCM_MAX] )
{
	int		i, ret = 0;

	for( i = 0; i < _DPCM_MAX; i++ ) {
		if( dpcm_tbl[i].flag != 0 ) {
			ret = i+1;
		}
	}
	return ret;
}



/*--------------------------------------------------------------
	ハードウェアエフェクトの使用個数を返す
 Input:
	
 Output:
	int	: 一番大きい音色番号
--------------------------------------------------------------*/
int getMaxHardEffect( int ptr[_HARD_EFFECT_MAX][5], int max )
{
	int		i, ret;

	ret = 0;

	for( i = 0; i < max; i++ ) {
		if( ptr[i][0] != 0 ) {
			ret = i+1;
		}
	}
	return ret;
}



/*--------------------------------------------------------------
	エフェクト波形の使用個数を返す
 Input:
	
 Output:
	int	: 一番大きい音色番号
--------------------------------------------------------------*/
int getMaxEffectWave( int ptr[_EFFECT_WAVE_MAX][33], int max )
{
	int		i, ret;

	ret = 0;

	for( i = 0; i < max; i++ ) {
		if( ptr[i][0] != 0 ) {
			ret = i+1;
		}
	}
	return ret;
}



/*--------------------------------------------------------------
	音色/エンベロープの書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeTone( FILE *fp, int tbl[128][1024], char *str, int max )
{
	int		i, j, x;

	fprintf( fp, "%s_table:\n", str );
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\tdw\t%s_%03d\n", str, i );
			} else {
				fprintf( fp, "\tdw\t0\n" );
			}			
		}
	}
	fprintf( fp, "%s_lp_table:\n", str );
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\tdw\t%s_lp_%03d\n", str, i );
			} else {
				fprintf( fp, "\tdw\t0\n" );
			}			
		}

		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\n%s_%03d:\n", str, i );
				//fprintf( fp, ";# of param: %03d\n", tbl[i][0] );
				x = 0;
				for( j = 1; j <= tbl[i][0]; j++ ) {
					if( tbl[i][j] == EFTBL_LOOP ) {
						if( x != 0 ) fprintf( fp, "\n" );
						fprintf( fp, "%s_lp_%03d:\n", str, i );
						x = 0;
					} else if( x == 0 ) {
						fprintf( fp, "\tdb\t$%02x", tbl[i][j]&0xff );
						x++;
					} else if( x == 7 ) {
						fprintf( fp, ",$%02x\n", tbl[i][j]&0xff );
						x = 0;
					} else {
						fprintf( fp, ",$%02x", tbl[i][j]&0xff );
						x++;
					}
				}
			}
		}
	}
	fprintf( fp, "\n\n" );
}



/*--------------------------------------------------------------
	FM音色の書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeToneFM( FILE *fp, int tbl[_FM_TONE_MAX][66], char *str, int max )
{
	int		i, j, x;

	fprintf( fp, "%s_data_table:\n", str );
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\tdw\t%s_%03d\n", str, i );
			} else {
				fprintf( fp, "\tdw\t0\n" );
			}
		}

		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\n%s_%03d:\n", str, i );
				x = 0;
				for( j = 1; j <= tbl[i][0]; j++ ) {				// tbl[i][0] = データー量(byte)
					if( x == 0 ) {
						fprintf( fp, "\tdb\t$%02x", tbl[i][j]&0xff );
						x++;
					} else if( x == 7 ) {
						fprintf( fp, ",$%02x\n", tbl[i][j]&0xff );
						x = 0;
					} else {
						fprintf( fp, ",$%02x", tbl[i][j]&0xff );
						x++;
					}
				}
			}
		}
	}
}

/*--------------------------------------------------------------
	VRC7音色の書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeToneVRC7( FILE *fp, int tbl[_VRC7_TONE_MAX][66], char *str, int max )
{
	int		i, j, x;

	fprintf( fp, "%s_data_table:\n", str );
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\tdw\t%s_%03d\n", str, i );
			} else {
				fprintf( fp, "\tdw\t0\n" );
			}
		}

		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\n%s_%03d:\n", str, i );
				x = 0;
				for( j = 1; j <= tbl[i][0]; j++ ) {				// tbl[i][0] = データー量(byte)
					if( x == 0 ) {
						fprintf( fp, "\tdb\t$%02x", tbl[i][j]&0xff );
						x++;
					} else if( x == 7 ) {
						fprintf( fp, ",$%02x\n", tbl[i][j]&0xff );
						x = 0;
					} else {
						fprintf( fp, ",$%02x", tbl[i][j]&0xff );
						x++;
					}
				}
			}
		}
	}

	fprintf( fp, "\n\n" );
}




/*--------------------------------------------------------------
	ハードウェアエフェクトの書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeHardEffect( FILE *fp, int tbl[_HARD_EFFECT_MAX][5], char *str, int max )
{
	int		i;

	fprintf( fp, "%s_effect_select:\n", str );
	for ( i = 0; i < max; i++ ) {
		fprintf( fp, "\tdb\t$%02x,$84,$%02x,$85,$00,$87,$80,$88\n",
			tbl[i][1], (tbl[i][3] | 0x80) );
		fprintf( fp, "\tdb\t$%02x,$86,$%02x,$87,$%02x,$ff,$00,$00\n",
			tbl[i][4], (tbl[i][2] & 0x00FF), ((tbl[i][2] & 0x0F00)>>8) );
	}
}

/*--------------------------------------------------------------
	エフェクト波形の書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeEffectWave( FILE *fp, int tbl[_EFFECT_WAVE_MAX][33], char *str, int max )
{
	int		i, j, x;

	fprintf( fp, "%s_4088_data:\n", str );
	for( i = 0; i < max; i++ ) {
		if( tbl[i][0] != 0 ) {
			x = 0;
			for( j = 1; j <= tbl[i][0]; j++ ) {				// tbl[i][0] = データー量(byte)
				if( x == 0 ) {
					fprintf( fp, "\tdb\t$%02x", tbl[i][j]&0xff );
					x++;
				} else if( x == 7 ) {
					fprintf( fp, ",$%02x\n", tbl[i][j]&0xff );
					x = 0;
				} else {
					fprintf( fp, ",$%02x", tbl[i][j]&0xff );
					x++;
				}
			}
		}
		else
		{
			/* ダミーデータを出力 */
			for (j = 0; j < 4; j++) {
				fprintf( fp, "\tdb\t$00,$00,$00,$00,$00,$00,$00,$00\n");
			}
		}
	}
	
	fprintf( fp, "\n\n" );
}



/*--------------------------------------------------------------
	N106音色の書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeToneN106( FILE *fp, int tbl[_N106_TONE_MAX][2+64], char *str, int max )
{
	int		i, j, x;

	// 使用チャンネル書き込み
	fprintf( fp, "%s_channel:\n", str );
	fprintf( fp, "\tdb\t%d\n", n106_track_num );
	// パラメータ書き込み
	fprintf( fp, "%s_wave_init:\n", str );
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			switch( tbl[i][0] ) {
			  case  2*2+1: j = 7; x = tbl[i][1]* 2*2; break;
			  case  4*2+1: j = 6; x = tbl[i][1]* 4*2; break;
			  case  6*2+1: j = 5; x = tbl[i][1]* 6*2; break;
			  case  8*2+1: j = 4; x = tbl[i][1]* 8*2; break;
			  case 10*2+1: j = 3; x = tbl[i][1]*10*2; break;
			  case 12*2+1: j = 2; x = tbl[i][1]*12*2; break;
			  case 14*2+1: j = 1; x = tbl[i][1]*14*2; break;
			  case 16*2+1: j = 0; x = tbl[i][1]*16*2; break;
			  default:     j = 0; x = 0; break;	
			}
			fprintf( fp, "\tdb\t$%02x,$%02x\n", j, x );
		}
	}
	// パラメータ書き込み
	fprintf( fp, "%s_wave_table:\n", str );
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "\tdw\t%s_wave_%03d\n", str, i );
			} else {
				fprintf( fp, "\tdw\t0\n" );
			}
		}
	}			
	if( max != 0 ) {
		for( i = 0; i < max; i++ ) {
			if( tbl[i][0] != 0 ) {
				fprintf( fp, "%s_wave_%03d:\n", str, i );
				fprintf( fp, "\tdb\t" );
				for( j = 0; j < tbl[i][0]/2-1; j++ ) {
					fprintf( fp, "$%02x,", (tbl[i][2+(j*2)+1]<<4)+tbl[i][2+(j*2)+0] );
				}
				fprintf( fp, "$%02x\n", (tbl[i][2+(j*2)+1]<<4)+tbl[i][2+(j*2)+0] );
			}
		}
	}
	fprintf( fp, "\n\n" );
}



/*--------------------------------------------------------------
	DPCMテーブルの書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeDPCM( FILE *fp, DPCMTBL dpcm_tbl[_DPCM_MAX], char *str, int max )
{
	int		i;
	int		freq,adr,size,delta_init;
	char		*fname;

	fprintf( fp, "%s:\n", str );
	for( i = 0; i < max; i++ ) {
		if( dpcm_tbl[i].flag != 0 ) {
			/*
			   $4013 * 16 + 1 = size
			   $4013 = (size - 1) / 16
			   $4012 * 64 = adr
			   adr = $4012 / 64
			*/
			freq = dpcm_tbl[i].freq;
			size = (dpcm_tbl[i].size - 1)/16;
			delta_init = dpcm_tbl[i].delta_init;
			if( dpcm_tbl[i].index == -1 ) {
				adr  = dpcm_tbl[i].start_adr/64;
				fname = dpcm_tbl[i].fname;
			} else {
				adr  = dpcm_tbl[dpcm_tbl[i].index].start_adr/64;
				fname = dpcm_tbl[dpcm_tbl[i].index].fname;
			}
			fprintf( fp, "\tdb\t$%02x,$%02x,$%02x,$%02x\t;%s\n", freq, delta_init, adr % 0x100, size, fname);
		} else {
			fprintf( fp, "\tdb\t$00,$00,$00,$00\t;unused\n");
		}
	}
	
	if (dpcm_bankswitch) {
		fprintf( fp, "%s_bank:\n", str );
		for( i = 0; i < max; i++ ) {
			int bank_ofs = 0;
			if( dpcm_tbl[i].flag != 0 ) {
				if( dpcm_tbl[i].index == -1 ) {
					bank_ofs = dpcm_tbl[i].bank_ofs;
					fname = dpcm_tbl[i].fname;
				} else {
					bank_ofs = dpcm_tbl[dpcm_tbl[i].index].bank_ofs;
					fname = dpcm_tbl[dpcm_tbl[i].index].fname;
				}
				if (bank_ofs == 0) {
					fprintf( fp, "\tdb\t2*2\t;%s\n", fname);
				} else {
					bank_ofs -= 1;
					fprintf( fp, "\tdb\t(DPCM_EXTRA_BANK_START + %d*2)*2\t;%s\n", bank_ofs, fname);
				}
			} else {
				fprintf( fp, "\tdb\t0\t;unused\n");
			}
		}
	}

	fprintf( fp, "\n" );
}




/*--------------------------------------------------------------
	
--------------------------------------------------------------*/
static void writeDPCMSampleSub( FILE *fp )
{

	fprintf( fp, "\t.org\t$FFFA\n");
	fprintf( fp, "\t.dw\tDMC_NMI\n");
	fprintf( fp, "\t.dw\tDMC_RESET\n");
	fprintf( fp, "\t.dw\tDMC_IRQ\n");
}

/*--------------------------------------------------------------
	DPCMデータの書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeDPCMSample( FILE *fp )
{
	int		i;
	int	nes_bank = 1; //8KB
	int	bank_ofs = 0; //16KB
	
	fprintf( fp, "; begin DPCM samples\n" );
	for( i = 0; i < dpcm_size; i++ ) {
		if (i % 0x2000 == 0) {
			nes_bank++;
			if (nes_bank == 4) {
				nes_bank = 2;
				bank_ofs++;
			}
			if (bank_ofs == 0) {
				fprintf( fp, "\t.bank\t%1d\n", nes_bank );
				putBankOrigin(fp, nes_bank);
			} else {
				fprintf( fp, "\t.bank\tDPCM_EXTRA_BANK_START + %d*2 + %d - 2\n", bank_ofs-1, nes_bank );
				dpcm_extra_bank_num++;
				fprintf(fp, "\t.org\t$%04x\n", 0x8000 + 0x2000*nes_bank);
			}
		}
		if( (i&0x0f) == 0x00 ) {
			fprintf( fp, "\tdb\t$%02x", dpcm_data[i] );
		}else if( (i&0x0f) != 0x0f ) {
			fprintf( fp, ",$%02x", dpcm_data[i] );
		} else {
			fprintf( fp, ",$%02x\n", dpcm_data[i] );
		}
		if (bank_ofs == 0) {
			bank_usage[nes_bank]++;
		}
	}
	fprintf( fp, "\n" );
	fprintf( fp, "; end DPCM samples\n\n" );
	
	if (dpcm_extra_bank_num) {
		int x;
		fprintf( fp, "; begin DPCM vectors\n" );
		fprintf( fp, "\t.bank\t3\n");
		writeDPCMSampleSub(fp);
		for (x = 2; x <= dpcm_extra_bank_num; x += 2) {
			fprintf( fp, "\t.bank\tDPCM_EXTRA_BANK_START + %d\n", x - 1);
			writeDPCMSampleSub(fp);
		}
		fprintf( fp, "; end DPCM vectors\n" );
	}
	fprintf( fp, "\n" );
}



/*--------------------------------------------------------------
	タイトル/作曲者/メーカー/打ち込み者をコメントとして書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeSongInfo( FILE *fp )
{
	fprintf( fp, "; Title: %s\n", song_name );
	fprintf( fp, "; Composer: %s\n", composer );
	fprintf( fp, "; Maker: %s\n", maker );
	if (programer != NULL) {
		fprintf( fp, "; Programer: %s\n", programer);
	}
	fprintf(fp, "\n");
}



/*--------------------------------------------------------------
	
 Input: 文字列データをdbとしてmaxバイト出力(終端以降は0で埋める)
	
 Output:
	
--------------------------------------------------------------*/
void printStrDb( FILE *fp, const char *str, int max)
{
	int i, end_flag = 0, newline_flag = 0;
	char c;
	for (i = 0; i < max; i++) {
		c = *(str + i);
		if (c == '\0') {
			end_flag = 1;
		}
		if (end_flag) {
			c = '\0';
		}
		switch (i % 8) {
			case 0:
				newline_flag = 0;
				fprintf(fp, "\tdb\t$%02x", c & 0xff);
				break;
			case 7:
				newline_flag = 1;
				fprintf(fp, ", $%02x",  c & 0xff);
				break;
			default:				
				fprintf(fp, ", $%02x",  c & 0xff);
				break;
		}
		if (newline_flag) {
			fprintf(fp, "\n");
		}
	}
	if (!newline_flag) {
		fprintf(fp, "\n");
	}
	
}



/*--------------------------------------------------------------
	タイトル/作曲者/メーカーをmacroとして書き込み
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void writeSongInfoMacro(FILE *fp)
{
	fprintf( fp, "TITLE\t.macro\n");
	printStrDb(fp, song_name, 32);
	fprintf(fp, "\t.endm\n");
	fprintf( fp, "COMPOSER\t.macro\n");
	printStrDb(fp, composer, 32);
	fprintf(fp, "\t.endm\n");
	fprintf( fp, "MAKER\t.macro\n");
	printStrDb(fp, maker, 32);
	fprintf(fp, "\t.endm\n");
}





/*--------------------------------------------------------------
	パラメータがn個のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBuf( int n, CMD *cmd, int com_no, char *ptr, int line, int enable )
{
	int		cnt, i;
	int		param[PARAM_MAX];

	for( i = 0; i < PARAM_MAX; i++ ) {
		param[i] = 0;
	}

	if( n != 0 ) {
		for(i = 0; i < n; i++) 
		{
			cnt = 0;
			param[i] = Asc2Int( ptr, &cnt );
			if( cnt == 0 ) {		/* パラメータが無い場合はエラーの出る数値に書き換える */
				param[i] = PARAM_OMITTED;
			}
			ptr += cnt;

			if (i < n-1) // nが2個以上のときは","の処理が入る
			{

				ptr = skipSpace( ptr );
				if( *ptr == ',' ) {
					ptr++;
					ptr = skipSpace( ptr );
				}
				else //  ","の区切りがない場合、パラメータは省略されている
				{
					for(i++; i<n; i++ ) // 現在の次のパラメータから省略
						param[i] = PARAM_OMITTED;
				}
			}
		}
	}

	if( enable != 0 ) {
		cmd->cnt = 0;
		cmd->line = line;
		cmd->cmd  = com_no;
		cmd->len = 0;
		if( n != 0 ) {
			for( i = 0; i < n; i++ ) {
				cmd->param[i] = param[i];
			}
		}
	}

	return ptr;
}



/*--------------------------------------------------------------
	音長パラメータの取得
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *getLengthSub( char *ptr, double *len, double def )
{
	int		cnt;
	double 	temp;

	/* フレーム指定 */
	if( *ptr == '#' ) {
		ptr++;
		*len = Asc2Int( ptr, &cnt );
		if( cnt != 0 ) {
			ptr += cnt;
			*len = *len / tbase;
		} else {
			*len = -1;
		}
	/* カウント指定 */
	} else if( *ptr == '%' ) {
		ptr++;
		*len = Asc2Int( ptr, &cnt );
		if( cnt != 0 ) {
			ptr += cnt;
		} else {
			*len = -1;
		}
	/* 音楽的音長指定 */
	} else {
		*len = Asc2Int( ptr, &cnt );
		if( cnt != 0 ) {
			ptr += cnt;
			if (*len > 0)
				*len = _BASE/(*len);
		} else {
			/* パラメータが無い場合はエラーの出る数値に書き換える */
			*len = def;
		}
		/* エラー/lコマンドの時は処理させない */
		if( *len != -1 ) {
			/* 符点の処理(複数可能に) */
			temp = *len;
			while( *ptr == '.' ) {
				temp /= 2;
				*len += temp;
				ptr++;
			}
		}
	}
	return ptr;
}


/*--------------------------------------------------------------
	音長取得
 Output:
	*len: 
--------------------------------------------------------------*/
char *getLength( char *ptr, double *len, double def )
{
	ptr = getLengthSub(ptr, len, def);
	/* 音長減算(一回だけ可能) */
	if (*ptr == '-' || *ptr == '~') {
		double len_adjust;
		ptr++;
		ptr = getLengthSub(ptr, &len_adjust, def);
		if (*len - len_adjust > 0) {
			*len = *len - len_adjust;
		} else {
			//dispError();呼び出し元でエラー捕捉
			*len = *len - len_adjust;
		}
	}
	return ptr;
}



/*--------------------------------------------------------------
	パラメータが1個(音長)のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBufL( CMD *cmd, int com_no, char *ptr, int line, int enable )
{
	cmd->cnt = 0;
	cmd->line = line;
	cmd->cmd  = com_no;

	ptr = getLength( ptr, &(cmd->len), -1 );
	if( cmd->len > 0 ) {
		if( enable != 0 ) {
			length = cmd->len;
		}
	} else {
		dispError( ABNORMAL_NOTE_LENGTH_VALUE, cmd->filename, line );
	}

	return ptr;
}



/*--------------------------------------------------------------
	パラメータが1個(音階/音長)のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBufN( CMD *cmd, int com_no, char *ptr, int line, int enable )
{
	int		oct_ofs, note;
	double	len;

	com_no += transpose;

	/* c+-++-++--とかも出来るように対策(普通しないけど) */
	while( 1 ) {
		if( *ptr == '+' ) {
			com_no++;
			ptr++;
		} else if( *ptr == '-' ) {
			com_no--;
			ptr++;
		} else {
			break;
		}
	}
	/* オクターブをまたぐ時の補正処理 */
	oct_ofs = 0;
	while( com_no < _NOTE_C ) {
		com_no += 12;
		oct_ofs--;
	}
	while( com_no > _NOTE_B ) {
		com_no -= 12;
		oct_ofs++;
	}

	note = ((octave+oct_ofs)<<4)+com_no;
	/* 音階の範囲チェック */
	if( note < 0 ) {
		switch( note ) {
		  case -5: note = 15; break;
		  case -6: note = 14; break;
		  case -7: note = 13; break;
		  default: note =  0; break;
		}
	} else if( note > MAX_NOTE ) {
		note = MAX_NOTE;
	}

	ptr = getLength( ptr, &len, length );
	if (len <= 0) {
		dispError( ABNORMAL_NOTE_LENGTH_VALUE, cmd->filename, line );
		len = 0.0;
	}
	if( enable != 0 ) {
		cmd->cnt  = 0;
		cmd->line = line;
		cmd->cmd  = note;
		cmd->len  = len;
	}

	return ptr;
}



/*--------------------------------------------------------------
	パラメータが1個(音階(直接指定)/音長)のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBufN0( CMD *cmd, char *ptr, int line, int enable )
{
	int		cnt, note;
	double	len;

	cnt  = 0;
	note = Asc2Int( ptr, &cnt );
	if( cnt == 0 ) {
		dispError( ABNORMAL_PITCH_VALUE, cmd->filename, line );
		return ptr+1;
	}
	ptr += cnt;

	// 音階の範囲チェック
	if( note < 0 ) {
		note = 0;
	} else if( note > MAX_NOTE ) {
		note = MAX_NOTE;
	}

	ptr = skipSpace( ptr );				// 余分なスペースをスキップ
	// ","があるときは音長が存在する
	if( *ptr == ',' ) {
		ptr++;
		ptr = skipSpace( ptr );			// 余分なスペースをスキップ

		ptr = getLength( ptr, &len, length );
		if (len <= 0) {
			dispError( ABNORMAL_NOTE_LENGTH_VALUE, cmd->filename, line );
			len = 0.0;
		}
	// ","がないときはデフォルトの音長を使用する
	} else {
		len = length;
	}

	if( enable != 0 ) {
		cmd->cnt  = 0;
		cmd->line = line;
		cmd->cmd  = note;
		cmd->len  = len;
	}

	return ptr;
}



/*--------------------------------------------------------------
	パラメータが1個(周波数(直接指定)/音長)のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBufN1( CMD *cmd, int com_no, char *ptr, int line, int enable )
{
	int		cnt, freq;
	double	len;

	cnt = 0;
	freq = Asc2Int( ptr, &cnt );
	// 文字数チェック
	if( cnt == 0 ) {
		dispError( ABNORMAL_PITCH_VALUE, cmd->filename, line );
		return ptr+1;
	}
	ptr += cnt;
	// パラメータ範囲チェック
	if( 0x0008 > freq && freq >= 0x07f2 ) {
		dispError( ABNORMAL_PITCH_VALUE, cmd->filename, line );
		return ptr+1;
	}
	// "," があるときは音長取得
	ptr = skipSpace( ptr );
	if( *ptr == ',' ) {
		ptr++;
		ptr = skipSpace( ptr );
		ptr = getLength( ptr, &len, length );
		if (len <= 0) {
			dispError( ABNORMAL_NOTE_LENGTH_VALUE, cmd->filename, line );
			len = 0.0;
		}
	// "," がないときはデフォルト音長に
	} else {
		len = length;
	}

	if( enable != 0 ) {
		cmd->cnt      = 0;
		cmd->line     = line;
		cmd->cmd      = com_no;
		cmd->len      = len;
		cmd->param[0] = freq;
	}

	return ptr;
}



/*--------------------------------------------------------------
	パラメータが1個(休符/音長)のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBufR( CMD *cmd, int com_no, char *ptr, int line, int enable )
{
	double	len;

	ptr = getLength( ptr, &len, length );
	if (len <= 0) {
		dispError( ABNORMAL_NOTE_LENGTH_VALUE, cmd->filename, line );
		len = 0.0;
	}
	
	if( enable != 0 ) {
		cmd->cnt = 0;
		cmd->line = line;
		cmd->cmd = com_no;
		cmd->len = len;
	}

	return ptr;
}


/*--------------------------------------------------------------
	パラメータが1個(キーオフ/音長)のコマンドの処理
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
char *setCommandBufK( CMD *cmd, int com_no, char *ptr, int line, int enable )
{
	double	len;

	ptr = getLength( ptr, &len, length );
	if (len < 0) { /* 音長0あり */
		dispError( ABNORMAL_NOTE_LENGTH_VALUE, cmd->filename, line );
		len = 0.0;
	}
	
	if( enable != 0 ) {
		cmd->cnt = 0;
		cmd->line = line;
		cmd->cmd = com_no;
		cmd->len = len;
	}

	return ptr;
}




/*--------------------------------------------------------------
	
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
CMD * analyzeData( int trk, CMD *cmd, LINE *lptr )
{
	int		i, line, com, cnt;
	char	*ptr;

	typedef struct {
		char	*cmd;
		int		num;
		unsigned long		enable;
	} MML;
	const MML mml[] = {
		{ "c", _NOTE_C,			(ALLTRACK) },
		{ "d", _NOTE_D,			(ALLTRACK) },
		{ "e", _NOTE_E,			(ALLTRACK) },
		{ "f", _NOTE_F,			(ALLTRACK) },
		{ "g", _NOTE_G,			(ALLTRACK) },
		{ "a", _NOTE_A,			(ALLTRACK) },
		{ "b", _NOTE_B,			(ALLTRACK) },
		{ "@n", _KEY,			(ALLTRACK&~DPCMTRACK&~VRC7TRACK&~N106TRACK) },
		{ "n", _NOTE,			(ALLTRACK) },
		{ "w", _WAIT,			(ALLTRACK) },
		{ "@t", _TEMPO2,		(ALLTRACK) },
		{ "t", _TEMPO,			(ALLTRACK) },
		{ "o", _OCTAVE,			(ALLTRACK) },
		{ ">", _OCT_UP,			(ALLTRACK) },
		{ "<", _OCT_DW,			(ALLTRACK) },
		{ "l", _LENGTH,			(ALLTRACK) },
		{ "v+", _VOL_PLUS,		(ALLTRACK&~DPCMTRACK) },
		{ "v-", _VOL_MINUS,		(ALLTRACK&~DPCMTRACK) },
		{ "v", _VOLUME,			(ALLTRACK&~DPCMTRACK) },
		{ "NB", _NEW_BANK,		(ALLTRACK) },
		{ "EPOF", _EP_OFF,		(ALLTRACK&~DPCMTRACK) },
		{ "EP",   _EP_ON,		(ALLTRACK&~DPCMTRACK) },
		{ "ENOF", _EN_OFF,		(ALLTRACK&~DPCMTRACK) },
		{ "EN",   _EN_ON,		(ALLTRACK&~DPCMTRACK) },
		{ "MPOF", _LFO_OFF,		(ALLTRACK&~DPCMTRACK) },
		{ "MP",   _LFO_ON,		(ALLTRACK&~DPCMTRACK) },

		{ "SMOF", _SMOOTH_OFF,		(TRACK(0)|TRACK(1)|TRACK(2)) },
		{ "SM", _SMOOTH_ON,		(TRACK(0)|TRACK(1)|TRACK(2)) },

		{ "PS", _PITCH_SHIFT,		(TRACK(0)|TRACK(1)|TRACK(2)) },

		{ "EH",	_HARD_ENVELOPE,		(TRACK(0)|TRACK(1)|NOISETRACK|FMTRACK|MMC5TRACK) },
		{ "MHOF", _MH_OFF,		(FMTRACK) },
		{ "MH",	  _MH_ON,		(FMTRACK) },
		{ "OP",   _VRC7_TONE,		(VRC7TRACK) },
		{ "SDQR", _SELF_DELAY_QUEUE_RESET,	(ALLTRACK&~TRACK(2)&~DPCMTRACK) },
		{ "SDOF", _SELF_DELAY_OFF,	(ALLTRACK&~TRACK(2)&~DPCMTRACK) },
		{ "SD", _SELF_DELAY_ON,		(ALLTRACK&~TRACK(2)&~DPCMTRACK) },
		{ "SA", _SHIFT_AMOUNT,		(N106TRACK) },
		{ "D", _DETUNE,			(ALLTRACK&~DPCMTRACK) },
		{ "K", _TRANSPOSE,		(ALLTRACK&~NOISETRACK&~DPCMTRACK) },
		{ "M", _SUN5B_HARD_SPEED,	(FME7TRACK) },
		{ "S", _SUN5B_HARD_ENV,	(FME7TRACK) },
		{ "N", _SUN5B_NOISE_FREQ,	(FME7TRACK) },
		{ "@q", _QUONTIZE2,		(ALLTRACK) },
		{ "@vr", _REL_ENV,		(ALLTRACK&~TRACK(2)&~DPCMTRACK) }, 
		{ "@v", _ENVELOPE,		(ALLTRACK&~TRACK(2)&~DPCMTRACK) },
		{ "@@r", _REL_ORG_TONE,		(TRACK(0)|TRACK(1)|FMTRACK|VRC7TRACK|VRC6PLSTRACK|N106TRACK|MMC5PLSTRACK) },
		{ "@@", _ORG_TONE,		(TRACK(0)|TRACK(1)|FMTRACK|VRC7TRACK|VRC6PLSTRACK|N106TRACK|MMC5PLSTRACK) },
		{ "@", _TONE,			(TRACK(0)|TRACK(1)|VRC6PLSTRACK|MMC5PLSTRACK|FME7TRACK) },
		{ "s", _SWEEP,			(TRACK(0)|TRACK(1)|FMTRACK) },
		{ "&", _SLAR,			(ALLTRACK) },
		{ "y", _DATA_WRITE,		(ALLTRACK) },
		{ "x", _DATA_THRUE,		(ALLTRACK) },

		{ "|:", _REPEAT_ST2,		(ALLTRACK) },
		{ ":|", _REPEAT_END2,		(ALLTRACK) },
		{ "\\", _REPEAT_ESC2,		(ALLTRACK) },




#if 0
		{ "SQOF", _SHUFFLE_QUONTIZE_OFF,	(ALLTRACK) },
		{ "SQR", _SHUFFLE_QUONTIZE_RESET,(ALLTRACK) },
		{ "SQ", _SHUFFLE_QUONTIZE,	(ALLTRACK) },
		{ "XX", _XX_COMMAND,		(ALLTRACK) },
#endif
//		{ "'", _ARTICULATION_ADJUST,	(ALLTRACK) },
		{ "k", _KEY_OFF,		(ALLTRACK) },
		
		{ "L", _SONG_LOOP,		(ALLTRACK) },
		{ "[", _REPEAT_ST,		(ALLTRACK) },
		{ "]", _REPEAT_END,		(ALLTRACK) },
		{ "|", _REPEAT_ESC,		(ALLTRACK) },
		{ "{", _CONT_NOTE,		(ALLTRACK) },
		{ "}", _CONT_END,		(ALLTRACK) },
		{ "q", _QUONTIZE,		(ALLTRACK) },
		{ "r", _REST,			(ALLTRACK) },
		{ "^", _TIE,			(ALLTRACK) },
		{ "!", _DATA_BREAK,		(ALLTRACK) },
		{ "",  _TRACK_END,		(ALLTRACK) },
	};

	cnt = 0;

	transpose = 0;

	for( line = 1; line <= lptr->line; line++ ) {
		if( (lptr[line].status == _TRACK) && ((lptr[line].param&(1<<trk)) != 0) ) {
			ptr = lptr[line].str;
			while( *ptr != '\0' ) {
				ptr = skipSpace( ptr );			// 余分なスペースをスキップ
				if( *ptr == '\0' ) break;		// このラインは終わり？
				// コマンドを検索する
				for( i = 0; mml[i].num != _TRACK_END; i++ ) {
					if( strncmp( mml[i].cmd, ptr, strlen(mml[i].cmd) ) == 0 ) break;
				}
				ptr += strlen(mml[i].cmd);		// コマンドの文字数だけ文字をスキップ
				cmd->filename = lptr[line].filename;	// エラー出力時のファイル名取得
				switch( mml[i].num ) {
				/* オクターブ */
				  case _OCTAVE:
					com = Asc2Int( ptr, &cnt );
					if( cnt != 0 ) {
						// コマンドは有効の時は処理を登録
						if( (mml[i].enable&(1<<trk)) != 0 ) {
							if( trk == BTRACK(0) || trk == BTRACK(1) || trk == BTRACK(2) ) {
								octave = com-2;
							} else {
								octave = com;
							}
						}
						ptr += cnt;
					}
					break;
				/* オクターブアップ */
				  case _OCT_UP:
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( octave_flag == 0 ) { octave++; } else { octave--; }
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* オクターブダウン */
				  case _OCT_DW:
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( octave_flag == 0 ) { octave--; } else { octave++; }
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* 音長設定 */
				  case _LENGTH:
					ptr = setCommandBufL( cmd, _LENGTH, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* 音符(nコマンド) */
				  case _NOTE:
					ptr = setCommandBufN0( cmd, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* 音符(@nコマンド) */
				  case _KEY:
					ptr = setCommandBufN1( cmd, _KEY, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* 音符 */
				  case _NOTE_C:
				  case _NOTE_D:
				  case _NOTE_E:
				  case _NOTE_F:
				  case _NOTE_G:
				  case _NOTE_A:
				  case _NOTE_B:
					ptr = setCommandBufN( cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* 休符/連符 */
				  case _REST:
				  case _CONT_END:
				  case _TIE:
				  case _WAIT:
					ptr = setCommandBufR( cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				 /* キーオフ */
				  case _KEY_OFF:
					ptr = setCommandBufK( cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				/* コマンドパラメータが0個の物 */
				  case _PITCH_SHIFT:		/* ピッチシフト */
				  case _SLAR:			/* スラー */
				  case _SONG_LOOP:			/* 曲ループ */
				  case _REPEAT_ST:		/* リピート(現状では展開する) */
				  case _REPEAT_ESC:		/* リピート途中抜け */
				  case _CONT_NOTE:		/* 連符開始 */
				  case _LFO_OFF:
				  case _EP_OFF:
				  case _EN_OFF:
				  case _MH_OFF:
				  case _SMOOTH_ON:
				  case _SMOOTH_OFF:
				  case _REPEAT_ST2:		/* リピート2 */
				  case _REPEAT_ESC2:	/* リピート途中抜け2 */
//				  case _SHUFFLE_QUONTIZE_RESET:
//				  case _SHUFFLE_QUONTIZE_OFF:
				  case _SELF_DELAY_OFF:
				  case _SELF_DELAY_QUEUE_RESET:
					if (mml[i].num == _SLAR && ( NOSLAR_TRACK & (1<<trk) ))
						ptr = setCommandBufR( cmd, _TIE, ptr, line, mml[i].enable&(1<<trk) );
					else
						setCommandBuf( 0, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );

					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;					
				/* コマンドパラメータが1個の物 */
				  case _TEMPO:			/* テンポ */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] <= 0 ) {
							dispError( ABNORMAL_TEMPO_VALUE, lptr[line].filename, line );
							cmd->cmd = _NOP;
						} else {
							tbase = (double)_BASETEMPO/(double)cmd->param[0];
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _TONE:			/* 音色切り替え */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						//vrc6用に制限を外す(内蔵矩形波、MMC5は@3まで)
						//if( cmd->param[0] < 0 || cmd->param[0] > 3 ) {
						if( cmd->param[0] < 0 || cmd->param[0] > 7 ) {
							dispError( ABNORMAL_TONE_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _REL_ORG_TONE:		/* リリース音色 */
				  case _ORG_TONE:		/* 音色切り替え */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if ((mml[i].num == _REL_ORG_TONE) && (cmd->param[0] == 255)) {
							//ok
						} else if ( cmd->param[0] < 0 || cmd->param[0] > 127 ) {
							dispError( ABNORMAL_TONE_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _ENVELOPE:		/* エンベロープ指定 */
					cmd->filename = lptr[line].filename;
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] == 255) { 
							volume_flag = 0x0000; 
						} else if( 0 <= cmd->param[0] && cmd->param[0] <= 127) {
							volume_flag = 0x8000; 
						} else {
							dispError( ABNORMAL_ENVELOPE_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						} 
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line ); 
					} 
					break; 
				  case _REL_ENV:		/* リリースエンベロープ指定 */ 
					cmd->filename = lptr[line].filename; 
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) ); 
					if( (mml[i].enable&(1<<trk)) != 0 ) { 
						if( cmd->param[0] == 255 ) { 
							volume_flag = 0x0000; 
						} else if( 0 <= cmd->param[0] && cmd->param[0] <= 127) {
							volume_flag = 0x8000;
						} else {
							dispError( ABNORMAL_ENVELOPE_NUMBER, lptr[line].filename, line ); 
							cmd->cmd = 0; 
							cmd->line = 0; 
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _VOL_PLUS:		/* 音量指定 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] == PARAM_OMITTED ) {
							cmd->param[0] = 1;
						}
						if( ((1<<trk) & ~(FMTRACK|VRC6SAWTRACK) && 0 <= volume_flag && volume_flag <= 15)
						 || ((1<<trk) & (FMTRACK|VRC6SAWTRACK) && 0 <= volume_flag && volume_flag <= 63) ) {
							cmd->cmd = _VOLUME;
							cmd->param[0] = volume_flag+cmd->param[0];
							if( ((1<<trk) & ~(FMTRACK|VRC6SAWTRACK) && (cmd->param[0] < 0 || cmd->param[0] > 15))
							 || ((1<<trk) & (FMTRACK|VRC6SAWTRACK) && (cmd->param[0] < 0 || cmd->param[0] > 63)) ) {
								dispError( VOLUME_RANGE_OVER_OF_RELATIVE_VOLUME, lptr[line].filename, line );
								cmd->cmd = 0;
								cmd->line = 0;
							} else {
								volume_flag = cmd->param[0];
							}
						} else {
							dispError( RELATIVE_VOLUME_WAS_USED_WITHOUT_SPECIFYING_VOLUME, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _VOL_MINUS:		/* 音量指定 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] == PARAM_OMITTED ) {
							cmd->param[0] = 1;
						}
						if( ((1<<trk) & ~(FMTRACK|VRC6SAWTRACK) && 0 <= volume_flag && volume_flag <= 15)
						 || ((1<<trk) & (FMTRACK|VRC6SAWTRACK) && 0 <= volume_flag && volume_flag <= 63) ) {
							cmd->cmd = _VOLUME;
							cmd->param[0] = volume_flag-cmd->param[0];
							if( cmd->param[0] < 0 ) {
								dispError( VOLUME_RANGE_UNDER_OF_RELATIVE_VOLUME, lptr[line].filename, line );
								cmd->cmd = 0;
								cmd->line = 0;
							} else {
								volume_flag = cmd->param[0];
							}
						} else {
							dispError( RELATIVE_VOLUME_WAS_USED_WITHOUT_SPECIFYING_VOLUME, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _VOLUME:			/* 音量指定 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( ((1<<trk) & ~(FMTRACK|VRC6SAWTRACK) && (cmd->param[0] < 0 || cmd->param[0] > 15))
						 || ((1<<trk) & (FMTRACK|VRC6SAWTRACK) && (cmd->param[0] < 0 || cmd->param[0] > 63)) ) {
							dispError( ABNORMAL_VOLUME_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						} else {
							volume_flag = cmd->param[0];
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _HARD_ENVELOPE:
					ptr = setCommandBuf( 2, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if ( (1<<trk) & (TRACK(0)|TRACK(1)) )
						{
						if( (cmd->param[0] < 0 || cmd->param[0] >  1)
						 && (cmd->param[1] < 0 || cmd->param[1] >  1) ) {
							dispError( ABNORMAL_ENVELOPE_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}

						}
						else
						{
						if( (cmd->param[0] < 0 || cmd->param[0] >  1)
						 && (cmd->param[1] < 0 || cmd->param[1] > 63) ) {
							dispError( ABNORMAL_ENVELOPE_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						} else {
							volume_flag = 0x8000;
						}
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _QUONTIZE:		/* クオンタイズ(length*n/gate_denom) */
					ptr = setCommandBuf( 2, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if ( cmd->param[1] == PARAM_OMITTED ) {
							cmd->param[1] = 0;
						}
						if (   cmd->param[0] < 0
						     ||cmd->param[0] > gate_denom
						     ||(cmd->param[0] == 0 && cmd->param[1] <= 0)
						     ||(cmd->param[0] == gate_denom && cmd->param[1] > 0) ) {
							dispError( ABNORMAL_QUANTIZE_VALUE,  lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _QUONTIZE2:		/* クオンタイズ(length-n) */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
#if 0
				  case _SHUFFLE_QUONTIZE:	/* シャッフルクオンタイズ設定 */
					ptr = setCommandBuf( 3, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if (   cmd->param[0] <= 0
						     ||cmd->param[1] <= 0
						     ||cmd->param[2] <= 0
						     ||cmd->param[0] == PARAM_OMITTED
						     ||cmd->param[1] == PARAM_OMITTED
						     ||cmd->param[2] == PARAM_OMITTED  ) {
							dispError( ABNORMAL_SHUFFLE_QUANTIZE_VALUE,  lptr[line].filename, line );
							cmd->cmd = _NOP;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
#endif
				  case _LFO_ON:			/* ソフトＬＦＯ */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255) 
						 && (cmd->param[0] < 0 || cmd->param[0] > 63) ) { 
							dispError( ABNORMAL_LFO_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _EP_ON:			/* ピッチエンベロープ */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255) 
						 && (cmd->param[0] < 0 || cmd->param[0] > 127) ) { 
							dispError( ABNORMAL_PITCH_ENVELOPE_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _EN_ON:			/* ノートエンベロープ */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255)
						 && (cmd->param[0] < 0 || cmd->param[0] > 127) ) { 
							dispError( ABNORMAL_NOTE_ENVELOPE_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _MH_ON:			/* ハードウェアエフェクト */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255) 
						 && (cmd->param[0] < 0 || cmd->param[0] > 15) ) { 
							dispError( ABNORMAL_HARD_EFFECT_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _DETUNE:			/* ディチューン */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255)
						 && (cmd->param[0] <-127 || cmd->param[0] > 126) ) {
							dispError( ABNORMAL_DETUNE_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _TRANSPOSE:			/* トランスポーズ */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255)
						 && (cmd->param[0] <-127 || cmd->param[0] > 126) ) {
							dispError( ABNORMAL_TRANSPOSE_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
						transpose = cmd->param[0];
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _REPEAT_END:		/* リピート終了 */
				  case _REPEAT_END2:	/* リピート終了 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] < 2 ) {
							dispError( ABNORMAL_VALUE_OF_REPEAT_COUNT, lptr[line].filename, line );
							cmd->param[0] = 2;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _VRC7_TONE:			/* VRC7ユーザー音色切り替え */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] < 0 || cmd->param[0] > 63 ) {
							dispError( ABNORMAL_TONE_NUMBER, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _SUN5B_HARD_SPEED:		/* PSGハードウェアエンベロープ速度 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] < 0 || cmd->param[0] > 65535 ) {
							dispError( ABNORMAL_ENVELOPE_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _SUN5B_HARD_ENV:		/* PSGハードウェアエンベロープ選択 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] < 0 || cmd->param[0] > 15 ) {
							dispError( ABNORMAL_ENVELOPE_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						} else {
							volume_flag = 0x8000;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _SUN5B_NOISE_FREQ:	/* PSGノイズ周波数 */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( cmd->param[0] < 0 || cmd->param[0] > 31 ) {
							dispError( ABNORMAL_PITCH_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						} else {
							volume_flag = 0x8000;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _TEMPO2:			/* フレーム基準テンポ */
					ptr = setCommandBuf( 2, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] <= 0) || (cmd->param[1] <= 0) ) {
							dispError( ABNORMAL_TEMPO_VALUE, lptr[line].filename, line );
							cmd->cmd = _NOP;
						} else {
							tbase = (double)cmd->param[0] * (double)cmd->param[1] / _BASE ;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _SWEEP:			/* スウィープ */
					ptr = setCommandBuf( 2, cmd, _SWEEP, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] < 0 || cmd->param[0] > 15)
						 || (cmd->param[1] < 0 || cmd->param[1] > 15) ) {
							dispError( ABNORMAL_SWEEP_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _DATA_WRITE:		/* データ(レジスタ)書き込み */
					ptr = setCommandBuf( 2, cmd, _DATA_WRITE, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _DATA_THRUE:		/* データ直接書き込み */
					ptr = setCommandBuf( 2, cmd, _DATA_THRUE, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
#if 0
				  case _XX_COMMAND:		/* デバッグ用 */
					ptr = setCommandBuf( 2, cmd, _XX_COMMAND, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
#endif
				  case _SELF_DELAY_ON:		/* セルフディレイ */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) != 0 ) {
						if( (cmd->param[0] != 255)
						 && (cmd->param[0] < 0 || cmd->param[0] > SELF_DELAY_MAX) ) {
							dispError( ABNORMAL_SELFDELAY_VALUE, lptr[line].filename, line );
							cmd->cmd = 0;
							cmd->line = 0;
						}
					} else {
						cmd->cmd = _NOP;
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;
				  case _DATA_BREAK:		/* データ変換中止 */
					setCommandBuf( 0, cmd, _TRACK_END, ptr, line, mml[i].enable&(1<<trk) );
					if( (mml[i].enable&(1<<trk)) == 0 ) {
						dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
					}
					break;

				  case _NEW_BANK:
					// 無視する場合でもptrは読み進める
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if (!auto_bankswitch) {
						if( (mml[i].enable&(1<<trk)) != 0 ) {
							if( cmd->param[0] == PARAM_OMITTED ) {
								/* そういう場合があります */
							}
						} else {
							dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
						}
					} else {
						cmd->cmd = _NOP;
					}
					break;

				  case _SHIFT_AMOUNT:			/* ピッチシフト量 (0～8) */
					ptr = setCommandBuf( 1, cmd, mml[i].num, ptr, line, mml[i].enable&(1<<trk) );
					if (pitch_correction) {
						if( (mml[i].enable&(1<<trk)) != 0 ) {
							if( (cmd->param[0] < 0 || cmd->param[0] > 8) ) { 
								dispError( ABNORMAL_SHIFT_AMOUNT, lptr[line].filename, line );
								cmd->cmd = 0;
								cmd->line = 0;
							}
						} else {
							dispError( UNUSE_COMMAND_IN_THIS_TRACK, lptr[line].filename, line );
						}
					} else {
						dispError( CANT_USE_SHIFT_AMOUNT_WITHOUT_PITCH_CORRECTION, lptr[line].filename, line );
					}
					break;
						
				  default:				/* その他(エラー) */
					dispError( COMMAND_NOT_DEFINED, lptr[line].filename, line );
					ptr++;
					break;
				}
				if( cmd->line != 0 ) {
					cmd++;
				}
			}
		} else if( lptr[line].status == _INCLUDE ) {
			cmd = analyzeData( trk, cmd, lptr[line].inc_ptr );
		}
	}
	return cmd;
}




#if 0
typedef struct {
	int flag;
	double diff;
	double base; //シャッフルさせるN分音符のカウント長
} SHFL_Q;


/*--------------------------------------------------------------
	シャッフルクオンタイズ
--------------------------------------------------------------*/
void shuffleQuontizeSub(CMD *ptr, SHFL_Q *shf, double count)
{
	if (shf->flag != 0) {
		double noteoff_time;
		if (double2int(count / shf->base) % 2 == 1 ) {
			//ノートオンの時刻が裏拍
			ptr->len = ptr->len - shf->diff;
		}
		noteoff_time = count + ptr->len;
		if (double2int(noteoff_time / shf->base) % 2 == 1) {
			//ノートオフの時刻が裏拍
			ptr->len = ptr->len + shf->diff;
		}
	}
}





void shuffleQuontize(CMD *ptr)
{
	double count = 0.0; //音長の累積。すなわちイベント発生時刻(カウント単位)
	SHFL_Q shuffle = {0, 0.0, 192.0};
	while (1) {
		if (ptr->cmd == _SHUFFLE_QUONTIZE) {
			shuffle.flag = 1;
			shuffle.base = _BASE / ptr->param[0];
			printf("shfl %e\n", shuffle.base);
			shuffle.diff = shuffle.base * 2 * ptr->param[1]/(ptr->param[2] + ptr->param[1]) - shuffle.base;
			/*
			たとえば16分音符を2:1にわけるなら
			shuffle.base = 192/16 = 12; つまりl16=l%12
			shuffle.diff = 24 * 2/3 - 12 
			             = 16 - 12 = 4
			というわけで8分音符(%24)を%12+4と%12-4、すなわち%16と%8にわける
			*/
			ptr->cmd = _NOP;
			ptr++;
		} else if (ptr->cmd == _SHUFFLE_QUONTIZE_RESET) {
			count = 0.0;
			ptr->cmd = _NOP;
			ptr++;
		} else if (ptr->cmd == _SHUFFLE_QUONTIZE_OFF) {
			shuffle.flag = 0;
			ptr->cmd = _NOP;
			ptr++;
		} else if (ptr->cmd == _CONT_NOTE) {
			//連符の中身には関与しないが、連符をカタマリとして捉える
			while( 1 ) {
				if( ptr->cmd == _TRACK_END ) {
					//連符途中で終了
					//ここではエラーを出さない
					return;
				} else if( ptr->cmd == _CONT_END ) {
					//このコマンドが持っている音長に対してクオンタイズ処理
					shuffleQuontizeSub(ptr, &shuffle, count);
					count += ptr->len;
					ptr++;
					break;
				} else if (ptr->cmd <= MAX_NOTE || ptr->cmd == _REST || ptr->cmd == _KEY
						|| ptr->cmd == _NOTE || ptr->cmd == _WAIT || ptr->cmd == _TIE || temp->cmd == _KEY_OFF) {
					//中身はスルー
					ptr++;
				} else {
					ptr++;
				}
			}
		} else if (ptr->cmd <= MAX_NOTE || ptr->cmd == _REST || ptr->cmd == _KEY
				|| ptr->cmd == _NOTE || ptr->cmd == _WAIT || ptr->cmd == _TIE || temp->cmd == _KEY_OFF) {
			shuffleQuontizeSub(ptr, &shuffle, count);
			count += ptr->len;
			ptr++;
		} else if (ptr->cmd == _TRACK_END) {
			break;
		} else {
			//他のはスルー
			ptr++;
		}
	}
}
#endif



/*--------------------------------------------------------------
	ループ/連符の展開
 Input:
	*ptr
 Output:
	**cmd
--------------------------------------------------------------*/
CMD *translateData( CMD **cmd, CMD *ptr )
{
	CMD		*top,*end,*temp;
	int		cnt ,i, loop;
	double	len, gate;

	loop = 0;
	gate = 0;
	top = ptr;
	end = NULL;

	while( 1 ) {
		switch( ptr->cmd ) {
		  case _REPEAT_ST:
			ptr++;
			nest++;
			ptr = translateData( cmd, ptr );
			if (ptr == NULL) {
				/* [が閉じられていない */
				return NULL;
			}
			nest--;
			break;
		  case _REPEAT_END:
			if( nest <= 0 ) {
				dispError( DATA_ENDED_BY_LOOP_DEPTH_EXCEPT_0, ptr->filename, ptr->line );
				ptr->cmd = _NOP;
				ptr++;
				break;
			}
			if( loop == 0 ) {
				loop = ptr->param[0];
				end = ptr+1;
			}
			if( loop == 1 ) {
				return end;
			}
			ptr = top;
			loop--;
			break;
		  case _REPEAT_ESC:
			if( nest <= 0 ) {
				dispError( DATA_ENDED_BY_LOOP_DEPTH_EXCEPT_0, ptr->filename, ptr->line );
				ptr->cmd = _NOP;
				ptr++;
				break;
			}
			if( loop == 1 ) {
				if( end != NULL ) {
					return end;
				}
			}
			ptr++;
			break;
		  case _CONT_NOTE:
			ptr++;
			temp = ptr;
			/* {} の中に[cdefgab]|n|@n|r|wが何個あるか? */
			cnt = 0;
			len = 0;
			while( 1 ) {
				if( temp->cmd == _TRACK_END ) {
					dispError( DATA_ENDED_BY_CONTINUATION_NOTE, mml_names[mml_idx], (ptr-1)->line );
					setCommandBuf( 0, *cmd, _TRACK_END, NULL, ptr->line, 1 );
					break;
				} else if( temp->cmd == _CONT_END ) {
					if (cnt == 0) {
						dispError( TUPLET_BRACE_EMPTY, mml_names[mml_idx], (ptr-1)->line );
						len = 0;
					} else {
						/* {}の中身は全部この長さになる */
						len = temp->len/(double)cnt;
					}
					break;
				} else if( temp->cmd <= MAX_NOTE || temp->cmd == _REST || temp->cmd == _KEY
					|| temp->cmd == _NOTE || temp->cmd == _WAIT || temp->cmd == _KEY_OFF ) {
					cnt++;
				}
				temp++;
			}
			if( temp->cmd != _TRACK_END ) {
				while( ptr->cmd != _TRACK_END ) {
					if( ptr->cmd == _CONT_END ) {
						ptr++;
						break;
					} else if( ptr->cmd <= MAX_NOTE || ptr->cmd == _REST || ptr->cmd == _KEY
						|| ptr->cmd == _NOTE || ptr->cmd == _WAIT || temp->cmd == _KEY_OFF ) {
						gate += len;
						(*cmd)->filename = ptr->filename;
						(*cmd)->cnt      = ptr->cnt;
						(*cmd)->frm      = ptr->frm;
						(*cmd)->line     = ptr->line;
						(*cmd)->cmd      = ptr->cmd;
						(*cmd)->len      = len;
						for( i = 0; i < 8; i++ ) { 
							(*cmd)->param[i] = ptr->param[i];
						}
						gate -= (*cmd)->len;
					} else if (ptr->cmd == _TIE) {
						/* 連符中のタイは削除 */
						(*cmd)->filename = ptr->filename;
						(*cmd)->cnt      = 0;
						(*cmd)->frm      = 0;
						(*cmd)->line     = ptr->line;
						(*cmd)->cmd      = _NOP;
						(*cmd)->len      = 0;
					} else {
						(*cmd)->filename = ptr->filename;
						(*cmd)->cnt      = ptr->cnt;
						(*cmd)->frm      = ptr->frm;
						(*cmd)->line     = ptr->line;
						(*cmd)->cmd      = ptr->cmd;
						(*cmd)->len      = ptr->len;
						for( i = 0; i < 8; i++ ) { 
							(*cmd)->param[i] = ptr->param[i];
						}
					}
					(*cmd)++;
					ptr++;
				}

			}
			break;
		  case _TRACK_END:
			(*cmd)->filename = ptr->filename;
			(*cmd)->cnt      = ptr->cnt;
			(*cmd)->frm      = ptr->frm;
			(*cmd)->line     = ptr->line;
			(*cmd)->cmd      = ptr->cmd;
			(*cmd)->len      = ptr->len;
			for( i = 0; i < 8; i++ ) { 
				(*cmd)->param[i] = ptr->param[i];
			}
			(*cmd)++;
			ptr++;
			if( nest != 0 ) {
				dispError( DATA_ENDED_BY_LOOP_DEPTH_EXCEPT_0, mml_names[mml_idx], (ptr-1)->line );
			}
			return NULL;
		  default:
			(*cmd)->filename = ptr->filename;
			(*cmd)->cnt      = ptr->cnt;
			(*cmd)->frm      = ptr->frm;
			(*cmd)->line     = ptr->line;
			(*cmd)->cmd      = ptr->cmd;
			(*cmd)->len      = ptr->len;
			for( i = 0; i < 8; i++ ) { 
				(*cmd)->param[i] = ptr->param[i];
			}
			(*cmd)++;
			ptr++;
			break;
		}
	}
}


/*--------------------------------------------------------------
	
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void putAsm( FILE *fp, int data )
{
	static char *fn = "";
	static int ln = 0;
	if( putAsm_pos == 0 ) {
		fn = mml_file_name;
		ln = mml_line_pos;
		fprintf( fp, "\tdb\t$%02x", data&0xff );
	} else if( putAsm_pos == 7 ) {
		fprintf( fp, ",$%02x",  data&0xff );
		fprintf( fp, "\t;Trk %c; %s: %d", str_track[mml_trk], fn, ln);
		fprintf( fp, "\n");
	} else {
		fprintf( fp, ",$%02x",  data&0xff );
	}
	if( ++putAsm_pos > 7 ) {
		putAsm_pos = 0;
	}
	bank_usage[curr_bank]++;
}


/*--------------------------------------------------------------
	
--------------------------------------------------------------*/
void putBankOrigin(FILE *fp, int bank)
{
	static int bank_org_written_flag[128] = {1};
	int org;
	if (bank > 127) {
		//assert(0);
		return;
	}
	if (bank_org_written_flag[bank] == 0) {
		switch (bank) {
		case 0:
			org = 0x8000;
			//assert(0);
			break;
		case 1:
			org = 0xa000;
			break;
		case 2:
			org = 0xc000;
			break;
		case 3:
			org = 0xe000;
			break;
		default:
			org = 0xa000;
			break;
		}
		fprintf(fp, "\t.org\t$%04x\n", org);
		bank_org_written_flag[bank] = 1;
		if (bank > bank_maximum) {
			bank_maximum = bank;
		}
	}

}

/*--------------------------------------------------------------
	!=0: OK,  ==0: out of range
--------------------------------------------------------------*/
int checkBankRange(int bank)
{
	if (allow_bankswitching) {
		if (bank < 0 || bank > 127) {
			return 0;
		}
	} else {
		if (bank < 0 || bank > 3) {
			return 0;
		}
	}
	return 1;
}



/*--------------------------------------------------------------
	
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
int double2int(double d)
{
	return (int)(d + 0.5);
}


/*******************************************************
 *
 *↓発音           ↓キーオフ
 *    _                  _
 *   | ＼               | ＼ 次の音(とかイベント)
 *  |    ＼_________   |    ＼_________
 * |                ＼|                ＼
 * |                  |                  ＼
 * <------------------> delta_time 発音から次のイベントまで
 * <--------------->    gate_time  発音からキーオフまで
 *                  <-> left_time  キーオフから次のイベントまでの残り時間
 *
 *******************************************************/

/*--------------------------------------------------------------
	スラー・タイを考慮したデルタタイムを得る
 Input:
	CMD *cmd; デルタタイムを読み始めるコマンドの位置
	int allow_slur = 1; スラー許可(音符の場合)
	               = 0; スラーなし(休符とか)
 Output:
	int *delta; デルタタイム
 Return:
	CMD *cmd; この関数内でcmdを読み進めたので、新しいcmd位置を返す
--------------------------------------------------------------*/
CMD *getDeltaTime(CMD *cmd, int *delta, int allow_slur) {
	*delta = 0;
	while( 1 ) {
		if( loop_flag == 0 ) {
			*delta += ((cmd+1)->frm - cmd->frm);
		} else {
			*delta += ((cmd+1)->lfrm - cmd->lfrm);
		}
		cmd++;
		/* if( cmd->cmd == _SLAR && allow_slur) {
			cmd++;
		} else */
		if( cmd->cmd != _TIE ) {
			break;
		}
	}
	return cmd;
}

/*--------------------------------------------------------------
	qと音長からゲートタイム計算
 Input:
 
 Output:
	
 Return:
	int gate;
--------------------------------------------------------------*/
int calcGateTime(int delta_time, const GATE_Q *gate_q) {
	int gate;
	gate = (delta_time * gate_q->rate) / gate_denom + gate_q->adjust;
	if (gate > delta_time) {
		gate = delta_time;
	} else if (gate < 0) {
		gate = 0;
	}
	if ( delta_time != 0 && gate <= 0 ) {
		gate = 1;
	}
	return gate;
}

/*--------------------------------------------------------------
	音長のあるコマンドの、音長部分の出力(256フレーム以上のときの処理)
 Input:
	int wait_com_no; 256フレーム以上のときに繋ぐコマンド(wかr)
	int len; フレーム音長
 Output:
	
--------------------------------------------------------------*/
void putLengthAndWait(FILE *fp, int wait_com_no, const int len, const CMD *cmd) {
	int len_nokori = len; /* 出力すべき残り音長(フレーム数) */

	if (len == 0) {
		dispWarning( FRAME_LENGTH_IS_0, cmd->filename, cmd->line );
		return;
	} else if (len < 0) {
		dispError(FRAME_LENGTH_LESSTHAN_0, cmd->filename, cmd->line);
		return;
	}

	if( len_nokori > 0xff ) {
		putAsm( fp, 0xff );
		len_nokori -= 0xff;
	} else {
		putAsm( fp, len_nokori );
		len_nokori = 0;
	}
	while (len_nokori != 0) { /* 出力すべき残りのフレーム数が0になるまでリピート */
		if( len_nokori > 0xff ) {
			/* 残り256フレーム以上のとき */
			putAsm( fp, wait_com_no ); 
			putAsm( fp, 0xff ); /* 255フレーム出力 */
			len_nokori -= 0xff;
		} else {
			/* 255フレーム以下のとき */
			putAsm( fp, wait_com_no );
			putAsm( fp, len_nokori ); /* 残り全部出力 */
			len_nokori = 0;
		}
	}
}


typedef struct {
	GATE_Q	gate_q;
	int	env;			// 現在の通常の(キーオンのときの)エンベロープ番号or音量
	int	rel_env;		// 現在のリリースエンベロープ番号(-1:未使用)
	int	last_written_env;	// 最後に書き込んだエンベロープ番号or音量
	int	tone;			// 
	int	rel_tone;		// 
	int	last_written_tone;	//
	int	key_pressed;		// キーオンオフの状態
	int	last_note[SELF_DELAY_MAX+1];		// 最後に書いたノート(@nは無視で)
	int	last_note_keep[SELF_DELAY_MAX+1];	// \コマンド使用時のlast_note状態
	int	self_delay;		// いくつ前のノートを使用するか？（負ならセルフディレイしない）
} PLAYSTATE;

void defaultPlayState(PLAYSTATE *ps)
{
	int i;
	ps->gate_q.rate = gate_denom;
	ps->gate_q.adjust = 0;
	ps->env = -1;
	ps->rel_env = -1;
	ps->last_written_env = -1;
	ps->tone = -1;
	ps->rel_tone = -1;
	ps->last_written_tone = -1;
	ps->key_pressed = 0;
	for (i = 0; i < arraysizeof(ps->last_note); i++) {
		ps->last_note[i] = -1;
		ps->last_note_keep[i] = -1;
	}
	ps->self_delay = -1;
}

/*--------------------------------------------------------------
	リリースエンベロープ＆音色出力、残り時間をrかwで埋める
 Input:
	*cmd putLengthWaitにエラー表示させるためだけに存在する
--------------------------------------------------------------*/
void putReleaseEffect(FILE *fp, const int left_time, const CMD *cmd, PLAYSTATE *ps)
{
	int note = MCK_REST;		//デフォルトは残り時間は休符でつなぐ
	
	//二重キーオフチェック
	if (ps->key_pressed == 0) {
		putAsm(fp, note);
		putLengthAndWait(fp, MCK_WAIT, left_time, cmd);
		return;
	}
	
	if( (ps->rel_env != -1 )		// リリースエンベロープ動作中
	 && (ps->last_written_env != ps->rel_env) ) {	// 現在のエンべロープと変換中のエンベロープが違う
		putAsm( fp, MCK_SET_VOL );	// リリースエンベロープ出力
		putAsm( fp, ps->rel_env );
	 	ps->last_written_env = ps->rel_env;
		note = MCK_WAIT;		//残り時間はウェイト
	}
	if( (ps->rel_tone != -1 )		// リリース音色動作中
	 && (ps->last_written_tone != ps->rel_tone) ) {	// 現在のエンべロープと変換中の音色が違う
		putAsm( fp, MCK_SET_TONE );	// リリース音色出力
		putAsm( fp, ps->rel_tone );
	 	ps->last_written_tone = ps->rel_tone;
		note = MCK_WAIT;		//残り時間はウェイト
	}
	if (note == MCK_WAIT && ps->self_delay >= 0 && ps->last_note[ps->self_delay] >= 0) {
		/* セルフディレイ */
		note = ps->last_note[ps->self_delay];
	}
	if (left_time != 0) {
		putAsm(fp, note);
		putLengthAndWait(fp, note, left_time, cmd);
	}
}



void doNewBank(FILE *fp, int trk, const CMD *cmd)
{
	int banktemp = curr_bank;
	if (cmd->param[0] == PARAM_OMITTED) {
		/* デフォルト */
		banktemp++;
	} else {
		banktemp = cmd->param[0];
	}
	if (checkBankRange(banktemp) == 0) {
		dispError( BANK_IDX_OUT_OF_RANGE,  cmd->filename, cmd->line );
		return;
	}
	if ((banktemp == 2 || banktemp == 3) && dpcm_bankswitch) {
		dispError( CANT_USE_BANK_2_OR_3_WITH_DPCMBANKSWITCH,  cmd->filename, cmd->line );
		return;
	}
	putAsm( fp, MCK_GOTO );
	fprintf( fp,"\n\tdb\tbank(%s_%02d_bnk%03d)*2\n",songlabel,trk,banktemp);
	bank_usage[curr_bank]++;
	fprintf( fp,"\tdw\t%s_%02d_bnk%03d\n",songlabel,trk,banktemp);
	bank_usage[curr_bank]+=2;
	fprintf( fp,"\n\t.bank\t%d\n",banktemp);
	curr_bank = banktemp;
	putBankOrigin(fp, curr_bank);
	fprintf( fp,"%s_%02d_bnk%03d:\n",songlabel,trk,curr_bank);
	putAsm_pos = 0; // 出力位置クリア
	return;
}



int isCmdNotOutput(CMD *cmd)
{
  switch(cmd->cmd)
  {
	case _NOP:
	case _TEMPO:
	case _TEMPO2:
 	case _OCTAVE:
	case _OCT_UP:
	case _OCT_DW:
	case _LENGTH:
	case _TRANSPOSE:
		return 1;
  }
  return 0;
}

int isNextSlar(CMD *cmd)
{
  while(cmd->cmd != _TRACK_END
	&& isCmdNotOutput(cmd)) cmd++;

  if (cmd->cmd == _SLAR)
	return 1;

  return 0;
}





/*--------------------------------------------------------------
	
 Input:
	
 Output:
	無し
--------------------------------------------------------------*/
void developeData( FILE *fp, const int trk, CMD *const cmdtop, LINE *lptr )
{
	tbase = 0.625;
	length = 48;
	volume_flag = -1;

	{
		/* テンポラリワークを作成 */
		CMD *cmd = cmdtop;
		CMD *temp = malloc( sizeof(CMD)*32*1024 );
		CMD *const tempback = temp;
		int i, j;
		for( i = 0; i < 32*1024; i++ ) {
			temp->cmd = 0;
			temp->cnt = 0;
			temp->frm = 0;
			temp->line = 0;
			for( j = 0; j < 8; j++ ) {
				temp->param[0] = 0;
			}
			temp++;
		}
		temp = tempback;
		/* チャンネルデータの頭からコマンドを解析、バッファにためる */
		temp = analyzeData( trk, temp, lptr );
		setCommandBuf( 0, temp, _TRACK_END, NULL, 0, 1 );
		temp = tempback;
		//shuffleQuontize(temp);
		nest = 0;
		translateData( &cmd, temp );
		cmd = cmdtop;
		free( tempback );
	}

	tbase = 0.625;
	
	{
		CMD *cmd = cmdtop;
		double	count, lcount, count_t;
		int		frame, lframe, frame_p, frame_d;
		double	tbase_p;

		/* カウントからフレームに変換 */
		/* なるべくキリのいい時点を起点にする */
		loop_flag = 0;
		
		count = 0; //トラック開始時点からの経過カウント数
		frame = 0; //トラック開始時点からの経過フレーム数
		lcount = 0; //ループ開始時点からの経過カウント数
		lframe = 0; //ループ開始時点からの経過フレーム数
		/*
			カウントはテンポ関係なく加算していく
			フレームは
			      A t120 l4 c  d   e   f  t240   g   a   b   c   !
			count:          0 48  96 144  192  192 240 288 336 384
			frame:          0 30  60  90  120  120 135 150 165 180
			tbase:      0.625           0.3125
			count_t:        0 48  96 144  192  384 432 480 528 576
			      B t240 l4 cc dd ee ff          g   a   b   c   !
		*/
		count_t = 0; //最初から今まで現在のテンポだったと仮定した時、現在の状態と同じ時間を経過させるためのカウント数
		do {
			cmd->cnt = count;
			cmd->frm = frame;
			cmd->lcnt = lcount;
			cmd->lfrm = lframe;

	//		printf("%s:%d:%4x %f %d %f\n", cmd->filename, cmd->line, cmd->cmd, cmd->cnt, cmd->frm, cmd->len);

			if( cmd->cmd == _REPEAT_ST2 ) {
				double	rcount = 0;
				double	rcount_esc = 0;		// \の手前まで
				double	rcount_t = 0;
				double	rcount_esc_t = 0;
				int	rframe = 0;
				int	rframe_esc = 0;
				int	rframe_err;
				int	repeat_esc_flag = 0;
				CMD	*repeat_esc2_cmd_ptr = NULL;
				
				cmd++;
				while( 1 ) {
					cmd->cnt = count;
					cmd->frm = frame;
					cmd->lcnt = lcount;
					cmd->lfrm = lframe;
					if( cmd->cmd == _REPEAT_END2 ) {
						count_t += rcount_t*(cmd->param[0]-2)+rcount_esc_t;
						count += rcount*(cmd->param[0]-2)+rcount_esc;
						frame += rframe*(cmd->param[0]-2)+rframe_esc;
						if( loop_flag != 0 ) {
							lcount += rcount*(cmd->param[0]-2)+rcount_esc;
							lframe += rframe*(cmd->param[0]-2)+rframe_esc;
						}
						/* フレーム補正 */
						rframe_err = double2int(count_t * tbase) - frame;
						//printf( "frame-error: %d frame\n", rframe_err );
						if (rframe_err > 0) {
							//printf( "frame-correct: %d frame\n", rframe_err );
							if (rframe_err >= 3)
							{
								dispWarning(REPEAT2_FRAME_ERROR_OVER_3, cmd->filename, cmd->line);
							}
							/* 2004.09.02 やっぱりやめる
							cmd->param[1] = rframe_err;
							frame += rframe_err;
							if( loop_flag != 0 ) {
								lframe += rframe_err;
							}
							*/
						} else {
							cmd->param[1] = 0;
						}
						if (repeat_esc_flag) {
							// 繰り返し回数を対応する\\コマンドにも
							repeat_esc2_cmd_ptr->param[0] = cmd->param[0];
						}
						break;
						
					} else if( cmd->cmd == _REPEAT_ESC2 ) {
						repeat_esc_flag = 1;
						repeat_esc2_cmd_ptr = cmd;
					} else if( cmd->cmd <= MAX_NOTE || cmd->cmd == _REST || cmd->cmd == _TIE
							|| cmd->cmd == _KEY || cmd->cmd == _NOTE || cmd->cmd == _WAIT || cmd->cmd == _KEY_OFF ) {
						count_t += cmd->len;
						rcount_t += cmd->len;
						frame_p = rframe;
						rframe = double2int(rcount_t * tbase);
						frame_d = rframe - frame_p;
						count += cmd->len;
						frame += frame_d;
/* 対ループずれ対策 */
						if( loop_flag != 0 ) {
							lcount += cmd->len;
							lframe += frame_d;
						}
						rcount += cmd->len;
						if( repeat_esc_flag == 0 ) {
							rcount_esc_t += cmd->len;
							rcount_esc += cmd->len;
							rframe_esc += frame_d;
						} 
					} else if( cmd->cmd == _TEMPO ) {
						tbase_p = tbase;
						tbase = (double)_BASETEMPO / (double)cmd->param[0];
						count_t = count_t * tbase / tbase_p;
						rcount_t = rcount_t * tbase / tbase_p;
						rcount_esc_t = rcount_esc_t * tbase / tbase_p;
					} else if( cmd->cmd == _TEMPO2 ) {
						tbase_p = tbase;
						tbase = (double)cmd->param[0] * (double)cmd->param[1] / _BASE;
						count_t = count_t * tbase / tbase_p;
						rcount_t = rcount_t * tbase / tbase_p;
						rcount_esc_t = rcount_esc_t * tbase / tbase_p;
					} else if( cmd->cmd == _SONG_LOOP ) {
						loop_flag = 1;
					}
					cmd++;
				}
			} else if( cmd->cmd <= MAX_NOTE || cmd->cmd == _REST || cmd->cmd == _TIE
					|| cmd->cmd == _KEY || cmd->cmd == _NOTE || cmd->cmd == _WAIT || cmd->cmd == _KEY_OFF ) {
				count_t += cmd->len;
				frame_p = frame;
				frame = double2int(count_t * tbase);
				frame_d = frame - frame_p;
				count += cmd->len;
	/* 対ループずれ対策 */
				if( loop_flag != 0 ) {
					lcount += cmd->len;
					lframe += frame_d;
				}
			} else if( cmd->cmd == _TEMPO ) {
				tbase_p = tbase;
				tbase = (double)_BASETEMPO / (double)cmd->param[0];
				count_t = count_t * tbase_p / tbase;
			} else if( cmd->cmd == _TEMPO2 ) {
				tbase_p = tbase;
				tbase = (double)cmd->param[0] * (double)cmd->param[1] / _BASE;
				count_t = count_t * tbase_p / tbase;
			} else if( cmd->cmd == _SONG_LOOP ) {
				loop_flag = 1;
			}
		} while( cmd++->cmd != _TRACK_END );
	}
	
	{
		CMD *cmd = cmdtop;
		PLAYSTATE ps;
		int repeat_depth = 0;
		int repeat_index = 0;
		int repeat_esc_flag = 0;
		int i;
		char loop_point_label[256];
		int slar_flag = 0;
		int slar_cmdcnt = 0;


		defaultPlayState(&ps);
		
		cmd = cmdtop;
		putAsm_pos = 0;
		loop_flag = 0;
		
		sprintf(loop_point_label, "%s_%02d_lp", songlabel, trk );
		
		mml_trk = trk;
		fprintf( fp, "\n%s_%02d:\t;Trk %c\n", songlabel, trk, str_track[trk] );
		
		mml_file_name = cmd->filename;
		mml_line_pos = cmd->line;
		
		// 三角波/ノイズトラック対策
		if( (trk == BTRACK(2)) || (trk == BTRACK(3)) ) {
			putAsm( fp, MCK_SET_TONE );
			putAsm( fp, 0x8f );
		}		
		
		
		
		do {
			const CMD cmdtemp = *cmd; //各switch内でcmdポインタが進む可能性があるので一旦保存
			mml_file_name = cmd->filename;
			mml_line_pos = cmd->line;
			
			// 自動バンク切り替え
			if (auto_bankswitch) {
				const int bank_limit = 8192 - 20; // 適当に余裕を持たせる
				if (bank_usage[curr_bank] > bank_limit) {
					CMD nbcmd;
					nbcmd.param[0] = curr_bank;
					while (bank_usage[ nbcmd.param[0] ] > bank_limit) {
						nbcmd.param[0]++;
					}
					nbcmd.filename = cmd->filename;
					nbcmd.line = cmd->line;
					doNewBank( fp, trk, &nbcmd );
				}
			}
			
			switch (cmdtemp.cmd) {
			  case _NOP:
			  case _TEMPO:
			  case _TEMPO2:
			  case _OCTAVE:
			  case _OCT_UP:
			  case _OCT_DW:
			  case _LENGTH:
			  case _TRANSPOSE:
				cmd++;
				break;
			  case _SLAR:
				if (!slar_flag)
				{
					slar_flag = 1;
					slar_cmdcnt=0;
				}
				putAsm( fp, MCK_SLAR );
				cmd++;
			  break;
			  case _PITCH_SHIFT:
				if (!slar_flag)
				{
					slar_flag = 1;
					slar_cmdcnt=0;
				}
				putAsm( fp, MCK_PITCH_SHIFT );
				cmd++;
			  break;
			  case _SMOOTH_ON:
				putAsm( fp, MCK_SMOOTH );
				putAsm( fp, 0x01 );
				cmd++;
			  break;
			  case _SMOOTH_OFF:
				putAsm( fp, MCK_SMOOTH );
				putAsm( fp, 0x00 );
				cmd++;
			  break;
			  case _ENVELOPE:
				putAsm( fp, MCK_SET_VOL );
				ps.env = cmd->param[0]&0x7f; 
				ps.last_written_env = ps.env;
				putAsm( fp, ps.env ); 
				ps.last_written_env = ps.env;
				cmd++; 
				break; 
			  case _REL_ENV: 
				if( cmd->param[0] == 255 ) { 
					ps.rel_env = -1;
				} else { 
					ps.rel_env = cmd->param[0]&0x7f; 
				}
				cmd++;
				break;
			  case _VOLUME:
				putAsm( fp, MCK_SET_VOL );
				if( trk == BFMTRACK || trk == BVRC6SAWTRACK) {
					ps.env = (cmd->param[0]&0x3f)|0x80;
				} else {
					ps.env = (cmd->param[0]&0x0f)|0x80;
				}
				putAsm( fp, ps.env );
				ps.last_written_env = ps.env;
				cmd++;
				break;
			  case _HARD_ENVELOPE:
				putAsm( fp, MCK_SET_FDS_HWENV );

				if ((trk == BTRACK(0)) || (trk == BTRACK(1) ))
					ps.env = ( ((cmd->param[0]&1)<<4)|((cmd->param[1]&1)<<5) );
					else
					ps.env = ((cmd->param[0]&1)<<6)|(cmd->param[1]&0x3f);
				putAsm( fp, (ps.env & 0xff) ); 
				ps.last_written_env = ps.env;
				cmd++;
				break;
			  case _TONE:
				ps.tone = cmd->param[0]|0x80;
				putAsm( fp, MCK_SET_TONE );
				putAsm( fp, ps.tone );
				ps.last_written_tone = ps.tone;
				cmd++;
				break;
			  case _ORG_TONE:
				ps.tone = cmd->param[0]&0x7f;
				putAsm( fp, MCK_SET_TONE );
				putAsm( fp, ps.tone );
				ps.last_written_tone = ps.tone;
				cmd++;
				break;
			  case _REL_ORG_TONE:
				if( cmd->param[0] == 255 ) { 
					ps.rel_tone = -1;
				} else {
					ps.rel_tone = cmd->param[0]&0x7f;
				}
				cmd++;
				break;
			  case _SONG_LOOP:
				//loop_count.cnt = cmd->cnt; //LEN
				//loop_count.frm = cmd->frm;
				fprintf( fp, "\n%s:\n", loop_point_label);
				loop_flag = 1;
				putAsm_pos = 0;
				cmd++;
				break;
			  case _QUONTIZE:
				ps.gate_q.rate = cmd->param[0];
				ps.gate_q.adjust = cmd->param[1];
				cmd++;
				break;
			  case _QUONTIZE2:
				ps.gate_q.rate = gate_denom;
				ps.gate_q.adjust = - cmd->param[0];
				cmd++;
				break;
			  case _REST:
				{
					int delta_time = 0;
					cmd = getDeltaTime(cmd, &delta_time, 0);
					if( delta_time == 0 ) {
						dispWarning( FRAME_LENGTH_IS_0, cmdtemp.filename, cmdtemp.line );
						break;
					}
					putAsm(fp, MCK_REST);
					putLengthAndWait(fp, MCK_REST, delta_time, &cmdtemp);
					ps.key_pressed = 0;
				}
				break;
			  case _WAIT:
				{
					int delta_time = 0;
					cmd = getDeltaTime(cmd, &delta_time, 0);
					if( delta_time == 0 ) {
						dispWarning( FRAME_LENGTH_IS_0, cmdtemp.filename, cmdtemp.line );
						break;
					}
					putAsm(fp, MCK_WAIT);
					putLengthAndWait(fp, MCK_WAIT, delta_time, &cmdtemp);
				}
				break;
			  case _KEY_OFF: /* 長さつきキーオフ */ 
				{
					int delta_time = 0;
					cmd = getDeltaTime(cmd, &delta_time, 0);
					if( delta_time == 0 ) {
						/* 音長0を許す */
					}
					putReleaseEffect(fp, delta_time, &cmdtemp, &ps);
					ps.key_pressed = 0;
				}
				break;
			  case _LFO_ON:
				putAsm( fp, MCK_SET_LFO );
				if( (cmd->param[0]&0xff) == 0xff ) {
					putAsm( fp, 0xff );
				} else {
					putAsm( fp, cmd->param[0]&0x7f );
				}
				cmd++;
				break;
			  case _LFO_OFF:
				putAsm( fp, MCK_SET_LFO );
				putAsm( fp, 0xff );
				cmd++;
				break;
			  case _DETUNE:
				putAsm( fp, MCK_SET_DETUNE );
				if( cmd->param[0] >= 0 ) {
					putAsm( fp, ( cmd->param[0]&0x7f)|0x80 );
				} else {
					putAsm( fp, (-cmd->param[0])&0x7f );
				}
				cmd++;
				break;
			  case _SWEEP:
				putAsm( fp, MCK_SET_HWSWEEP );
				putAsm( fp, ((cmd->param[0]&0xf)<<4)+(cmd->param[1]&0xf) );
				cmd++;
				break;
			  case _EP_ON:
				putAsm( fp, MCK_SET_PITCHENV );
				putAsm( fp, cmd->param[0]&0xff );
				cmd++;
				break;
			  case _EP_OFF:
				putAsm( fp, MCK_SET_PITCHENV );
				putAsm( fp, 0xff );
				cmd++;
				break;
			  case _EN_ON:
				putAsm( fp, MCK_SET_NOTEENV );
				putAsm( fp, cmd->param[0]&0xff );
				cmd++;
				break;
			  case _EN_OFF:
				putAsm( fp, MCK_SET_NOTEENV );
				putAsm( fp, 0xff );
				cmd++;
				break;
			  case _MH_ON:
				putAsm( fp, MCK_SET_FDS_HWEFFECT );
				putAsm( fp, cmd->param[0]&0xff );
				cmd++;
				break;
			  case _MH_OFF:
				putAsm( fp, MCK_SET_FDS_HWEFFECT );
				putAsm( fp, 0xff );
				cmd++;
				break;
			  case _VRC7_TONE:
				putAsm( fp, MCK_SET_TONE );
				putAsm( fp, cmd->param[0]|0x40 );
				cmd++;
				break;
			  case _SUN5B_HARD_SPEED:
				putAsm( fp, MCK_SET_SUN5B_HARD_SPEED );
				putAsm( fp, cmd->param[0]&0xff );
				putAsm( fp, (cmd->param[0]>>8)&0xff );
				cmd++;
				break;
			  case _SUN5B_HARD_ENV:
				putAsm( fp, MCK_SUN5B_HARD_ENV );
				ps.env = (cmd->param[0]&0x0f)|0x10|0x80;
				putAsm( fp,  ps.env );
				cmd++;
				break;
			  case _SUN5B_NOISE_FREQ:
				putAsm( fp, MCK_SET_SUN5B_NOISE_FREQ );
				putAsm( fp, cmd->param[0]&0x1f );
				cmd++;
				break;
			  case _NEW_BANK:
				doNewBank( fp, trk, cmd );
				cmd++;
				break;
			  case _DATA_WRITE:
				putAsm( fp, MCK_DATA_WRITE );
				putAsm( fp,  cmd->param[0]    &0xff );
				putAsm( fp, (cmd->param[0]>>8)&0xff );
				putAsm( fp,  cmd->param[1]    &0xff );
				cmd++;
				break;
			  case _DATA_THRUE:
				putAsm( fp,  cmd->param[0]    &0xff );
				putAsm( fp,  cmd->param[1]    &0xff );
				cmd++;
				break;
			  case _REPEAT_ST2:
				fprintf( fp, "\n%s_%02d_lp_%04d:\n", songlabel, trk, repeat_index );
				repeat_depth++;
				putAsm_pos = 0;
				cmd++;
				break;
			  case _REPEAT_END2:
				if( --repeat_depth < 0 ) {
					dispError( DATA_ENDED_BY_LOOP_DEPTH_EXCEPT_0, cmd->filename, cmd->line );
				} else {
					if (repeat_esc_flag != 0) {
						// 常に戻る
						putAsm( fp, MCK_GOTO );
					} else {
						putAsm( fp, MCK_REPEAT_END );
						putAsm( fp, cmd->param[0]&0x7f );
					}
					fprintf( fp,"\n\tdb\tbank(%s_%02d_lp_%04d)*2\n", songlabel, trk, repeat_index );
					bank_usage[curr_bank]++;
					fprintf( fp,"\tdw\t%s_%02d_lp_%04d\n", songlabel, trk, repeat_index );
					bank_usage[curr_bank]+=2;
					
					fprintf( fp, "%s_%02d_lp_exit_%04d:\n", songlabel, trk, repeat_index );
					repeat_index++;
					putAsm_pos = 0;
					/* 2004.09.02 やっぱりやめる
					if ( cmd->param[1] > 0 ) {
						putAsm( fp, MCK_WAIT );
						putAsm( fp, cmd->param[1]&0xFF);
					} */
					if (repeat_esc_flag != 0) {
						for (i = 0; i < arraysizeof(ps.last_note); i++) {
							ps.last_note[i] = ps.last_note_keep[i];
						}
						repeat_esc_flag = 0;
					}
				}
				cmd++;
				break;
			  case _REPEAT_ESC2:
				if( (repeat_depth-1) < 0 ) {
					dispError( DATA_ENDED_BY_LOOP_DEPTH_EXCEPT_0, cmd->filename, cmd->line );
				} else {
					putAsm( fp, MCK_REPEAT_ESC );
					putAsm( fp, cmd->param[0]&0x7f );
					fprintf( fp,"\n\tdb\tbank(%s_%02d_lp_exit_%04d)*2\n", songlabel, trk, repeat_index );
					bank_usage[curr_bank]++;
					fprintf( fp, "\tdw\t%s_%02d_lp_exit_%04d\n", songlabel, trk, repeat_index );
					bank_usage[curr_bank]+=2;
					putAsm_pos = 0;
					repeat_esc_flag = 1;
					for (i = 0; i < arraysizeof(ps.last_note); i++) {
						ps.last_note_keep[i] = ps.last_note[i];
					}
				}
				cmd++;
				break;
			  case _SELF_DELAY_ON:
				if( cmd->param[0] == 255 ) { 
					ps.self_delay = -1;
				} else {
					ps.self_delay = cmd->param[0];
				}
				cmd++;
				break;
			  case _SELF_DELAY_OFF:
				ps.self_delay = -1;
				cmd++;
				break;
			  case _SELF_DELAY_QUEUE_RESET:
				for (i = 0; i < arraysizeof(ps.last_note); i++) {
					ps.last_note[i] = -1;
					ps.last_note_keep[i] = -1;
				}
				cmd++;
				break;
			  case _SHIFT_AMOUNT:
				putAsm( fp, MCK_SET_SHIFT_AMOUNT );
				putAsm( fp, cmd->param[0] & 0xff );
				cmd++;
				break;
			  case _TRACK_END:
				break;
			  case _KEY:
			  default:
				{
					int note;
					int delta_time; /* 発音から次のイベントまでのフレーム数 */
					int gate_time; /* 発音からキーオフまでのフレーム数 */
					int left_time; /* キーオフから次のイベントまでの残りフレーム数 */
					
					if (cmdtemp.cmd == _KEY) {
						note = cmd->param[0]&0xffff;
					} else {
						note = cmdtemp.cmd;
						if (note < MIN_NOTE || MAX_NOTE < note) {
							dispError( COMMAND_NOT_DEFINED, cmd->filename, cmd->line );
							cmd++;
							break;
						}
					}


					
					
					delta_time = 0;
					cmd = getDeltaTime(cmd, &delta_time, 1);

					if (isNextSlar(cmd)) 
					{
						GATE_Q temp_gate;

						temp_gate.rate = 8;
						temp_gate.adjust = ps.gate_q.adjust;
						gate_time = calcGateTime(delta_time, &temp_gate);
					}
					else
						gate_time = calcGateTime(delta_time, &(ps.gate_q));

					
//					gate_time = calcGateTime(delta_time, &(ps.gate_q));


					left_time = delta_time - gate_time;
					
					if( delta_time == 0 ) {
						dispWarning( FRAME_LENGTH_IS_0, cmdtemp.filename, cmdtemp.line );
						break;
					}
					

					if (slar_flag && slar_cmdcnt > 1)
					{
						dispError( ABNORMAL_NOTE_AFTER_COMMAND, cmd->filename, cmd->line );
						cmd++;
						break;
					}
					slar_flag = 0;
					
					if( ps.last_written_env != ps.env ) {		// 最後に書き込んだエンべロープor音量と、現在の通常のエンベロープor音量が違う
						if ( (trk == BFMTRACK) && (ps.env > 0xFF) ) {
							putAsm( fp, MCK_SET_FDS_HWENV );	// ハードエンベ出力
							putAsm( fp, (ps.env & 0xff) );
						} else {
							putAsm( fp, MCK_SET_VOL );	// エンベロープ出力
							putAsm( fp, ps.env );
						}
						ps.last_written_env = ps.env;
					}
					
					if( ps.last_written_tone != ps.tone ) {	// 最後に書き込んだ音色と、現在の通常の音色が違う
						putAsm( fp, MCK_SET_TONE );	// 音色出力
						putAsm( fp, ps.tone );
						ps.last_written_tone = ps.tone;
					}

					if( (ps.tone == -1) &&
					    ((trk == BTRACK(0))  || (trk == BTRACK(1)) ||
					     (trk == BMMC5TRACK) || (trk == BMMC5TRACK+1)) ) {
						// 内蔵矩形波＆MMC5は音色未指定時@0に
						putAsm( fp, MCK_SET_TONE );
						ps.tone = 0x80;
						putAsm( fp, ps.tone );
						ps.last_written_tone = ps.tone;
					}
					
					if (cmdtemp.cmd == _KEY) {
						putAsm( fp, MCK_DIRECT_FREQ );
						putAsm( fp,  note    &0xff );
						if ( ((trk >= BVRC6TRACK) && (trk <= BVRC6SAWTRACK)) ||
						     ((trk >= BFME7TRACK) && (trk <= BFME7TRACK+2 )) ) {
							// VRC6＆SUN5Bは12bit
							putAsm( fp, (note>>8)&0x0f );
						} else {
							// 2A03＆MMC5は11bit
							putAsm( fp, (note>>8)&0x07 );
						}
					} else {
						if( note < 0 ) {				/* 最低音の対策 */
							note += 16;
						}
						putAsm( fp, note );
						
						
						for (i = arraysizeof(ps.last_note) - 1 ; i > 0; i--) {
							ps.last_note[i] = ps.last_note[i-1];
						}
						ps.last_note[0] = note;
					}
					
					
					putLengthAndWait(fp, MCK_WAIT, gate_time, &cmdtemp);
					ps.key_pressed = 1;
					
					// クオンタイズ処理
					if ( left_time != 0 ) {
						putReleaseEffect(fp, left_time, &cmdtemp, &ps);
						ps.key_pressed = 0;
					}
				}
				break;
			} // switch (cmdtemp.cmd)
			
			if (slar_flag)
				slar_cmdcnt++;
			
		} while( cmd->cmd != _TRACK_END );
		
		
		track_count[mml_idx][trk][0].cnt = cmd->cnt;
		track_count[mml_idx][trk][0].frm = cmd->frm;

		if( loop_flag == 0 ) {
			track_count[mml_idx][trk][1].cnt = 0;
			track_count[mml_idx][trk][1].frm = 0;

			fprintf( fp, "\n%s:\n", loop_point_label );
			putAsm_pos = 0;
			putAsm( fp, MCK_REST );
			putAsm( fp, 0xff );
		} else {
			track_count[mml_idx][trk][1].cnt = cmd->lcnt;
			track_count[mml_idx][trk][1].frm = cmd->lfrm;
		}
		// putAsm( fp, MCK_DATA_END );
		putAsm( fp, MCK_GOTO );
		fprintf( fp,"\n\tdb\tbank(%s)*2\n", loop_point_label );
		bank_usage[curr_bank]++;
		fprintf( fp,"\tdw\t%s\n", loop_point_label );
		bank_usage[curr_bank]+=2;
		fputc( '\n', fp );
	}
}


/*--------------------------------------------------------------
	
--------------------------------------------------------------*/
void setSongLabel(void)
{
	sprintf(songlabel, "song_%03d", mml_idx);
}


/*--------------------------------------------------------------
	リザルト表示ルーチン
	i:trk number
	trk: track symbol
--------------------------------------------------------------*/

void display_counts_sub(int i, char trk)
{
	printf( "   %c   |", trk);
	if( track_count[mml_idx][i][0].cnt != 0 ) {
		printf (" %6d   %5d|", double2int(track_count[mml_idx][i][0].cnt), track_count[mml_idx][i][0].frm );
	} else {
		printf( "               |" );
	}
	if( track_count[mml_idx][i][1].cnt != 0 ) {
		printf (" %6d   %5d|\n", double2int(track_count[mml_idx][i][1].cnt),track_count[mml_idx][i][1].frm );
	} else {
		printf( "               |\n" );
	}
}



/*--------------------------------------------------------------
	データ作成ルーチン
 Input:
	無し
 Return:
	==0:正常 !=0:異常
--------------------------------------------------------------*/
int data_make( void )
{
	FILE	*fp;
	int		i, j, track_ptr;
	int		tone_max, envelope_max, pitch_env_max, pitch_mod_max;
	int		arpeggio_max, fm_tone_max, dpcm_max, n106_tone_max,vrc7_tone_max;
	int		hard_effect_max, effect_wave_max;
	LINE	*line_ptr[MML_MAX];
	CMD		*cmd_buf;
	int	trk_flag[_TRACK_MAX];

	for(i=0; i < _TRACK_MAX; i++) {
		bank_sel[i] = -1; // 初期状態は切り替え無し
	}
	for( i = 0; i < _DPCM_MAX; i++ ) {
		dpcm_tbl[i].flag = 0;
		dpcm_tbl[i].index = -1;
	}

	/* 全てのMMLからエフェクトを読み込み */
	for (mml_idx = 0; mml_idx < mml_num; mml_idx++) {
		line_ptr[mml_idx] = readMmlFile(mml_names[mml_idx]);
		if( line_ptr[mml_idx] == NULL ) return -1;
		getLineStatus(line_ptr[mml_idx], 0 );
#if DEBUG
		for( i = 1; i < line_max; i++ ) {
			printf( "%4d : %04x\n", i, line_ptr[mml_idx][i].status );
		}
#endif


		getTone(     line_ptr[mml_idx] );
		getEnvelope( line_ptr[mml_idx] );
		getPitchEnv( line_ptr[mml_idx] );
		getPitchMod( line_ptr[mml_idx] );
		getArpeggio( line_ptr[mml_idx] );
		getDPCM(     line_ptr[mml_idx] );
		getFMTone(   line_ptr[mml_idx] );
		getVRC7Tone( line_ptr[mml_idx] );
		getN106Tone( line_ptr[mml_idx] );
		getHardEffect(line_ptr[mml_idx]);
		getEffectWave(line_ptr[mml_idx]);
	}

	tone_max      = checkLoop(       tone_tbl,      _TONE_MAX );
	envelope_max  = checkLoop(   envelope_tbl,  _ENVELOPE_MAX );
	pitch_env_max = checkLoop(  pitch_env_tbl, _PITCH_ENV_MAX );
	pitch_mod_max = getMaxLFO(  pitch_mod_tbl, _PITCH_MOD_MAX );
	arpeggio_max  = checkLoop(   arpeggio_tbl,  _ARPEGGIO_MAX );
	dpcm_max      = getMaxDPCM( dpcm_tbl );
	fm_tone_max   = getMaxTone(   fm_tone_tbl,   _FM_TONE_MAX );
	n106_tone_max = getMaxTone( n106_tone_tbl, _N106_TONE_MAX );
	vrc7_tone_max = getMaxTone( vrc7_tone_tbl, _VRC7_TONE_MAX );
	hard_effect_max = getMaxHardEffect( hard_effect_tbl, _HARD_EFFECT_MAX );
	effect_wave_max = getMaxEffectWave( effect_wave_tbl, _EFFECT_WAVE_MAX );

	sortDPCM( dpcm_tbl );					// 音色のダブりを削除
	dpcm_size = checkDPCMSize( dpcm_tbl );
	//printf("dpcmsize $%x\n",dpcm_size);
	if ( !allow_bankswitching && (dpcm_size > _DPCM_TOTAL_SIZE)) {	// サイズをチェック
		dispError( DPCM_FILE_TOTAL_SIZE_OVER, NULL, 0 );
		dpcm_size = 0;
	} else {
		dpcm_data = malloc( dpcm_size );
		readDPCM( dpcm_tbl );
	}
	/* ピッチエンベロープのパラメータ修正 */
	for( i = 0; i < pitch_env_max; i++ ) {
		if( pitch_env_tbl[i][0] != 0 ) {
			for( j = 1; j <= pitch_env_tbl[i][0]; j++ ) {
				if( 0 < pitch_env_tbl[i][j] && pitch_env_tbl[i][j] < 127 ) {
					pitch_env_tbl[i][j] = pitch_env_tbl[i][j]|0x80;
				} else if( 0 >= pitch_env_tbl[i][j] && pitch_env_tbl[i][j] >= -127 ) {
					pitch_env_tbl[i][j] = (0-pitch_env_tbl[i][j]);
				}
			}
		}
	}

	{
		fp = fopen( ef_name, "wt" );
		if( fp == NULL ) {
			if( message_flag == 0 ) {
				printf( "%s : ファイルが作成できませんでした。中止します。\n", ef_name );
			} else {
				printf( "%s : Don't create file. Stops.\n", ef_name );
			}
			return -1;
		}
		


		/* 音色書き込み */
		writeTone( fp, tone_tbl, "dutyenve", tone_max );
		/* エンベロープ書き込み */
		writeTone( fp, envelope_tbl, "softenve", envelope_max );
		/* ピッチエンベロープ書き込み */
		writeTone( fp, pitch_env_tbl, "pitchenve", pitch_env_max );
		/* ノートエンベロープ書き込み */
		writeTone( fp, arpeggio_tbl, "arpeggio", arpeggio_max );
		/* LFO書き込み */
		fprintf( fp,"lfo_data:\n" );
		if( pitch_mod_max != 0 ) {
			for( i = 0; i < pitch_mod_max; i++ ) {
				if( pitch_mod_tbl[i][0] != 0 ) {
					fprintf( fp, "\tdb\t$%02x,$%02x,$%02x,$%02x\n",
						pitch_mod_tbl[i][1], pitch_mod_tbl[i][2],
						pitch_mod_tbl[i][3], pitch_mod_tbl[i][4] );
				} else {
					fprintf( fp, "\tdb\t$00,$00,$00,$00\n" );
				}
			}
			fprintf( fp, "\n" );
		}
		/* FM音色書き込み */
		writeToneFM( fp, fm_tone_tbl, "fds", fm_tone_max );
		writeHardEffect( fp, hard_effect_tbl, "fds", hard_effect_max );
		writeEffectWave( fp, effect_wave_tbl, "fds", effect_wave_max );
		/* namco106音色書き込み */
		writeToneN106( fp, n106_tone_tbl, "n106", n106_tone_max );
		/* VRC7音色書き込み */
		writeToneVRC7( fp, vrc7_tone_tbl, "vrc7", vrc7_tone_max );
		/* DPCM書き込み */
		writeDPCM( fp, dpcm_tbl, "dpcm_data", dpcm_max );
		writeDPCMSample( fp );
		
		// MMLファイル書き込み
		if( include_flag != 0 ) {
			fprintf( fp, "\t.include\t\"%s\"\n", out_name );
		}
		
		fclose( fp );
	}

	/* MML->ASMデータ変換 */
	fp = fopen( out_name, "wt" );
	if( fp == NULL ) {
		if( message_flag == 0 ) {
			printf( "%s : ファイルが作成できませんでした。中止します。\n", out_name );
		} else {
			printf( "%s : Don't create file. Stops.\n", out_name );
		}
		return -1;
	}

	/* 出力ファイルにタイトル/作曲者/打ち込み者の情報をコメントとして書き込み */
	writeSongInfo(fp);

	//printf(" test info:vrc7:%d vrc6:%d n106:%d\n",vrc7_track_num,vrc6_track_num, n106_track_num);

	for(i=0; i < _TRACK_MAX; i++) trk_flag[i]=0;

	for(i=0; i <= BNOISETRACK; i++) trk_flag[i]=1;
	
	trk_flag[BDPCMTRACK]=1;
		
	if (fds_track_num)
		trk_flag[BFMTRACK]=1;
		
	if (vrc7_track_num)
		for(i=BVRC7TRACK; i < BVRC7TRACK+vrc7_track_num; i++) trk_flag[i]=1;
	if (vrc6_track_num)
		for(i=BVRC6TRACK; i < BVRC6TRACK+vrc6_track_num; i++) trk_flag[i]=1;
	if (n106_track_num)
		for(i=BN106TRACK; i < BN106TRACK+n106_track_num; i++) trk_flag[i]=1;
	if (fme7_track_num)
		for(i=BFME7TRACK; i < BFME7TRACK+fme7_track_num; i++) trk_flag[i]=1;
	if (mmc5_track_num)
		for(i=BMMC5TRACK; i < BMMC5TRACK+mmc5_track_num; i++) trk_flag[i]=1;


	
	fprintf( fp, "\t.bank\t0\n");
	fprintf( fp, "\t.if TOTAL_SONGS > 1\n");
	fprintf( fp, "song_addr_table:\n" );
	for (mml_idx = 0; mml_idx < mml_num; mml_idx++) {
		setSongLabel();
		fprintf( fp, "\tdw\t%s_track_table\n", songlabel);
	}
	
	fprintf( fp, "\t.if (ALLOW_BANK_SWITCH)\n" );
	fprintf( fp, "song_bank_table:\n" );
	for (mml_idx = 0; mml_idx < mml_num; mml_idx++) {
		setSongLabel();
		fprintf( fp, "\tdw\t%s_bank_table\n", songlabel);
	}
	fprintf( fp, "\t.endif ; ALLOW_BANK_SWITCH\n" );
	fprintf( fp, "\t.endif ; TOTAL_SONGS > 1\n" );
	
	for (mml_idx = 0; mml_idx < mml_num; mml_idx++) {
		setSongLabel();
		fprintf( fp, "%s_track_table:\n", songlabel );
		for( i = 0; i < _TRACK_MAX; i++ ){
			if (trk_flag[i]) fprintf( fp, "\tdw\t%s_%02d\n", songlabel, i );
		}
		
		fprintf( fp, "\t.if (ALLOW_BANK_SWITCH)\n" );
		fprintf( fp, "%s_bank_table:\n", songlabel );
		for( i = 0; i < _TRACK_MAX; i++ ){
			if (trk_flag[i]) fprintf( fp, "\tdb\tbank(%s_%02d)*2\n", songlabel, i );
		}
		fprintf( fp, "\t.endif\n" );
	}

	


	curr_bank = 0x00;

	/* 全てのMMLについて */
	for (mml_idx = 0; mml_idx < mml_num; mml_idx++) {
		setSongLabel();
		/* トラック単位でデータ変換 */
		for( i = 0; i < _TRACK_MAX; i++ ) {
			if ( bank_sel[i] != -1 && !auto_bankswitch) {
				if (trk_flag[i] == 0) {
					if( message_flag == 0 ) {
						printf( "Warning: 未使用トラック(%c)に対しての#SETBANKを無視します\n", str_track[i]);
					} else {
						printf( "Warning: Ignored #SETBANK on unused track(%c)\n", str_track[i]);
					}
				} else if ((bank_sel[i] == 2 || bank_sel[i] == 3) && dpcm_bankswitch) {
					dispError( CANT_USE_BANK_2_OR_3_WITH_DPCMBANKSWITCH, NULL, 0);
				} else {
					curr_bank = bank_sel[i];
					fprintf( fp, "\n\n");
					fprintf( fp, "\t.bank\t%d\n", bank_sel[i] );
					putBankOrigin(fp, bank_sel[i]);
				}
			}
			
			if (trk_flag[i]) {
				cmd_buf = malloc( sizeof(CMD)*32*1024 );
				developeData( fp, i, cmd_buf, line_ptr[mml_idx] );
				free( cmd_buf );
			}
		}
	}
	fclose( fp );
	
	{

		fp = fopen( inc_name, "wt" );
		if( fp == NULL ) {
			if( message_flag == 0 ) {
				printf( "%s : ファイルが作成できませんでした。中止します。\n", inc_name );
			} else {
				printf( "%s : Don't create file. Stops.\n", inc_name );
			}
			return -1;
		}

		fprintf( fp, "TOTAL_SONGS\tequ\t$%02x\n", mml_num );
		fprintf( fp, "SOUND_GENERATOR\tequ\t$%02x\n", sndgen_flag );
		track_ptr = 0;
		track_ptr += 4;
		fprintf( fp, "PTRDPCM\t\tequ\t%2d\n", track_ptr);
		track_ptr += dpcm_track_num;
		fprintf( fp, "PTRFDS\t\tequ\t%2d\n", track_ptr);
		track_ptr += fds_track_num;
		fprintf( fp, "PTRVRC7\t\tequ\t%2d\n", track_ptr);
		track_ptr += vrc7_track_num;
		fprintf( fp, "PTRVRC6\t\tequ\t%2d\n", track_ptr);
		track_ptr += vrc6_track_num;
		fprintf( fp, "PTRN106\t\tequ\t%2d\n", track_ptr);
		track_ptr += n106_track_num;
		fprintf( fp, "PTRFME7\t\tequ\t%2d\n", track_ptr);
		track_ptr += fme7_track_num;
		fprintf( fp, "PTRMMC5\t\tequ\t%2d\n", track_ptr);
		track_ptr += mmc5_track_num;
		fprintf( fp, "PTR_TRACK_END\t\tequ\t%2d\n", track_ptr);
		
		//fprintf( fp, "INITIAL_WAIT_FRM\t\tequ\t%2d\n", 0x26);
		fprintf( fp, "PITCH_CORRECTION\t\tequ\t%2d\n", pitch_correction);
		fprintf( fp, "DPCM_RESTSTOP\t\tequ\t%2d\n", dpcm_reststop);
		fprintf( fp, "DPCM_BANKSWITCH\t\tequ\t%2d\n", dpcm_bankswitch);
		fprintf( fp, "DPCM_EXTRA_BANK_START\t\tequ\t%2d\n", bank_maximum+1);
		fprintf( fp, "BANK_MAX_IN_4KB\t\tequ\t(%d + %d)*2+1\n", bank_maximum, dpcm_extra_bank_num);

		if (!allow_bankswitching || (!dpcm_bankswitch && (bank_maximum + dpcm_extra_bank_num <= 3))) {
			fprintf( fp, "ALLOW_BANK_SWITCH\t\tequ\t0\n");
		} else {
			fprintf( fp, "ALLOW_BANK_SWITCH\t\tequ\t1\n");
			fprintf( fp, "BANKSWITCH_INIT_MACRO\t.macro\n");
			switch (bank_maximum) {
			case 0:
				fprintf(fp, "\tdb\t0,1,0,0,0,0,0,0\n");
				break;
			case 1:
				fprintf(fp, "\tdb\t0,1,2,3,0,0,0,0\n");
				break;
			case 2:
				fprintf(fp, "\tdb\t0,1,2,3,4,5,0,0\n");
				break;
			case 3:
			default:
				fprintf(fp, "\tdb\t0,1,2,3,4,5,6,7\n");
				break;
			}
			fprintf(fp, "\t.endm\n");
		}
		
		/* 出力ファイルにタイトル/作曲者/打ち込み者の情報をマクロとして書き込み */
		writeSongInfoMacro(fp);

		fprintf(fp,"\n\n");
		fclose( fp );
	}

	if( error_flag == 0 ) {
		
		/* 全てのMMLについて */
		for (mml_idx = 0; mml_idx < mml_num; mml_idx++) {
			printf("\n");
			if (mml_num > 1) {
				printf(	"Song %d: %s\n", mml_idx+1, mml_names[mml_idx]);
			}
			printf(	"-------+---------------+---------------+\n"
				"Track  |     Total     |     Loop      |\n"
				" Symbol|(count)|(frame)|(count)|(frame)|\n"
				"-------+-------+-------+-------+-------+\n");
			for (i = 0; i < _TRACK_MAX; i++) {
				if (trk_flag[i])
					display_counts_sub(i, str_track[i]);
			}
			printf(	"-------+-------+-------+-------+-------+\n");
		}
		return 0;
	} else {
		remove( out_name );				/* エラーがあったときは出力ファイルを削除 */
		remove( ef_name );
		return -1;
	}
}
