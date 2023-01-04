#include <windows.h>
#pragma hdrstop

#include <vector>
#include <string>

struct ClipboardRAII
{
    ClipboardRAII()
    {
        if (!OpenClipboard(NULL))
        {
            throw "toclip.exe: can not open clipboard.";
        }
        if (!EmptyClipboard())
        {
            throw "toclip.exe: can not destroy clipboard and get ownership.";
        }
    }
    ~ClipboardRAII() { CloseClipboard(); }

    ClipboardRAII(const ClipboardRAII &) = delete;
    ClipboardRAII &operator=(const ClipboardRAII &) = delete;
};

struct GlobalMemoryRAII
{
    GlobalMemoryRAII(UINT uFlags, SIZE_T dwBytes) : hg(Allocate(uFlags, dwBytes)) {}
    ~GlobalMemoryRAII() { GlobalFree(hg); }
    void ReAlloc(UINT uFlags, SIZE_T dwBytes) const
    {
        auto newHandle = GlobalReAlloc(hg, dwBytes, uFlags);
        if (newHandle == NULL)
        {
            throw "can not re-allocate global memory";
        }
        hg = newHandle;
    }

    mutable HGLOBAL hg;

    static HGLOBAL Allocate(UINT uFlags, SIZE_T dwBytes)
    {
        auto hg = GlobalAlloc(uFlags, dwBytes);
        if (hg == NULL)
        {
            throw "toclip.exe: can not allocate global memory.";
        }
        return hg;
    }

    GlobalMemoryRAII() = delete;
    GlobalMemoryRAII(const GlobalMemoryRAII &) = delete;
    GlobalMemoryRAII &operator=(const GlobalMemoryRAII &) = delete;
};

template <typename T>
struct GlobalLockerRAII
{
    const HGLOBAL hg;
    T *const address;
    GlobalLockerRAII(const GlobalMemoryRAII &gm) : hg(gm.hg), address(Lock(hg)) {}
    GlobalLockerRAII(const HGLOBAL hGlobal) : hg(hGlobal), address(Lock(hg)) {}
    ~GlobalLockerRAII() { GlobalUnlock(hg); }
    static T *Lock(const HGLOBAL hg)
    {
        const auto address = static_cast<T *>(GlobalLock(hg));
        if (address == NULL)
        {
            throw "can not lock global memory.";
        }
        return address;
    }

    GlobalLockerRAII() = delete;
    GlobalLockerRAII(const GlobalLockerRAII &) = delete;
    GlobalLockerRAII &operator=(const GlobalLockerRAII &) = delete;
};

int main(int argc, char *argv[])
{
    int clipboardFormat = CF_TEXT;
    const GlobalMemoryRAII memory(GMEM_MOVEABLE, 0);

    if (argc < 2)
    {
        size_t size = 1024 * 64;
        std::vector<char> buf(size + 1);
        int pos = 0;
        while (!feof(stdin))
        {
            buf.resize(size + 1 /*null terminator*/);
            int bufRest = size - pos;
            const auto cntRead = fread(buf.data() + pos, sizeof(char), bufRest, stdin);
            pos += cntRead;
            if (bufRest != cntRead)
            {
                break;
            }
            size *= 4; // まだ読みきってないので、バッファを拡張
        }
        buf[pos++] = '\0';

        memory.ReAlloc(GMEM_MOVEABLE, pos);

        {
            GlobalLockerRAII<char> locker(memory);
            char *const clipbuf = locker.address;
            memcpy(clipbuf, buf.data(), pos);
            if (IsTextUnicode(clipbuf, pos, 0))
                clipboardFormat = CF_UNICODETEXT;
            else
            {
                //@@@ MultiByteToWideChar() で utf16 にする。
                // utf8 も CP_UTF8 で utf16 にする。
            }
        }
    }
    else
    { //@@@ これ、区切り文字を改行にするのはいかがなものだろう。
        //@@@ wchar_t 対応。
        std::string lines;
        for (int lpc = 1; argc > lpc; lpc++)
        {
            lines.append(argv[lpc]).append("\n");
        }
        memory.ReAlloc(GMEM_MOVEABLE, lines.size());
        GlobalLockerRAII<char> locker(memory);
        char *const clipbuf = locker.address;
        memcpy(clipbuf, lines.c_str(), lines.size());
    }

    ClipboardRAII clipboard;
    if (!SetClipboardData(clipboardFormat, memory.hg))
    {
        throw "toclip.exe: can not set data to clipboard.";
    }

    return 0;
}
