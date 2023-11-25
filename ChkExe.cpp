


//
//	EXEファイルを調べる
//
//	since 2023/11/07 by BESHI
//



//#define	CommandLine



/*
ChkExe.CPP 62: // TextLineInfo
ChkExe.CPP 394: // 外部変数
ChkExe.CPP 425: // 空白をスキップする SkipSpace()
ChkExe.CPP 446: // 次の空白までスキップする SkipToSpace()
ChkExe.CPP 467: // 空白をスキップする SkipSpace()
ChkExe.CPP 486: // 空白以外をスキップする SkipToSpace()
ChkExe.CPP 504: // 文字列を表示する Print()
ChkExe.CPP 755: // MS-DOSヘッダを調べる CheckMsdosHeader()
ChkExe.CPP 818: // NTヘッダを調べる CheckNtHeader()
ChkExe.CPP 1382: // セクションテーブルを調べる CheckSectionTable()
ChkExe.CPP 1548: // EXEファイルを調べる CheckExe()
ChkExe.CPP 1599: // WinMain()
ChkExe.CPP 1717: // GetAppDataFolder()
ChkExe.CPP 1739: // WndProc()
*/



#include	<windows.h>
#include	<commdlg.h>
#include	<shlobj.h>
#include	<stdio.h>

#include	"ChkExe.h"



char	ClassName[] = "CheckExeClass";
char	AppName[] = "Check EXE";



HWND		hwndMain = NULL;
HINSTANCE	hInstance = NULL;



char *		ExeFilePath[ MAX_PATH ];
const DWORD	ReadBufferSize = 3000;
char *		ReadBuffer[ ReadBufferSize + 100 ];
int			fRead = 0;	// 読み込めたらnot0



//
// TextLineInfo													//TAG_JUMP_MARK
//
//	行の追加
//		TextLineInfo *	p = new TextLineInfo;
//		if( p == NULL )		...
//		if( p->AddText("text") == NULL ){
//			delete p;
//			...
//		}
//		p->SetToLast();
//
class TextLineInfo
{

	char *			pText;
	DWORD			nTextLength;

	TextLineInfo *	pPrev;	// 前の行
	TextLineInfo *	pNext;	// 次の行

	int	fInList;	// リストに属していればnot0

	//////

	static TextLineInfo *	pTextLineTop;		// 最初の行
	static TextLineInfo *	pTextLineLast;		// 最後の行
	static TextLineInfo *	pTextLineCurrent;	// 画面の１番上の行
	static DWORD	nNumLine;		// リスト中の現在の行数
	static DWORD	nMaxLine;		// リスト中の最大行数
	static int		nAddMode;		// 追加と削除の方法
		// 0:最終行の次へ追加、最大行数を超えた時は先頭行を削除する
		// 1:先頭行の前へ追加、最大行数を超えた時は最終行を削除する

	//////

	public:

	TextLineInfo () {

		if( pTextLineTop == NULL )
			pTextLineTop = pTextLineLast = pTextLineCurrent = this;

		pText = NULL;  pPrev = NULL; pNext = NULL;
		nTextLength = 0;
		fInList = 0;	// まだリストに属していない

	}

	~TextLineInfo () {

		Disconnect();	// リストから切り離す

		delete[] pText;
		nTextLength = 0;
		pText = NULL;  pPrev = NULL; pNext = NULL;
		fInList = 0;

	}

	// テキストを登録する
	char *	AddText ( char *pStr ) {

		if( pStr == NULL )		return NULL;

		delete[]	pText;	// 登録していたテキストを削除

		nTextLength = lstrlen( pStr );
		pText = new char[ nTextLength+16 ];
		if( pText == NULL )		return NULL;

		*pText = 0;
		if( nTextLength == 0 )	return pStr;

		lstrcpy( pText, pStr );

		return pStr;

	}

	// pP の次に自分を挿入
	TextLineInfo * Insert ( TextLineInfo * pP ) {

		if( pP == NULL )		return NULL;
		if( pP == pPrev )		return pP;
		if( pP == this )		return this;

		if( ( ! fInList ) && ( nNumLine >= nMaxLine )  )	return NULL;

		Disconnect();	// 自分をリストから切り離す

		pNext = pP->pNext;
		pP->pNext = this;
		pPrev = pP;
		if( pNext != NULL )	pNext->pPrev = this;

		fInList = 1;

		++nNumLine;
		CheckNumLine();	// 最大行数を超えたら超えた分を削除

		return pP;

	}

	// 自分をリストから切り離す
	void Disconnect () {

		if( ! fInList )		return;	// リストに所属していない

		if( pTextLineTop == this )		pTextLineTop = pNext;
		if( pTextLineLast == this )		pTextLineLast = pPrev;
		if( pTextLineCurrent == this ){
			if( pNext != NULL )	pTextLineCurrent = pNext;
			else				pTextLineCurrent = pPrev;
		}

		if( pPrev != NULL )		pPrev->pNext = pNext;
		if( pNext != NULL )		pNext->pPrev = pPrev;

		pNext = pPrev = NULL;
		fInList = 0;

		if( nNumLine != 0 )	--nNumLine;

	}

	// 先頭行へ移動もしくは追加する
	//	それまで先頭だった行のアドレスを返す。
	TextLineInfo * SetToTop () {

		if( pTextLineTop == this )	return this;
		if( ( ! fInList )&&( nAddMode != 0 )&&( nNumLine >= nMaxLine )  ){
			// 自分がまだリストに所属していなくて、
			// 最大行数をオーバーした時に先頭行を削除するモードの場合は、
			// 最大行数に達していた時は追加を諦める。
			return NULL;
		}

		Disconnect();	// 自分をリストから切り離す

		if( pTextLineTop == NULL ){	// リストが空の場合
			pTextLineTop = this;
			pTextLineLast = this;
			pTextLineCurrent = this;
			fInList = 1;
			nNumLine = 1;
			return NULL;
		}

		// リストの先頭に自分を追加

		TextLineInfo *	mp = pTextLineTop;

		pNext = mp;
		mp->pPrev = this;
		pTextLineTop = this;

		fInList = 1;

		++nNumLine;
		CheckNumLine();	// 最大行数をオーバーしたらその分を削除する

		return mp;

	}

	// 最終行へ移動もしくは追加する
	//	それまで末尾だった行のアドレスを返す。
	TextLineInfo * SetToLast () {

		if( pTextLineLast == this )		return this;
		if( ( ! fInList )&&( nAddMode == 0 )&&( nNumLine >= nMaxLine )  ){
			// 自分がまだリストに所属していなくて、
			// 最大行数をオーバーした時に最終行を削除するモードの場合は、
			// 最大行数に達していた時は追加を諦める。
			return NULL;
		}

		Disconnect();	// 自分をリストから切り離す

		if( pTextLineLast == NULL ){	// リストが空の場合
			pTextLineTop = this;
			pTextLineLast = this;
			pTextLineCurrent = this;
			fInList = 1;
			nNumLine = 1;
			return NULL;
		}

		// リストの末尾に自分を追加

		TextLineInfo *	mp = pTextLineLast;

		pPrev = mp;
		mp->pNext = this;
		pTextLineLast = this;

		fInList = 1;

		++nNumLine;
		CheckNumLine();

		return mp;

	}

	// 自分を画面の１番上の行にする
	//	それまで画面の１番上だった行のアドレスを返す。
	TextLineInfo * SetToCurrent () {

		if( ! fInList )		return NULL;	// リストに所属していない

		TextLineInfo *	mp = pTextLineCurrent;
		pTextLineCurrent = this;
		return mp;

	}

	// 指定行を画面の１番上の行にする
	//	それまで画面の１番上だった行のアドレスを返す。
	TextLineInfo * SetToCurrent ( DWORD nLine ) {

		TextLineInfo *	mp = pTextLineCurrent;
		pTextLineCurrent = GetLine( nLine );
		return mp;

	}

	char *	GetTextLine () {  return pText;  }
	DWORD	GetTextLength () {  return nTextLength;  }
	TextLineInfo * GetNextLine () {  return pNext;  }
	TextLineInfo * GetPrevLine () {  return pPrev;  }

	TextLineInfo * GetTopLine () {  return pTextLineTop;  }
	TextLineInfo * GetLastLine () {  return pTextLineLast;  }
	static TextLineInfo * GetCurrentLine () {  return pTextLineCurrent;  }
	static TextLineInfo * SetCurrentLine ( DWORD nLine ){
		TextLineInfo *	mp = pTextLineCurrent;
		pTextLineCurrent = GetLine( nLine );
		return mp;
	}
	DWORD	GetNumLine () {  return nNumLine;  }
	void	SetNumLine ( DWORD n ){  nNumLine = n;  }
	DWORD	GetMaxLine () {  return nMaxLine;  }
	void	SetMaxLine ( DWORD n ){  nMaxLine = n;  }
	int		GetAddMode () {  return nAddMode;  }
	void	SetAddMode ( int n ){  nAddMode = n;  }

	// 先頭から nLine番目の行を返す
	static TextLineInfo * GetLine ( DWORD nLine ) {

		TextLineInfo *	p = pTextLineTop;
		if( p == NULL )		return NULL;

		for( int i=0; i<nLine; ++i ){
			if( p->pNext == NULL )		break;
			p = p->pNext;
		}

		return p;

	}

	// リストの先頭から指定行数を削除する
	//	新しい先頭行のアドレスを返す。
	TextLineInfo * DeleteLinesTop ( DWORD nLine ) {

		TextLineInfo *	p = pTextLineTop;
		for( int i=0; i<nLine; ++i ){
			if( p == NULL )	break;
			TextLineInfo *	mp = p->pNext;
			delete p;
			p = mp;
		}
		return pTextLineTop = p;

	}

	// リストの最後から指定行数を削除する
	//	新しい末尾行のアドレスを返す。
	TextLineInfo * DeleteLinesLast ( DWORD nLine ) {

		TextLineInfo *	p = pTextLineLast;
		for( int i=0; i<nLine; ++i ){
			if( p == NULL )	break;
			TextLineInfo *	mp = p->pPrev;
			delete p;
			p = mp;
		}
		return pTextLineLast = p;

	}

	// リストを削除する
	static void DeleteTextLineAll () {

		TextLineInfo *	p = pTextLineTop;
		while( p != NULL ){
			TextLineInfo *	mp = p->pNext;
			delete p;
			p = mp;
		}
		nNumLine = 0;
		pTextLineTop = pTextLineLast = pTextLineCurrent = NULL;

	}

	// 行数を調べる
	//	最大行数を超えていたら、超えた分を削除する。
	//	削除の方法は nAddModeに従う。
	//	新しい先頭行もしくは末尾行のアドレスを返す。
	TextLineInfo * CheckNumLine () {

		if( nNumLine <= nMaxLine )		return NULL;

		int		n = nMaxLine - nNumLine;
		if( nAddMode == 0 )	return DeleteLinesTop( n );
		else				return DeleteLinesLast( n );

	}

};

TextLineInfo * TextLineInfo::pTextLineTop = NULL;		// 最初の行
TextLineInfo * TextLineInfo::pTextLineLast = NULL;		// 最後の行
TextLineInfo * TextLineInfo::pTextLineCurrent = NULL;	// 画面の１番上の行
DWORD	TextLineInfo::nNumLine = 0;		// リスト中の行数
DWORD	TextLineInfo::nMaxLine = 1000;	// リスト中の最大行数
int		TextLineInfo::nAddMode = 0;		// 追加と削除の方法



// 外部変数														//TAG_JUMP_MARK



DWORD	nLineHeight;	// １行の高さ GetTextMetrics()
const DWORD	MaxLineNumber = 1000;		// 最大行数
//char *	pLine[ MaxLineNumber ];	// 表示文字列のアドレスの配列
DWORD	nMaxLine = 0;		// 現在の最大行数



SCROLLINFO	si;
int			nMaxDisp;	// 最大表示可能行数
int			nPos;		// スクロール位置、１行目に表示する行の番号
int			nMaxPos;	// nPosの最大値+1



WORD	NumSections = 0;	// セクション数
DWORD	EntryPointRVA = 0;	// 実行開始位置(RVA)
DWORD	SectionAlignment = 0;	// セクションのサイズの単位量
DWORD	FileAlignment = 0;	// ファイル内でのセクションのサイズの単位量
int		fMachine64 = 0;		// 64bitCPUの時にnot0
int		fCIL = 0;			// プログラムが .NETのメタデータである時にnot0



LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );



// 空白をスキップする SkipSpace()								//TAG_JUMP_MARK
char * SkipSpace ( char * pStr )
{

	if( pStr == NULL )		return NULL;

	char *	p = pStr;

	for( ; *p != 0; ++p ){
		if( *p == ' ' )		continue;
		if( *p == '\t' )	continue;
		break;
	}

	return p;

}
//char * SkipSpace ( char * pStr )



// 次の空白までスキップする SkipToSpace()						//TAG_JUMP_MARK
char * SkipToSpace ( char * pStr )
{

	if( pStr == NULL )		return NULL;

	char *	p = pStr;

	for( ; *p != 0; ++p ){
		if( *p == ' ' )		break;
		if( *p == '\t' )	break;
	}

	return p;

}
//char * SkipToSpace ( char * pStr )



/*
// 空白をスキップする SkipSpace()								//TAG_JUMP_MARK
char * SkipSpace ( char * pStr )
{

	if( pStr == NULL )		return NULL;

	char *	p = pStr;

	for( ; *p != 0; ++p )  if( *p != ' ' )  break;

	return p;

}
//char * SkipSpace ( char * pStr )
*/



/*
// 空白以外をスキップする SkipToSpace()							//TAG_JUMP_MARK
char * SkipToSpace ( char * pStr )
{

	if( pStr == NULL )		return NULL;

	char *	p = pStr;

	for( ; *p != 0; ++p )  if( *p == ' ' )  break;

	return p;

}
//char * SkipToSpace ( char * pStr )
*/



// 文字列を表示する Print()										//TAG_JUMP_MARK
void Print ( char * pStr )
{

	if( pStr == NULL )		return;

	#ifdef CommandLine
	printf( "%s\n", pStr );
	#endif

	//int	len = lstrlen( pStr );
	//pLine[ nMaxLine ] = new char[len+1];
	//if( pLine[nMaxLine] == NULL )	return;
	//lstrcpy( pLine[nMaxLine], pStr );

	TextLineInfo * p = new TextLineInfo;
	if( p == NULL )		return;
	if( p->AddText( pStr ) == NULL ){  delete p;  return;  }
	p->SetToLast();

	//++nMaxLine;
	nMaxLine = p->GetNumLine();;
	nMaxPos = nMaxLine - nMaxDisp;	// nPosの最大値+1

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE ;
	si.nPos = nPos;
	si.nMin = 0;
	si.nMax = nMaxLine;
	si.nPage = nMaxDisp;
	SetScrollInfo( hwndMain, SB_VERT, &si, TRUE );
	UpdateWindow( hwndMain );

	SendMessage( hwndMain, WM_VSCROLL, MAKELONG(SB_BOTTOM,0), 0L );

}
//void Print ( char * pStr )



struct ArgInfo
{
	char *		pArgv;
	ArgInfo *	pNext;
};



// コマンドラインを分解する
//
//	呼び出し元はメモリを開放すること。
//		char ** p = BCommandLineToArgvA( pCommandLine, &Argc );
//		...
//		for( int i=0; i<Argc; ++i ){
//			delete[] p[i];
//		}
//		delete[] p;
//
// App  Arg1  "Arg2"  "Ar \"g\" 3"  A"rg4"  "Arg5
//						--->  [App]  [Arg1]  [Arg2]  [Ar "g" 3]  [Arg4]  [Arg5]
// App aaa"bbb  --->  [App]  [aaabbb]
// App aaa" bbb  --->  [App]  [aaa bbb]
// App aaa" "bbb  --->  [App]  [aaa bbb]
// App aaa" " bbb  --->  [App]  [aaa ]  [bbb]
// App aaa" "bbb ccc  --->  [App]  [aaa bbb]  [ccc]
// App "  --->  [App]  []
// 'App "        '  --->  [App]  [        ]
// App aaa" "bbb" "ccc ddd  --->  [App]  [aaa bbb ccc]  [ddd]
// App"  --->  [App"]
// App""  --->  [App""]
// App" "  --->  'App" "'を実行しようとしてエラーになる
// App"" "  --->  [App""]  []
// App""" "  --->  'App""" "'を実行しようとしてエラーになる
// App"""" "  --->  [App""""]  []
// App"aaa  --->  'App"aaa'を実行しようとしてエラーになる
// App""aaa  --->  'App""aaa'を実行しようとしてエラーになる
// App"" aaa  --->  [App""]  [aaa]
//
char ** BCommandLineToArgvA( char * pCommandLine, int * pArgc )
{

	if( pCommandLine == NULL )		return NULL;
	if( pArgc == NULL )				return NULL;

	char *		pc = pCommandLine;
	ArgInfo *	pac;	// 現在の引数
	ArgInfo *	pat;	// 引数の先頭(プログラム名)
	int			nArgv = 0;

	// プログラム名の処理
	//	空白までをコピーする。
	{
	char *	mp = pc;
	pc = SkipToSpace( pc );	// 空白までスキップ
	int	len = pc - mp;
	char *	ps = new char[len+1];
	if( ps == NULL )	return NULL;
	CopyMemory( ps, mp, len );
	ps[len] = 0;
	pat = pac = new ArgInfo;
	if( pac == NULL ){
		delete[] ps;
		return NULL;
	}
	pac->pNext = NULL;
	pac->pArgv = ps;
	++nArgv;
	pc = SkipSpace( pc );	// 引数へ
	}// end

	// 引数の処理

	int		fInArgv = 0;	// 引数処理中
	int		fInDQ = 0;		// ""内処理中
	int		fDQ = 0;		// " が出た (最初の " は除く)
	int		fYen = 0;		// ￥ が出た

	// 引数ごとの処理
	while( *pc != 0 ){

		if( *pc == '"' ){  fInArgv = fInDQ = 1;  ++pc;  }

		char *	p = pc;
		char *	mpc = pc;	// コピーすべき引数の先頭

		for( ; *p!=0; ++p ){

			// スペースの処理
			//	""内でなければ処理終了
			if(  ( *p == ' ' )|| ( *p == '\t' )  ){
				if( ! fInDQ )	break;
				fInArgv = 1;
				fYen = fDQ = 0;
				continue;
			}

			// " の処理
			//	直前に ￥ があればそのままスルー。
			//	""内の処理中の時は、
			//		次がスペースなら、この引数の処理終了。
			//		次がスペースでない時は、""内の処理終了。
			//	""内の処理中でない時は、
			//		""内の処理を始める。
			if( *p == '"' ){
				if( ! fYen ){
					if( ! fInDQ )	fInDQ = 1;
					else{
						if(  ( *(p+1) == ' ' )||( *(p+1) == '\t' )  )
							break;
						else
							fInDQ = 0;
					}
				}
				fInArgv = 1;
				fYen = 0;
				fDQ = 1;
				continue;
			}

			fYen = 0;

			if( *p == '\\' )	fYen = 1;

			fInArgv = 1;

		}// for
		fInArgv = fInDQ = 0;
		fYen = fDQ = 0;

		// ArgInfo を確保
		int	len = p - mpc;
		ArgInfo *	pa = new ArgInfo;
		if( pa == NULL )	break;
		pa->pNext = NULL;
		pa->pArgv = new char[len+1];
		if( pa->pArgv == NULL ){
			delete pa;
			break;
		}

		// 引数をコピー
		//	￥”は￥を消して ”だけをコピー。
		//	￥無しの ”はコピーしない。
		{
		char *	pS = mpc;
		char *	pD = pa->pArgv;
		for( int i=0; i<len; ++i ){
			if( *pS == '"' ){  ++pS;  continue;  }
			if( *pS == '\\' ){
				if( *(pS+1) == '"' ){  ++pS;  ++i;  }
			}
			*pD++ = *pS++;
		}
		*pD = 0;
		}// end

		// pat はNULLではない
		pac->pNext = pa;
		pac = pa;

		++nArgv;

		// 次の引数へ
		pc = SkipToSpace( p );	// " が残っていたらそれをスキップ
		pc = SkipSpace( pc );	// スペースをスキップ

	}// while

	// 呼び出し元に渡す配列を確保
	char **	pArgv = new char*[nArgv];
	if( pArgv == NULL ){
		ArgInfo *	p = pat;
		while( p != NULL ){
			ArgInfo * mp = p->pNext;
			delete[]	p->pArgv;
			delete		p;
			p = mp;
		}
		return NULL;
	}

	// それぞれの引数のアドレスを配列にセット
	{
	ArgInfo *	p = pat;
	for( int i=0; i<nArgv; ++i ){
		pArgv[i] = NULL;
		if( p != NULL ){
			pArgv[i] = p->pArgv;
			p = p->pNext;
		}
	}
	}// end

	// ArgInfo を削除
	{
	ArgInfo *	p = pat;
	while( p != NULL ){
		ArgInfo * mp = p->pNext;
		delete	p;
		p = mp;
	}
	}// end

	*pArgc = nArgv;

	return pArgv;

}



// MS-DOSヘッダを調べる CheckMsdosHeader()						//TAG_JUMP_MARK
// MS-DOSヘッダの構造
// WinNT.h
//typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
//    WORD   e_magic;                     // Magic number
//				//"MZ"
//    WORD   e_cblp;                      // Bytes on last page of file
//    WORD   e_cp;                        // Pages in file
//    WORD   e_crlc;                      // Relocations
//    WORD   e_cparhdr;                   // Size of header in paragraphs
//    WORD   e_minalloc;                  // Minimum extra paragraphs needed
//    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
//    WORD   e_ss;                        // Initial (relative) SS value
//    WORD   e_sp;                        // Initial SP value
//    WORD   e_csum;                      // Checksum
//    WORD   e_ip;                        // Initial IP value
//    WORD   e_cs;                        // Initial (relative) CS value
//    WORD   e_lfarlc;                    // File address of relocation table
//    WORD   e_ovno;                      // Overlay number
//    WORD   e_res[4];                    // Reserved words
//    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
//    WORD   e_oeminfo;                   // OEM information; e_oemid specific
//    WORD   e_res2[10];                  // Reserved words
//    LONG   e_lfanew;                    // File address of new exe header
//				// NTヘッダへのオフセット
//  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;
//	Windpwsでは、e_magicとe_lfanewのみ使用。
// MS-DOSヘッダとNTヘッダの間にMS-DOSスタブプログラム
//
// C:\msys64\mingw64\include\WinNT.h
//
char * CheckMsdosHeader( char * pBuf, int * pRslt )
{

	char *	pB = pBuf;

	if( pB == NULL )
		if( pRslt != NULL ){  *pRslt = 10000;  return pB;  }

	Print( "[ MS-DOSヘッダ ]" );

	// e_magic
	Print( "  ・シグネチャ" );
	if( *pB++ != 'M' )
		if( pRslt != NULL ){  *pRslt = 10100;  return pB;  }
	if( *pB++ != 'Z' )
		if( pRslt != NULL ){  *pRslt = 10200;  return pB;  }

	// e_lfanew
	//	NTヘッダへのオフセット
	IMAGE_DOS_HEADER *	p = (IMAGE_DOS_HEADER*)pBuf;
	//printf( "%x %x\n", p->e_lfanew, sizeof(IMAGE_DOS_HEADER) );
	//printf( "%c%c\n", *(pBuf+p->e_lfanew), *(pBuf+p->e_lfanew+1) );

	if( pRslt != NULL )		*pRslt = 0;

	return pBuf+p->e_lfanew;

}
//char * CheckMsdosHeader( char * pBuf, int * pRslt )



// NTヘッダを調べる CheckNtHeader()								//TAG_JUMP_MARK
// NTへッダの構造
//	シグルチャ
//	ファイルヘッダ
//	オプショナルヘッダ
//
//WinNT.h
//typedef struct _IMAGE_NT_HEADERS {
//    DWORD Signature;
//    IMAGE_FILE_HEADER FileHeader;
//    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
//} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;
//#ifdef _WIN64
//typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
//typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;
//#else
//typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
//typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;
//#endif
//
// シグネチャ PEファイルであることを示す
//WinNT.h
//#define IMAGE_DOS_SIGNATURE                 0x4D5A      // MZ
//#define IMAGE_OS2_SIGNATURE                 0x4E45      // NE
//#define IMAGE_OS2_SIGNATURE_LE              0x4C45      // LE
//#define IMAGE_NT_SIGNATURE                  0x50450000  // PE00
//
// C:\msys64\mingw64\include\WinNT.h
//
char * CheckNtHeader( char * pBuf, int * pRslt )
{

	char *	pB = pBuf;

	if( pB == NULL )
		if( pRslt != NULL ){  *pRslt = 20000;  return pB;  }

	Print( "[ NTヘッダ ]" );

	// Signature
	Print( "  ・シグネチャ" );
	if( *pB++ != 'P' )
		if( pRslt != NULL ){  *pRslt = 20100;  return pB;  }
	if( *pB++ != 'E' )
		if( pRslt != NULL ){  *pRslt = 20200;  return pB;  }
	WORD *	pW = (WORD*)pB;
	if( *pW++ != 0 )
		if( pRslt != NULL ){  *pRslt = 20300;  return (char*)pW;  }
	pB = (char*)pW;


	//WinNT.h
	//typedef struct _IMAGE_FILE_HEADER {
	//    WORD    Machine;
	//    WORD    NumberOfSections;
	//    DWORD   TimeDateStamp;
	//    DWORD   PointerToSymbolTable;
	//    DWORD   NumberOfSymbols;
	//    WORD    SizeOfOptionalHeader;
	//    WORD    Characteristics;
	//} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;
	// C:\msys64\mingw64\include\WinNT.h
	{

	Print( "  ・ファイルヘッダ" );

	IMAGE_FILE_HEADER *	pFileHeader = (IMAGE_FILE_HEADER*)pB;
	char *	pM = "";
	int		fError =0;

	switch( pFileHeader->Machine ){

	case IMAGE_FILE_MACHINE_UNKNOWN:
		pM = "(Unknown) (x86)";			break;
	case IMAGE_FILE_MACHINE_AM33:
		pM = "Matsushita AM33";			break;
	case IMAGE_FILE_MACHINE_AMD64:
		pM = "x64";  fMachine64 = 1;	break;
	case IMAGE_FILE_MACHINE_ARM:
		pM = "ARM little endian";		break;
	case IMAGE_FILE_MACHINE_EBC:
		pM = "EFI byte code";			break;
	case IMAGE_FILE_MACHINE_I386:
		pM = "Intel 386 or later processors and compatible processors (x86)";
		break;
	case IMAGE_FILE_MACHINE_IA64:
		pM = "Intel Itanium processor family";	break;
	case IMAGE_FILE_MACHINE_M32R:
		pM = "Mitsubishi M32R little endian";	break;
	case IMAGE_FILE_MACHINE_MIPS16:
		pM = "MIPS16";					break;
	case IMAGE_FILE_MACHINE_MIPSFPU:
		pM = "MIPS with FPU";			break;
	case IMAGE_FILE_MACHINE_MIPSFPU16:
		pM = "MIPS16 with FPU";			break;
	case IMAGE_FILE_MACHINE_POWERPC:
		pM = "Power PC little endian";	break;
	case IMAGE_FILE_MACHINE_POWERPCFP:
		pM = "Power PC with floating point support";	break;
	case IMAGE_FILE_MACHINE_R4000:
		pM = "MIPS little endian";		break;
	case IMAGE_FILE_MACHINE_SH3:
		pM = "Hitachi SH3";				break;
	case IMAGE_FILE_MACHINE_SH3DSP:
		pM = "Hitachi SH3 DSP";			break;
	case IMAGE_FILE_MACHINE_SH4:
		pM = "Hitachi SH4";				break;
	case IMAGE_FILE_MACHINE_SH5:
		pM = "Hitachi SH5";				break;
	case IMAGE_FILE_MACHINE_THUMB:
		pM = "Thumb";					break;
	case IMAGE_FILE_MACHINE_WCEMIPSV2:
		pM = "MIPS little-endian WCE v2";	break;
	default:
		fError = 1;
	}// switch
	char	buf[300];
	if( fError )
		wsprintf( buf, "    Machine : Invalide Value(0x%04X)",
													pFileHeader->Machine );
	else
		wsprintf( buf, "    Machine : %s", pM );
	Print( buf );
	if( fError ){
		*pRslt = 21000;
		return pB;
	};

	NumSections = pFileHeader->NumberOfSections;
	wsprintf( buf, "    セクション数 : %d", NumSections );
	Print( buf );

	// #define IMAGE_FILE_RELOCS_STRIPPED 0x0001
	// #define IMAGE_FILE_EXECUTABLE_IMAGE 0x0002
	// #define IMAGE_FILE_LINE_NUMS_STRIPPED 0x0004
	// #define IMAGE_FILE_LOCAL_SYMS_STRIPPED 0x0008
	// #define IMAGE_FILE_AGGRESIVE_WS_TRIM 0x0010
	// #define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020
	// #define IMAGE_FILE_BYTES_REVERSED_LO 0x0080
	// #define IMAGE_FILE_32BIT_MACHINE 0x0100
	// #define IMAGE_FILE_DEBUG_STRIPPED 0x0200
	// #define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
	// #define IMAGE_FILE_NET_RUN_FROM_SWAP 0x0800
	// #define IMAGE_FILE_SYSTEM 0x1000
	// #define IMAGE_FILE_DLL 0x2000
	// #define IMAGE_FILE_UP_SYSTEM_ONLY 0x4000
	// #define IMAGE_FILE_BYTES_REVERSED_HI 0x8000
	// C:\msys64\mingw64\include\WinNT.h
	Print( "    ファイルの特性" );
	if( pFileHeader->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE )
		Print( "      実行可能" );
	if( pFileHeader->Characteristics & IMAGE_FILE_32BIT_MACHINE )
		Print( "      32ビット" );
	if( pFileHeader->Characteristics & IMAGE_FILE_SYSTEM )
		Print( "      システムファイル" );
	if( pFileHeader->Characteristics & IMAGE_FILE_DLL )
		Print( "      DLLファイル" );
	pB += sizeof( IMAGE_FILE_HEADER );

	}// end ファイルヘッダ


	//typedef struct _IMAGE_OPTIONAL_HEADER {
	//    //
	//    // Standard fields.
	//    //
	//
	//    WORD    Magic;
	//    BYTE    MajorLinkerVersion;
	//    BYTE    MinorLinkerVersion;
	//    DWORD   SizeOfCode;
	//    DWORD   SizeOfInitializedData;
	//    DWORD   SizeOfUninitializedData;
	//    DWORD   AddressOfEntryPoint;
	//    DWORD   BaseOfCode;
	//    DWORD   BaseOfData;
	//
	//    //
	//    // NT additional fields.
	//    //
	//
	//    DWORD   ImageBase;
	//    DWORD   SectionAlignment;
	//				// セクションのサイズの単位量
	//				// この量が0x1000の時、セクションの大きさが0x1010でも
	//				// セクションのサイズは0x2000とされる。
	//    DWORD   FileAlignment;
	//				// ファイル内でのセクションのサイズの単位量
	//    WORD    MajorOperatingSystemVersion;
	//    WORD    MinorOperatingSystemVersion;
	//    WORD    MajorImageVersion;
	//    WORD    MinorImageVersion;
	//    WORD    MajorSubsystemVersion;
	//    WORD    MinorSubsystemVersion;
	//    DWORD   Win32VersionValue;
	//    DWORD   SizeOfImage;
	//    DWORD   SizeOfHeaders;
	//			// ヘッダーの総サイズ
	//			// MS-DOSヘッダ、NTヘッダ、セクシションテーブルの合計
	//			// FileAlignmentの倍数
	//    DWORD   CheckSum;
	//    WORD    Subsystem;
	//    WORD    DllCharacteristics;
	//    DWORD   SizeOfStackReserve;
	//    DWORD   SizeOfStackCommit;
	//    DWORD   SizeOfHeapReserve;
	//    DWORD   SizeOfHeapCommit;
	//    DWORD   LoaderFlags;
	//    DWORD   NumberOfRvaAndSizes;
	//    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	//		// 0 	IMAGE_DIRECTORY_ENTRY_EXPORT 	エクスポート情報
	//		// 1 	IMAGE_DIRECTORY_ENTRY_IMPORT 	インポート情報
	//		// 2 	IMAGE_DIRECTORY_ENTRY_RESOURCE 	リソース
	//		// 3 	IMAGE_DIRECTORY_ENTRY_EXCEPTION 	例外情報
	//		// 4 	IMAGE_DIRECTORY_ENTRY_SECURITY 	セキュリティ情報
	//		// 5 	IMAGE_DIRECTORY_ENTRY_BASERELOC 	ベース再配置情報
	//		// 6 	IMAGE_DIRECTORY_ENTRY_DEBUG 	デバッグ情報
	//		// 7 	IMAGE_DIRECTORY_ENTRY_ARCHITECTURE アーキテクチャ固有の情報
	//		// 8 	IMAGE_DIRECTORY_ENTRY_GLOBALPTR 	グローバル ポインター
	//		//	9 	IMAGE_DIRECTORY_ENTRY_TLS 	TLS（Thread Local Storage）
	//		// 10 	IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG 	ロード構成情報
	//		// 11 	IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT
	//		//									バインドされたインポート情報
	//		// 12 	IMAGE_DIRECTORY_ENTRY_IAT 	インポート アドレス テーブル
	//		// 13 	IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 	遅延インポート情報
	//		// 14 	IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 	.NET メタデータ
	//		// 15 	（なし） 	予約
	//		//typedef struct _IMAGE_DATA_DIRECTORY {
	//		//    DWORD   VirtualAddress;	// このデータのアドレス(RVA)
	//		//    DWORD   Size;				// このデータのサイズ
	//		//} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;
	//} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;
	// C:\msys64\mingw64\include\WinNT.h
	{

	Print( "  ・オプショナルヘッダ" );

	if( fMachine64 ){

	IMAGE_OPTIONAL_HEADER64 *	pOptionalHeader = (IMAGE_OPTIONAL_HEADER64*)pB;

	switch( pOptionalHeader->Magic ){
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		Print( "    PE32" );
		break;
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		Print( "    PE+ : 64bit用のファイル" );
		break;
	case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
		Print( "    ROM" );
		break;
	default:
		{
		*pRslt = 22000;
		return pB;
		}
	}// switch

	{
	EntryPointRVA = pOptionalHeader->AddressOfEntryPoint;
	char	b[300];
	wsprintf( b, "    実行開始位置(相対仮想アドレスRVA) : 0x%08X",
															EntryPointRVA );
	Print( b );
	wsprintf( b, "    イメージファイルが読み込まれる位置(実アドレス) : 0x%08X",
												pOptionalHeader->ImageBase );
	Print( b );
	SectionAlignment = pOptionalHeader->SectionAlignment;
	wsprintf( b, "    各セクションのサイズの単位量 : 0x%08X",
														SectionAlignment );
	Print( b );
	FileAlignment = pOptionalHeader->FileAlignment;
	wsprintf( b, "    ファイル内での各セクションのサイズの単位量(1) : 0x%08X",
															FileAlignment );
	Print( b );
	wsprintf( b, "    イメージを読み込むのに必要なサイズ : 0x%08X",
												pOptionalHeader->SizeOfImage );
	Print( b );
	wsprintf( b, "    全ヘッダーのサイズ((1)の倍数) : 0x%08X",
										pOptionalHeader->SizeOfHeaders );
	Print( b );
	switch( pOptionalHeader->Subsystem ){
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
		Print( "    Windowsプログラム (GUI)" );
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
		Print( "    コンソールプログラム (CUI)" );
		break;
	case IMAGE_SUBSYSTEM_NATIVE:
		Print( "    デバイスドライバ" );
		break;
	case IMAGE_SUBSYSTEM_UNKNOWN :
		Print( "    不明なサブシステム" );
		break;
	case IMAGE_SUBSYSTEM_OS2_CUI:
		Print( "    OS/2 CUI" );
		break;
	case IMAGE_SUBSYSTEM_POSIX_CUI:
		Print( "    POSIX CUI" );
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
		Print( "    Windows CE システム" );
		break;
	case IMAGE_SUBSYSTEM_EFI_APPLICATION:
		Print( "    拡張ファームウェア インターフェイス (EFI) "
													"アプリケーション" );
		break;
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
		Print( "    ブート サービスを備えた EFI ドライバー" );
		break;
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
		Print( "    ランタイム サービスを備えた EFI ドライバー" );
		break;
	case IMAGE_SUBSYSTEM_EFI_ROM :
		Print( "    拡張ファームウェア インターフェイス (EFI)  ROM イメージ" );
		break;
	case IMAGE_SUBSYSTEM_XBOX:
		Print( "    Xbox システム" );
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
		Print( "    このブート アプリケーション" );
		break;
	default:
		{
		*pRslt = 23000;
		return pB;
		}
	}// switch

	wsprintf( b, "    スタック領域として予約されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfStackReserve );
	Print( b );
	wsprintf( b, "    スタック領域として約束されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfStackCommit );
	Print( b );
	wsprintf( b, "    ヒープ領域として予約されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfHeapReserve );
	Print( b );
	wsprintf( b, "    ヒープ領域として約束されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfHeapCommit );
	Print( b );

	Print( "    データディレクトリ" );
	for( int i=0; i<IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i ){
		if( pOptionalHeader->DataDirectory[i].Size == 0 )	continue;
		char *	pM;
		switch( i ){
		case IMAGE_DIRECTORY_ENTRY_EXPORT:
			pM = "      エクスポート情報";				break;
		case IMAGE_DIRECTORY_ENTRY_IMPORT:
			pM = "      インポート情報";				break;
		case IMAGE_DIRECTORY_ENTRY_RESOURCE:
			pM = "      リソース";						break;
		case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
			pM = "      例外情報";						break;
		case IMAGE_DIRECTORY_ENTRY_SECURITY:
			pM = "      セキュリティ情報";				break;
		case IMAGE_DIRECTORY_ENTRY_BASERELOC:
			pM = "      ベース再配置情報";				break;
		case IMAGE_DIRECTORY_ENTRY_DEBUG:
			pM = "      デバッグ情報";					break;
		case IMAGE_DIRECTORY_ENTRY_ARCHITECTURE:
			pM = "      アーキテクチャ固有の情報";		break;
		case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
			pM = "      グローバル ポインター";			break;
		case IMAGE_DIRECTORY_ENTRY_TLS:
			pM = "      TLS（Thread Local Storage）";	break;
		case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
			pM = "      ロード構成情報";				break;
		case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
			pM = "      バインドされたインポート情報"; break;
		case IMAGE_DIRECTORY_ENTRY_IAT:
			pM = "      インポート アドレス テーブル"; break;
		case IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT:
			pM = "      遅延インポート情報";			break;
		case IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR:
			pM = "      .NET メタデータ";
			fCIL = 1;
			break;
		case 15:
			pM = "      (予約)";						break;
		default:
			pM = "      (不明)";						break;
		}// switch

		Print( pM );

	}// for

	pB += sizeof( IMAGE_OPTIONAL_HEADER64 );

	}// end

	}
	else{	// fMachine64

	IMAGE_OPTIONAL_HEADER32 *	pOptionalHeader = (IMAGE_OPTIONAL_HEADER32*)pB;

	switch( pOptionalHeader->Magic ){
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		Print( "    PE32" );
		break;
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		Print( "    PE+ : 64bit用のファイル" );
		break;
	case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
		Print( "    ROM" );
		break;
	default:
		{
		*pRslt = 22000;
		return pB;
		}
	}// switch

	{
	EntryPointRVA = pOptionalHeader->AddressOfEntryPoint;
	char	b[300];
	wsprintf( b, "    実行開始位置(相対仮想アドレスRVA) : 0x%08X",
															EntryPointRVA );
	Print( b );
	wsprintf( b, "    イメージファイルが読み込まれる位置(実アドレス) : 0x%08X",
												pOptionalHeader->ImageBase );
	Print( b );
	SectionAlignment = pOptionalHeader->SectionAlignment;
	wsprintf( b, "    各セクションのサイズの単位量 : 0x%08X",
															SectionAlignment );
	Print( b );
	FileAlignment = pOptionalHeader->FileAlignment;
	wsprintf( b, "    ファイル内での各セクションのサイズの単位量(1) : 0x%08X",
															FileAlignment );
	Print( b );
	wsprintf( b, "    イメージを読み込むのに必要なサイズ : 0x%08X",
												pOptionalHeader->SizeOfImage );
	Print( b );
	wsprintf( b, "    全ヘッダーのサイズ((1)の倍数) : 0x%08X",
										pOptionalHeader->SizeOfHeaders );
	Print( b );
	switch( pOptionalHeader->Subsystem ){
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
		Print( "    Windowsプログラム (GUI)" );
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
		Print( "    コンソールプログラム (CUI)" );
		break;
	case IMAGE_SUBSYSTEM_NATIVE:
		Print( "    デバイスドライバ" );
		break;
	case IMAGE_SUBSYSTEM_UNKNOWN :
		Print( "    不明なサブシステム" );
		break;
	case IMAGE_SUBSYSTEM_OS2_CUI:
		Print( "    OS/2 CUI" );
		break;
	case IMAGE_SUBSYSTEM_POSIX_CUI:
		Print( "    POSIX CUI" );
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
		Print( "    Windows CE システム" );
		break;
	case IMAGE_SUBSYSTEM_EFI_APPLICATION:
		Print( "    拡張ファームウェア インターフェイス (EFI) "
													"アプリケーション" );
		break;
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
		Print( "    ブート サービスを備えた EFI ドライバー" );
		break;
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
		Print( "    ランタイム サービスを備えた EFI ドライバー" );
		break;
	case IMAGE_SUBSYSTEM_EFI_ROM :
		Print( "    拡張ファームウェア インターフェイス (EFI)  ROM イメージ" );
		break;
	case IMAGE_SUBSYSTEM_XBOX:
		Print( "    Xbox システム" );
		break;
	case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
		Print( "    このブート アプリケーション" );
		break;
	default:
		{
		*pRslt = 23000;
		return pB;
		}
	}// switch

	wsprintf( b, "    スタック領域として予約されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfStackReserve );
	Print( b );
	wsprintf( b, "    スタック領域として約束されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfStackCommit );
	Print( b );
	wsprintf( b, "    ヒープ領域として予約されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfHeapReserve );
	Print( b );
	wsprintf( b, "    ヒープ領域として約束されるサイズ : 0x%08X",
									pOptionalHeader->SizeOfHeapCommit );
	Print( b );

	Print( "    データディレクトリ" );
	for( int i=0; i<IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i ){
		if( pOptionalHeader->DataDirectory[i].Size == 0 )	continue;
		char *	pM;
		switch( i ){
		case IMAGE_DIRECTORY_ENTRY_EXPORT:
			pM = "      エクスポート情報";				break;
		case IMAGE_DIRECTORY_ENTRY_IMPORT:
			pM = "      インポート情報";				break;
		case IMAGE_DIRECTORY_ENTRY_RESOURCE:
			pM = "      リソース";						break;
		case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
			pM = "      例外情報";						break;
		case IMAGE_DIRECTORY_ENTRY_SECURITY:
			pM = "      セキュリティ情報";				break;
		case IMAGE_DIRECTORY_ENTRY_BASERELOC:
			pM = "      ベース再配置情報";				break;
		case IMAGE_DIRECTORY_ENTRY_DEBUG:
			pM = "      デバッグ情報";					break;
		case IMAGE_DIRECTORY_ENTRY_ARCHITECTURE:
			pM = "      アーキテクチャ固有の情報";		break;
		case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
			pM = "    グローバル ポインター";			break;
		case IMAGE_DIRECTORY_ENTRY_TLS:
			pM = "      TLS（Thread Local Storage）";	break;
		case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
			pM = "      ロード構成情報";				break;
		case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
			pM = "      バインドされたインポート情報"; break;
		case IMAGE_DIRECTORY_ENTRY_IAT:
			pM = "      インポート アドレス テーブル"; break;
		case IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT:
			pM = "      遅延インポート情報";			break;
		case IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR:
			pM = "      .NET メタデータ";
			fCIL = 1;
			break;
		case 15:
			pM = "      (予約)";						break;
		default:
			pM = "      (不明)";						break;
		}// switch

		Print( pM );

	}// for

	pB += sizeof( IMAGE_OPTIONAL_HEADER32 );

	}// end

	}// if fMachine64

	}// end オプショナルヘッダ


	if( pRslt != NULL )		*pRslt = 0;

	return pB;

}
//char * CheckNtHeader( char * pBuf, int * pRslt )



// セクションテーブルを調べる CheckSectionTable()				//TAG_JUMP_MARK
// セクションテーブルの構造
//	セクションテーブル
//		セクションヘッダ
//			...
//		セクションヘッダ
//	セクションデータ
//		...
//	セクションデータ
//
// C:\msys64\mingw64\include\WinNT.h
//
char * CheckSectionTable( char * pBuf, int * pRslt )
{

	char *	pB = pBuf;

	if( pB == NULL )
		if( pRslt != NULL ){  *pRslt = 30000;  return pB;  }

	Print( "[ セクションテーブルとセクションヘッダ ]" );

	// セクションヘッダ
	//
	//#define IMAGE_SIZEOF_SHORT_NAME              8
	//
	//typedef struct _IMAGE_SECTION_HEADER {
	//    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
	//    union {
	//            DWORD   PhysicalAddress;
	//            DWORD   VirtualSize;
	//    } Misc;
	//    DWORD   VirtualAddress;
	//    DWORD   SizeOfRawData;
	//    DWORD   PointerToRawData;
	//    DWORD   PointerToRelocations;
	//    DWORD   PointerToLinenumbers;
	//    WORD    NumberOfRelocations;
	//    WORD    NumberOfLinenumbers;
	//    DWORD   Characteristics;
	//} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;
	//
	//#define IMAGE_SIZEOF_SECTION_HEADER          40
	//
	// C:\msys64\mingw64\include\WinNT.h
	//
	{
	char	buf[ 300 ];
	char	buf2[ 100 ];

	Print( "  ・セクションテーブル" );

	IMAGE_SECTION_HEADER *	pSectionHeader = (IMAGE_SECTION_HEADER*)pB;

	for( int i=0; i<NumSections; ++i, ++pSectionHeader ){

		wsprintf( buf, "    セクションヘッダ #%d", i );
		Print( buf );

		CopyMemory( buf, pSectionHeader->Name, IMAGE_SIZEOF_SHORT_NAME );
		buf[ IMAGE_SIZEOF_SHORT_NAME ] = 0;
		wsprintf( buf2, "      セクション名 : %s", buf );
		Print( buf2 );

		wsprintf( buf, "      セクションのサイズ : 0x%08X",
											pSectionHeader->Misc.VirtualSize );
		Print( buf );

		wsprintf( buf, "      セクションの相対仮想アドレス : 0x%08X",
											pSectionHeader->VirtualAddress );
		Print( buf );

		Print( "      セクションの特性" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_TYPE_NO_PAD )
			Print( "        NO_PAD" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_CNT_CODE )
			Print( "        コードを含む" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA )
			Print( "        初期化データあり" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			Print( "        未初期化データあり" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_LNK_OTHER )
			Print( "        LNK_OTHER" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_LNK_INFO )
			Print( "        LNK_INFO" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_LNK_REMOVE )
			Print( "        LNK_REMOVE" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_LNK_COMDAT )
			Print( "        LNK_COMDAT" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC )
			Print( "        NO_DEFER_SPEC_EXC" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_GPREL )
			Print( "        GPREL" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_FARDATA )
			Print( "        MEM_FARDATA" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_PURGEABLE )
			Print( "        MEM_PURGEABLE" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_16BIT )
			Print( "        MEM_16BIT" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_LOCKED )
			Print( "        MEM_LOCKED" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_PRELOAD )
			Print( "        MEM_PRELOAD" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_1BYTES )
			Print( "        ALIGN_1BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_2BYTES )
			Print( "        ALIGN_2BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_4BYTES )
			Print( "        ALIGN_4BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_8BYTES )
			Print( "        ALIGN_8BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_16BYTES )
			Print( "        ALIGN_16BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_32BYTES )
			Print( "        ALIGN_32BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_64BYTES )
			Print( "        ALIGN_64BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_128BYTES )
			Print( "        ALIGN_128BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_256BYTES )
			Print( "        ALIGN_256BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_512BYTES )
			Print( "        ALIGN_512BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_1024BYTES )
			Print( "        ALIGN_1024BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_2048BYTES )
			Print( "        ALIGN_2048BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_4096BYTES )
			Print( "        ALIGN_4096BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_8192BYTES )
			Print( "        ALIGN_8192BYTES" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_ALIGN_MASK )
			Print( "        ALIGN_ALIGN_MASK" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL )
			Print( "        LNK_NRELOC_OVFL" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_DISCARDABLE )
			Print( "        MEM_DISCARDABLE" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_CACHED )
			Print( "        MEM_NOT_CACHED" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_PAGED )
			Print( "        MEM_NOT_PAGED" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_SHARED )
			Print( "        MEM_SHARED" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE )
			Print( "        実行可能" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_READ )
			Print( "        読み取り専用" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE )
			Print( "        書き込み可能" );
		if( pSectionHeader->Characteristics & IMAGE_SCN_SCALE_INDEX )
			Print( "        書き込み可能" );


	}// for

	}// セクションテーブル

	if( pRslt != NULL )		*pRslt = 0;

	return pB;

}
//char * CheckSectionTable( char * pBuf, int * pRslt )



// EXEファイルを調べる CheckExe()								//TAG_JUMP_MARK
//	成功したら０を返す。
//
// PEファイルフォーマットの構造(ほとんどのEXEがPE)
//	MS-DOS用ヘッダ
//	MS-DOS用スタブプログラム
//	NTヘッダ
//	セクションテーブル
//
int CheckExe ( void )
{

	if( ! fRead )		return 100;

	char	Path[ MAX_PATH ];

	wsprintf( Path, "[[[ %s ]]]", ExeFilePath );
	//MessageBox( hwndMain, Path, AppName, MB_OK );
	Print( Path );

	int		rslt;
	char *	pBuf = (char*)ReadBuffer;

	pBuf = CheckMsdosHeader( pBuf, &rslt );
	if( rslt != 0 )		return rslt;

	pBuf = CheckNtHeader( pBuf, &rslt );
	if( rslt != 0 )		return rslt;

	pBuf = CheckSectionTable( pBuf, &rslt );
	if( rslt != 0 )		return rslt;

	Print( "" );

	Print( Path );

	if( fMachine64 )		Print( "  64ビットプログラム" );
	else					Print( "  32ビットプログラム" );

	if( fCIL )	Print( "  中間言語プログラム(中間言語をデータとして持つ)" );
	else		Print( "  機械語プログラム" );

	Print( "" );

	return 0;

}
// int CheckExe ( void )



// WinMain()													//TAG_JUMP_MARK
#ifdef CommandLine
int main()
#else
int WinMain( HINSTANCE hInst, HINSTANCE hPrevInst,
												char * CmdLine, int CmdShow )
#endif
{

	//hInstance = hInst;
	hInstance = GetModuleHandle( NULL );	// コマンドライン版

	{
	int	Argc;
	char **	Argv = BCommandLineToArgvA( GetCommandLineA(), &Argc );

	if( Argc > 1 ){
	//Print( Argv[1] );
	lstrcpy( (char*)ExeFilePath, Argv[1] );

	HANDLE	hFile = CreateFile( Argv[1], GENERIC_READ,
			NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		Print( "Open Error" );
		ExitProcess( 2 );
	}

	DWORD	bytesRead;
	BOOL	fOK;
	fOK = ReadFile( hFile,
						ReadBuffer, ReadBufferSize, &bytesRead, NULL );
	CloseHandle( hFile );
	if( ! fOK ){
		Print( "Read Error" );
		ExitProcess( 3 );
	}

	fRead = 1;	// 読み込み成功

	int	rslt = CheckExe();
	if( rslt == 0 )		Print( "done." );
	else{
		char	buf[100];
		wsprintf( buf, "error : %d", rslt );
		Print( buf );
	}
	Print( "" );

	}//if

	for( int i=0; i<Argc; ++i )	delete[] Argv[i];
	delete[] Argv;

	}// end

	/*
	{
	nMaxLine = 0;
	pLine[0] = NULL;
	for( int i=0; i<100; ++i ){
		char	buf[ 100 ];
		int	len = wsprintf( buf, "%3d: 猫でもわかるスクロールバー", i );
		pLine[i] = new char[len+1];
		if( pLine[i] == NULL )		break;
		pLine[i+1] = NULL;
		lstrcpy( pLine[i], buf );
		++nMaxLine;
	}// for
	}// end
	*/

	// ウインドウクラスの登録
	{
	WNDCLASSEX	wc;
	wc.cbSize = sizeof( WNDCLASSEX );
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	//wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = "FirstMenu";
	wc.lpszClassName = ClassName;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	RegisterClassEx( &wc );
	}

	// ウインドウの作成
	{
	hwndMain = CreateWindowEx( WS_EX_CLIENTEDGE, ClassName, AppName,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_VISIBLE ,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL );
	}

	SendMessage( hwndMain, WM_VSCROLL, MAKELONG(SB_BOTTOM,0), 0L );

	// メッセージループ
	MSG	msg;
	while( GetMessage( &msg, NULL, 0, 0 ) ){
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	//for( int i=0; i<nMaxLine; ++i )
	//	delete[] pLine[i];

	TextLineInfo::DeleteTextLineAll();

	return msg.wParam;

}



// GetAppDataFolder()											//TAG_JUMP_MARK
// shlobj.h, shell32.lib
char * GetAppDataFolder( char * pBuf )
{

	if( pBuf == NULL )			return NULL;

	SHGetFolderPath(
		NULL,			//HWND   hwndOwner,
		CSIDL_APPDATA,	//int    nFolder,
		NULL,			//HANDLE hToken,
		0,				//DWORD  dwFlags,
		pBuf			//LPTSTR pszPath
	);
	//lstrcat( pBuf, "\\SubDir" );

	return pBuf;

}



// WndProc()												//TAG_JUMP_MARK
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{


	switch( uMsg ){

	case WM_CREATE:
		{
		// 初期化

		HDC	hdc = GetDC( hWnd );
		TEXTMETRIC	tm;
		GetTextMetrics( hdc, &tm );
		nLineHeight = tm.tmHeight;
		ReleaseDC( hWnd, hdc );

		// 行の高さで割り切れるサイズでないとスクロールがうまくいかないので
		// クライアント領域の高さを行の高さの倍数にする。
		RECT	r;
		GetClientRect( hWnd, &r );	// 左上座標、幅、高さ
		nPos = 0;
		nMaxDisp = r.bottom / nLineHeight;
		nMaxPos = nMaxLine - nMaxDisp;	// nPosの最大値+1
		int		def = r.bottom - nMaxDisp * nLineHeight;

		GetWindowRect( hWnd, &r );	// 左上座標、右下座標+1
		MoveWindow( hWnd,
			r.left, r.top, r.right - r.left, r.bottom - r.top  - def, TRUE );
						// 左上座標、幅、高さ

		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE ;
		si.nPos = nPos;
		si.nMin = 0;
		si.nMax = nMaxLine;
		si.nPage = nMaxDisp;
		SetScrollInfo( hWnd, SB_VERT, &si, TRUE );

		}// WM_CREATE
		break;

	case WM_SIZE:
		{

		// 行の高さで割り切れるサイズでないとスクロールがうまくいかないので
		// クライアント領域の高さを行の高さの倍数にする。
		RECT	r;
		GetClientRect( hWnd, &r );
		nMaxDisp = r.bottom / nLineHeight;
		nMaxPos = nMaxLine - nMaxDisp;	// nPosの最大値+1
		int		def = r.bottom - nMaxDisp * nLineHeight;

		GetWindowRect( hWnd, &r );	// 左上座標、右下座標+1
		MoveWindow( hWnd,
			r.left, r.top, r.right - r.left, r.bottom - r.top  - def, TRUE );
						// 左上座標、幅、高さ

		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE ;
		si.nPos = nPos;
		si.nMin = 0;
		si.nMax = nMaxLine;
		si.nPage = nMaxDisp;
		SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
		UpdateWindow( hWnd );

		SendMessage( hwndMain, WM_VSCROLL, MAKELONG(SB_BOTTOM,0), 0L );

		}// WM_SIZE
		break;

	case WM_VSCROLL:
		{

		if( nMaxLine < nMaxDisp )	break;

		int		dy;

		switch( LOWORD(wParam) ){

		case SB_LINEUP:		dy = -1;						break;
		case SB_LINEDOWN:	dy = 1;							break;
		case SB_PAGEUP:		dy = -1 * si.nPage;				break;
		case SB_PAGEDOWN:	dy = 1 * si.nPage;				break;
		case SB_TOP:		dy = -nPos;						break;
		case SB_BOTTOM:		dy = nMaxLine - nPos;			break;
		case SB_THUMBTRACK:	dy = HIWORD(wParam) - si.nPos;	break;
		default:			dy = 0;							break;

		}// switch

		if( dy == 0 )	return 0;

		int	n = nPos + dy;
		if( n < 0 )				n = 0;
		if( n > nMaxPos )		n = nMaxPos;
		dy = n - nPos;

		//printf( "nPos = %d  n = %d  dy = %d  nMaxLine = %d\n",
		//											nPos, n, dy, nMaxLine );

		if( dy == 0 )	return 0;
		if( n == nPos )	return 0;

		//printf( "nPos = %d  n = %d  dy = %d  nMaxLine = %d\n",
		//											nPos, n, dy, nMaxLine );

		si.nPos = n;
		nPos = n;
		TextLineInfo::SetCurrentLine( n );
		SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
		//ScrollWindow( hWnd, 0, -dy*nLineHeight, NULL, NULL );
		ScrollWindowEx( hWnd, 0, -dy*nLineHeight, NULL, NULL, NULL, NULL,
													SW_INVALIDATE | SW_ERASE );
		UpdateWindow( hWnd );

		}// WM_VSCROLL
		return 0;

	case WM_PAINT:
		{

		PAINTSTRUCT	ps;
		BeginPaint( hWnd, &ps );
		TextLineInfo * pLine = TextLineInfo::GetCurrentLine();
		for( int i=0; i<nMaxDisp; ++i ){

			if( i + si.nPos > nMaxLine )	break;
			if( pLine == NULL )				break;

			//int	len = lstrlen( pLine[i+si.nPos] );
			//TextOut( ps.hdc, 10, i*nLineHeight, pLine[i+si.nPos], len );

			TextOut( ps.hdc, 10, i*nLineHeight,
							pLine->GetTextLine(), pLine->GetTextLength() );
			pLine = pLine->GetNextLine();

		}// for
		EndPaint( hWnd, &ps );

		}// WM_PAINT
		break;

	case WM_KEYDOWN:
		{
		WORD	wScrollNotify = 0xFFFF;

		switch( wParam ){
		case VK_UP:			wScrollNotify = SB_LINEUP;		break;
		case VK_DOWN:		wScrollNotify = SB_LINEDOWN;	break;
		case VK_PRIOR:		wScrollNotify = SB_PAGEUP;		break;
		case VK_NEXT:		wScrollNotify = SB_PAGEDOWN;	break;
		case VK_HOME:		wScrollNotify = SB_TOP;			break;
		case VK_END:		wScrollNotify = SB_BOTTOM;		break;
		}// switch

		if( wScrollNotify != 0xffff )
			SendMessage( hwndMain, WM_VSCROLL, MAKELONG(wScrollNotify,0), 0L );

		}// WM_KEYDOWN
		break;

	case WM_MOUSEWHEEL:
		// WORD GET_KEYSTATE_WPARAM(wParam)
		// short GET_WHEEL_DELTA_WPARAM(wParam)
		// short GET_X_LPARAM(lParam)
		// short GET_Y_LPARAM(lParam)
		{

		WORD	wScrollNotify = SB_LINEUP;
		if( GET_WHEEL_DELTA_WPARAM(wParam) < 0 )
			wScrollNotify = SB_LINEDOWN;
		SendMessage( hwndMain, WM_VSCROLL, MAKELONG(wScrollNotify,0), 0L );

		//printf( "%d\n", GET_WHEEL_DELTA_WPARAM(wParam) );

		}// WM_MOUSEWHEEL
		break;

	case WM_CLOSE:
		{
		// 破棄を拒否できる
		DestroyWindow( hWnd );
		}// WM_CLOSE
		break;

	case WM_DESTROY:
		{
		// 破棄を拒否できない
		PostQuitMessage( 0 );
		}// WM_DESTROY
		break;

	case WM_COMMAND:
		{

		if( lParam != 0 ){
			// lParam : コントロールウインドウのハンドル
			//	HIWORD(wParam) : 通知コード
			//	LOWORD(wParam) : コントロールID
			break;
		}

		if( HIWORD( wParam ) != 0 ){
			// HIWORD( wParam ) : アクセラレータのメッセージ
			break;
		}

		switch( LOWORD( wParam ) ){

		case IDM_OPEN:
			{

			OPENFILENAME	ofn;
			ZeroMemory( &ofn, sizeof(OPENFILENAME) );

			char	FilterString[] =
				"EXE Files" "\0"	"*.exe" "\0"
				"All Files" "\0"	"*.*" "\0"
				"\0";

			char			dir[ MAX_PATH ];
			GetAppDataFolder( dir );

			ExeFilePath[0] = 0;	// これをしないと表示されないことあり

			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.hInstance = hInstance;
			ofn.lpstrFilter = FilterString;
			ofn.lpstrFile = (char*)ExeFilePath;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
							OFN_LONGNAMES | OFN_EXPLORER | OFN_HIDEREADONLY ;
			ofn.lpstrTitle = AppName;
			ofn.lpstrInitialDir = dir;
			if( ! GetOpenFileName( &ofn ) )		break;

			{

			HANDLE	hFile = CreateFile( (char*)ExeFilePath, GENERIC_READ,
					NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if( hFile == INVALID_HANDLE_VALUE ){
				//MessageBox( hWnd, "Open Error", AppName, MB_OK );
				Print( "Open Error" );
				break;
			}

			DWORD	bytesRead;
			BOOL	fOK;
			fOK = ReadFile( hFile,
								ReadBuffer, ReadBufferSize, &bytesRead, NULL );
			CloseHandle( hFile );
			if( ! fOK ){
				//MessageBox( hWnd, "Read Error", AppName, MB_OK );
				Print( "Read Error" );
				break;
			}

			fRead = 1;	// 読み込み成功

			int	rslt = CheckExe();
			if( rslt == 0 )		Print( "done." );
			else{
				char	buf[100];
				wsprintf( buf, "error : %d", rslt );
				Print( buf );
			}
			Print( "" );
			InvalidateRect( hWnd ,NULL, TRUE );
			UpdateWindow( hWnd );

			}// read file

			}// IDM_OPEN
			break;

		case IDM_EXIT:
			{

			PostQuitMessage( 0 );

			}// IDM_EXIT
			break;

		default:
			DestroyWindow( hWnd );

		}// switch

		}// WM_COMMAND
		break;

	default:

		return DefWindowProc( hWnd, uMsg, wParam, lParam );

	}// switch


	return 0;

}



// end of this file : ChkExe.cpp
