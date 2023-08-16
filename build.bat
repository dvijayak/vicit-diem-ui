taskkill /F /IM vicit-diem.exe

cl ^
    main.cpp vendor/cJSON.c ^
        /std:c++14 /Zi /FS /EHsc /Od /W4 /wd4221 /wd4706 /MP /nologo ^
        /MD ^
        /DUNICODE /D_UNICODE ^
        /D__WXMSW__ /DWXUSINGDLL ^
        /Ivendor/include ^
        /Ivendor/include/cJSON ^
        /Ivendor/lib/win64/vc14x_x64_dll/mswud ^
    /link ^
        /out:vicit-diem.exe ^
        /DEBUG:FULL /nologo /SUBSYSTEM:WINDOWS ^
        /LIBPATH:vendor/lib/win64/vc14x_x64_dll ^
    && vicit-diem.exe
