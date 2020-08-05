#include <windows.h>
#include <shlobj.h>

#include <stdio.h>
#include <locale.h>



int main(int argc, char* argv[])
{
    HGLOBAL hg;

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
    else if ( 0 != (hg = GetClipboardData( CF_TEXT )) ) {
        fwrite( GlobalLock(hg), 1, GlobalSize(hg) - 1/*trim null terminater*/, stdout );
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
 