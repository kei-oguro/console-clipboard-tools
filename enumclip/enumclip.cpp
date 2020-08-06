#include <windows.h>

#include <stdio.h>

int main()
{
    if ( OpenClipboard(0))
    {
        int format = 0;
        while ( 0 != (format = EnumClipboardFormats(format)) )
        {
            char name[4096/*@@MAGIC!!*/];
            int namelen = GetClipboardFormatNameA(format, name, sizeof(name)-1 );
            if ( namelen > 0 )
            {
                if ( namelen == sizeof(name)-1 )
                    name[sizeof(name)-1] = 0;
                puts(name);
            }
        }
        CloseClipboard();
    }
    return 0;
}

