#include <windows.h>
#pragma hdrstop

#include <stdio.h>
#include <assert.h>


int main(int argc, char* argv[])
{
    int clipboardFormat = CF_TEXT;
    HGLOBAL hg;

    if ( argc < 2 ) {
        int size = 1024 * 64;
        char * buf = (char*)malloc( size );
        char * dest = buf;
        int pos = 0;
        while( !feof(stdin) ){
            int cntRead;
            int bufRest = size-pos;
            if ( bufRest != (cntRead=fread( dest, sizeof(char), bufRest, stdin )) ) {
                pos += cntRead;
                break;
            }
            // まだ読みきってないので、バッファを拡張
            buf = (char*)realloc( buf, size * 4 );
            dest = buf + size;
            pos = size;
            size *= 4;
        }
        if ( pos == 0 )
        {
            free(buf);
            return 0;
        }

        if ( FALSE == (hg = GlobalAlloc( GMEM_MOVEABLE, pos )) ) {
            printf( "toclip.exe: can not allocate global memory.\n" );
            free( buf );
            return 1;
        }

        {
            char * clipbuf = (char*)GlobalLock( hg );
            memcpy( clipbuf, buf, pos );
            if ( IsTextUnicode(clipbuf, pos, 0) )
                clipboardFormat = CF_UNICODETEXT;
            GlobalUnlock( clipbuf );
        }

        free( buf );
    }
    else {
        // counts args bytes
        static const char * const lineTerminater = "\r\n";
        static const size_t lineTerminaterSize = strlen(lineTerminater);
        int size = 0;
        for ( int lpc = 1; argc > lpc; lpc++ ) {
            size += strlen(argv[lpc]) + lineTerminaterSize;
        }

        // alloc buff
        if ( FALSE == (hg = GlobalAlloc( GMEM_MOVEABLE, size )) ) {
            printf( "toclip.exe: can not allocate global memory.\n" );
            return 1;
        }

        // copy to clip buff
        char * const clipbuf = (char*)GlobalLock( hg );
        {
            char * str = clipbuf;
            for ( int lpc = 1; argc > lpc; lpc++ ) {
                strcpy( str, argv[lpc] );
                str += strlen( argv[lpc] );
                memcpy( str, lineTerminater, lineTerminaterSize );
                str += lineTerminaterSize;
            }
            assert(size == str-clipbuf);
        }
        GlobalUnlock( clipbuf );
    }

    if ( !OpenClipboard( NULL ) ) {
        printf( "toclip.exe: can not open clipboard.\n" );
        GlobalFree( hg );
        return 2;
    }
    if ( !EmptyClipboard() ) {
        printf( "toclip.exe: can not destory clipboard and get ownership.\n" );
        GlobalFree( hg );
        CloseClipboard();
        return 3;
    }

    if ( !SetClipboardData( clipboardFormat, hg ) ) {
        printf( "toclip.exe: can not set data to clipboard.\n" );
        GlobalFree( hg );
        CloseClipboard();
        return 4;
    }
    CloseClipboard();

    return 0;
}


