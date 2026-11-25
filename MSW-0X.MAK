
msw-0x.exe: msw-0x.obj msw-grfx.obj alg_v11.obj msw-misc.obj msw-0x.def msw-0x.res msw-0x.mak
    link msw-0x msw-grfx alg_v11 msw-misc, /A:16, NUL, /nod mlibcaw libw, msw-0x.def
    rc msw-0x.res

msw-grfx.obj : msw-grfx.c 0x.h type.h varext.h macro.h msw-0x.mak
    cl -c -GW -Osw -W2  -Zp  -AM -FPa msw-grfx.c

msw-0x.obj : msw-0x.c 0x.h type.h vardef.h macro.h msw-0x.mak
    cl -c -Gw -Osw -W2  -Zp  -AM -FPa msw-0x.c

alg_v11.obj : alg_v11.c 0x.h type.h varext.h macro.h msw-0x.mak
    cl -c -GW -Oxazb2 -W2  -Zp  -AM -FPa alg_v11.c

msw-misc.obj : msw-misc.c 0x.h type.h varext.h macro.h msw-0x.mak
    cl -c -GW -Osw -W2  -Zp  -AM -FPa msw-misc.c

msw-0x.res : msw-0x.rc 0x.h msw-0x.ico msw-0x.dlg msw-0x.mak
    rc -r msw-0x.rc


