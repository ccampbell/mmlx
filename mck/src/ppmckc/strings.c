/************************************************************/
/*															*/
/************************************************************/
#include	<stddef.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>


/*--------------------------------------------------------------
	スペース／タブのスキップ
 Input:
	char	*ptr		:データ格納ポインタ
 Output:
	char	*ptr		;スキップ後のポインタ
--------------------------------------------------------------*/
char *skipSpaceOld( char *ptr )
{
	while( *ptr != '\0' ) {
		if( *ptr != ' ' && *ptr != '\t' ) {
			break;
		}
		ptr++;
	}
	return ptr;
}


/*--------------------------------------------------------------
	文字列のスキップ
--------------------------------------------------------------*/

char *skipQuote( char *ptr )
{
	if (*ptr && 
	    *ptr == '\"')
	{
		ptr++; // skip start charactor
		while( *ptr )
		{
			if ( *ptr == '\"') // end of the quote
			{
				ptr++; break;
			}

			if ( *ptr == '\\' && *( ptr+1 ) ) // skip Escape
				ptr++;
			ptr++;
		}
	}
	return ptr;
}

/*--------------------------------------------------------------
	コメント文字のチェック
--------------------------------------------------------------*/
int  isComment( char *ptr )
{
	if (*ptr && 
	    ( *ptr == ';' ||
//	   (*ptr == '/' && *(ptr+1) == '/') )
	    *ptr == '/' ) )
		return 1;

	return 0;
}

/*--------------------------------------------------------------
	コメントのスキップ
--------------------------------------------------------------*/
char *skipComment( char *ptr )
{
	if (isComment(ptr))
	{
		while(1) 
		{
			// '\0' = EOL or EOF , '\n' = EOL
			if (*ptr == '\0' || *ptr == '\n') 
				break;
			ptr++;
		}
	}
	return  ptr;
}


/*--------------------------------------------------------------
	スペース／タブのスキップ(行コメントも飛ばす)
--------------------------------------------------------------*/
char *skipSpace( char *ptr )
{
	while (1) {
		if (*ptr == '\0') break; //EOL or EOF
		if (*ptr == ' ' || *ptr == '\t') {
			//Skip Space
			ptr++;
			continue;
		} else if ( isComment(ptr) ) {
			//Skip Comment(return EOL)
			ptr = skipComment( ptr );
		} else {
			//Normal Chars
			break;
		}
	}
	return ptr;
}




/*----------------------------------------------------------
	文字が漢字かどうかのチェック
 Input:
	char	c	: 文字
 Return:
	0:漢字以外 1: 漢字コード
----------------------------------------------------------*/
int checkKanji( unsigned char c )
{
	if( 0x81 <= c && c <= 0x9f ) return 1;
	if( 0xe0 <= c && c <= 0xef ) return 1;
	return 0;
}



/*----------------------------------------------------------
	文字列を大文字にする(漢字対応版)
 Input:
	char *ptr	: 文字列へのポインタ
 Output:
	none
----------------------------------------------------------*/
void strupper( char *ptr )
{
	while( *ptr != '\0' ) {
		if( checkKanji( (unsigned char)*ptr ) == 0 ) {
			*ptr = toupper( (int)*ptr );
			ptr++;
		} else {
			/* 漢字の時の処理 */
			ptr += 2;
		}
	}
}




/*--------------------------------------------------------------
	文字列を数値に変換
 Input:

 Output:

--------------------------------------------------------------*/
int Asc2Int( char *ptr, int *cnt )
{
	int		num;
	char	c;
	int		minus_flag = 0;

	num = 0;
	*cnt = 0;

	if( *ptr == '-' ) {
		minus_flag = 1;
		ptr++;
		(*cnt)++;
	}
	switch( *ptr ) {
	/* 16進数 */
	  case 'x':
	  case '$':
		ptr++;
		(*cnt)++;
		while( 1 ) {
			c = toupper( *ptr );
			if( '0' <= c && c <= '9' ) {
				num = num * 16 + (c-'0');
			} else if( 'A' <= c && c <= 'F' ) {
				num = num * 16 + (c-'A'+10);
			} else {
				break;
			}
			(*cnt)++;
			ptr++;
		}
		break;
	/* 2進数 */
	  case '%':
		ptr++;
		(*cnt)++;
		while( 1 ) {
			if( '0' <= *ptr && *ptr <= '1' ) {
				num = num * 2 + (*ptr-'0');
			} else {
				break;
			}
			(*cnt)++;
			ptr++;
		}
	/* 10進数 */
	  default:
		while( 1 ) {
			if( '0' <= *ptr && *ptr <= '9' ) {
				num = num * 10 + (*ptr-'0');
			} else {
				break;
			}
			(*cnt)++;
			ptr++;
		}
	}
	if( minus_flag != 0 ) {
		num = -num;
	}
	return num;
}

