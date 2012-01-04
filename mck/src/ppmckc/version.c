char *patchstr = "patches FDS enable patch([OK]), 4-46, 4-356, 5-17, 5-95, 5-313, 5-658\n";
char *hogereleasestr = ("ppmck release 9 by h7\nppmck release 9 ex5 by BouKiCHi\n");

#ifdef ENGLISH
#define	LANGUAGE		1			// 0だとデフォルトで日本語 1だと英語
#else
#define LANGUAGE		0
#endif

int message_flag = LANGUAGE;			// 表示メッセージの出力設定( 0:Jp 1:En )

