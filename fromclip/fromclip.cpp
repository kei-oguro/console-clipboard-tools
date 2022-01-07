#include <windows.h>
#include <shlobj.h>

#include <stdio.h>
#include <locale.h>

// ん？ 標準出力には何で出すのが便利なんだ？
// u16 で出力して、 mingw console で漢字が化ける。

static void convertToUTF16(char *mbbuf, size_t mbsize, wchar_t **ubuf, size_t *usize, int codepage)
{
    if (codepage == 0)
    {
        static const int cpToTry[] = {
            65001,  // utf8
            932,    // sjis
            CP_ACP, // The system default Windows ANSI code page.
        };
        *usize = 0;
        for (int i = 0; i < sizeof(cpToTry) / sizeof(cpToTry[0]); ++i)
        {
            convertToUTF16(mbbuf, mbsize, ubuf, usize, cpToTry[i]);
            if (*usize != 0)
                return;
        }
    }
    else
    {
        *usize = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, mbbuf, mbsize, 0, 0);
        *ubuf = (wchar_t *)malloc(*usize);
        MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, mbbuf, mbsize, *ubuf, *usize);
    }
}

//@@ 引数で入力文字コードを指定できた方がいいかも。 MultiByteToWideChar() のコードページに 932 や 65001 を与える。

int main()
{
    HGLOBAL hg;
    int codepage = 0; // 将来オプションで設定する。

    setlocale( LC_ALL, "jpn" );
    if ( !OpenClipboard( NULL ) ) {
        printf( "fromclip.exe: can not open clipboard.\n" );
        return 1;
    }
    if ( 0 != (hg = GetClipboardData( CF_UNICODETEXT )) ) {
        wprintf( L"%s", (wchar_t*)GlobalLock(hg) );
        GlobalUnlock(hg);
        CloseClipboard();
    }
    else if (0 != (hg = GetClipboardData(CF_TEXT)))
    {
        void *buf = GlobalLock(hg);
        size_t size = GlobalSize(hg);
        wchar_t *ubuf;
        size_t usize;
        convertToUTF16((char *)buf, size, &ubuf, &usize, codepage);
        if (usize == 0)
        {
            fwrite(buf, 1, GlobalSize(hg) - 1 /*trim null terminater*/, stdout);
        }
        else
        {
            wprintf(L"%s", ubuf);
            free(ubuf);
        }
        GlobalUnlock(hg);
        CloseClipboard();
    }
    else if ( 0 != (hg = GetClipboardData( CF_HDROP )) ) {
        DROPFILES * df = (DROPFILES*)GlobalLock( hg );
        if ( df->fWide ) {
            wchar_t * str = (wchar_t*)((char*)df + df->pFiles);
            while ( *str ) {
                _putws( str );
                str = wcschr( str, 0 ) + 1/* skip terminater */;
            }
        }
        else {
            /*@@convertToUTF16() した方がいい */
            char * str = (char*)df + df->pFiles;
            while ( *str ) {
                puts( str );
                str = strchr( str, 0 ) + 1/* skip terminater */;
            }
        }
        GlobalUnlock(hg);
        CloseClipboard();
    }
    else {
        printf( "fromclip: can not get clipboard data.\n" );
        CloseClipboard();
        return 2;
    }

    return 0;
}
 