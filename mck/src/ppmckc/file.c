/************************************************************/
/*															*/
/************************************************************/
#include	<stddef.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

extern int message_flag;

/* -------------------------------------------------------------
 * 文字がパスデリミタかどうか判断
 * 漢字ファイル名未対応?
 * -------------------------------------------------------------*/
int is_pathdelimiter(char c)
{
#if defined(UNIX) || defined(LINUX)
	return (c == '/');
#else
	return (c == '\\' || c == '/');
#endif
}



/*--------------------------------------------------------------
	文字列ptrをパス/ファイルネーム/拡張子に分割(path,name,extにコピー)
 Input:

 Output:
	none
--------------------------------------------------------------*/
void splitPath( const char *ptr, char *path, char *name, char *ext )
{
	const char *temp, *eopath, *eoname;

	if( *ptr == '\0' ) {								/* エラーチェック */
		*path = '\0';
		*name = '\0';
		*ext = '\0';
		return;
	}

	temp = ptr;
	while( *temp != '\0' ) ++temp;						/* 一度最後まで進む */
	eoname = temp;
	while( --temp != ptr ) {
		if( is_pathdelimiter(*temp) ) {
			temp++;
			break;
		}
	}
	eopath = temp;
	while( *temp != '\0' ) {
		if ( *temp == '.' ) {
			eoname = temp;
		}
		++temp;
	}
	strncpy( path, ptr, eopath-ptr );
	*(path+(eopath-ptr)) = '\0';
	strncpy( name, eopath, eoname-eopath );
	*(name+(eoname-eopath)) = '\0';
	strcpy( ext, eoname );
}



/*--------------------------------------------------------------
  	path + name + ext を ptr にコピー
 Input:

 Output:
	none
--------------------------------------------------------------*/
void makePath( char *ptr, const char *path, const char *name, const char *ext )
{
	strcpy( ptr, path );
	strcat( ptr, name );
	strcat( ptr, ext );
}



/*--------------------------------------------------------------
	ファイルサイズを求める
 Input:
	char	fname	:FILE NAME
 Return:
	ファイルサイズ(0の場合はエラー)
--------------------------------------------------------------*/
int getFileSize( char *ptr )
{
	FILE	*fp;
	int		size;

	fp = fopen( ptr, "rb" );
	if( fp == NULL ) return 0;

	fseek( fp, 0L, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0L, SEEK_SET );
	fclose( fp );

	return size;
}



/*--------------------------------------------------------------
	改行コードを変更しながらファイルを読み込む
 Input:
	char	fname	:FILE NAME
 Return:
	ポインタ(NULLの場合はエラー)
--------------------------------------------------------------*/
char *readTextFile( char *filename )
{
	
	FILE *fp;
	int size, sizeb, line_idx;
	char *top, *p;
	int c, c2;
	/* ファイルオープン */
	fp = fopen(filename, "rb");
	if (!fp) {
		if (message_flag == 0) {
			printf( "%s : ファイルが開けません\n", filename );
		} else {
			printf( "%s : Can't open file\n", filename );
		}
		return NULL;
	}
	/* サイズを取得 */
	fseek(fp, 0L, SEEK_END);
	sizeb = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	/*
	if (sizeb == 0) {
		fclose(fp);
		if (message_flag == 0) {
			printf( "%s : ファイルサイズが0です\n", filename );
		} else {
			printf( "%s : File is empty\n", filename );
		}
		return NULL;
	}
	*/
	/* バッファを作成 */
	top = malloc((sizeb + 1) * sizeof(c)); /* 末尾にナル文字を付加するので1バイト多く */
	if (top == NULL) {
		fclose(fp);
		if (message_flag == 0) {
			printf( "%s : メモリが確保出来ませんでした\n", filename );
		} else {
			printf( "%s : Out of memory\n", filename );
		}
		return NULL;
	}

	/*
	size = fread(str, 1, sizeb, fp);
	*/
	line_idx = 1;
	p = top;
	while (1) {
		c = fgetc(fp);
		if (c == EOF) {
			/* file end */
			*p++ = '\0';
			break;
		} else if (c == '\x0d') {
			/* CRLF or CR ? */
			c2 = fgetc(fp);
			if (c2 == '\x0a'){
				/* CRLF */;
				*p++ = '\n';
				line_idx++;
			} else if (c2 == EOF){
				/* CR */;
				/* file end */
				*p++ = '\n';
				*p++ = '\0';
				line_idx++;
				break;
			} else {
				/* CR */;
				*p++ = '\n';
				line_idx++;
				c2 = ungetc(c2, fp);
				if (c2 == EOF) {
					/* ungetc fail */
					fclose(fp);
					free(top);
					printf("%s : ungetc failed\n", filename);
					return NULL;
					
				}
			}
		} else if (c == '\x0a'){
			/* LF */
			*p++ = '\n';
			line_idx++;
		} else if (c == '\0') {
			/* may be binary file */
			fclose(fp);
			free(top);
			if (message_flag == 0) {
				printf("%s : 不適切な文字'\\0'が見つかりました(おそらくバイナリファイルを開いた)\n", filename);
			} else {
				printf("%s : Illegal charcter '\\0' found (file may be a binary file)\n", filename);
			}
			return NULL;
		} else {
			/* other char */
			*p++ = c;
		}
	}
	fclose(fp);
	size = (p - top) / sizeof(c);
	/*
	printf("read %d byte -> store %d byte (\\0 を含む) \n", sizeb, size);
	printf("read %d line\n", line_idx);
	*/
	if (size > sizeb + 1) {
		free(top);
		if (message_flag == 0) {
			printf("%s : 確保したメモリ以上にファイルを読み込みました\n", filename);
		} else {
			printf("%s : File was read exceeding allocated memory\n", filename);
		}
		return NULL;
	}
	return top;
}





/*--------------------------------------------------------------

--------------------------------------------------------------*/
static int dmcpath_inited = 0;
static char dmcpath[10][512];

static void initDmcPath(void)
{
	char *p,*pl;
	int i, l;
	dmcpath_inited = 1;
	p = getenv("DMC_INCLUDE");
	if (p == NULL)
		return;
	
	for (i = 0; i < 10; i++) {
		pl = strchr(p, ';');

		if (pl == NULL)
			l = strlen(p);
		else
			l = pl-p;

		if (l == 0) {
			dmcpath[i][0] = '\0';
		} else {
			strncpy(dmcpath[i],p,l);
			p += l;
			while (*p == ';') p++;
		}

		if (!is_pathdelimiter(dmcpath[i][strlen(dmcpath[i])])) {
			strcat(dmcpath[i], "/");
		}
	}
}



FILE *openDmc(char *name)
{
	FILE 	*fileptr;
	char	testname[512];
	int	i;

	if (!dmcpath_inited) {
		initDmcPath();
	}
	
	fileptr = fopen(name, "rb");
	if (fileptr != NULL) return(fileptr);

	for (i = 0; i < 10; i++) {
		if (strlen(dmcpath[i])) {
			strcpy(testname, dmcpath[i]);
			strcat(testname, name);
	
			fileptr = fopen(testname, "rb");
			if (fileptr != NULL) break;
		}
	}

	return (fileptr);
}
