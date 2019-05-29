@ECHO off

if not exist dist64 mkdir dist64
if exist dist64\* del /q dist64\*

cd ..\libndtypes
copy /y Makefile.vc Makefile
copy /y ndtypes.h.in ndtypes.h

cd compat
copy /y Makefile.vc Makefile
cd ..

cd serialize
copy /y Makefile.vc Makefile
cd ..

nmake /nologo clean
nmake /nologo

copy /y libndtypes-0.2.0dev3.lib ..\vcbuild\dist64
copy /y libndtypes-0.2.0dev3.dll ..\vcbuild\dist64
copy /y libndtypes-0.2.0dev3.dll.lib ..\vcbuild\dist64
copy /y libndtypes-0.2.0dev3.dll.exp ..\vcbuild\dist64
copy /y ndtypes.h ..\vcbuild\dist64

cd tests
copy /y Makefile.vc Makefile
nmake /nologo clean
nmake /nologo

copy /y runtest.exe ..\..\vcbuild\dist64
copy /y runtest_shared.exe ..\..\vcbuild\dist64

cd ..\..\vcbuild



