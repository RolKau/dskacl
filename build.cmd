@echo off
@rem Copyright (C) 2012 Roland Kaufmann. All rights reserved. See LICENSE.txt
setlocal enableextensions enabledelayedexpansion
if "%BASEDIR%" equ "" set BASEDIR=%ProgramFiles%\WinDDK\3790.1830
rem First build the 32-bit version; this also do any cleaning
set PATH=%BASEDIR%\bin\x86;%SystemRoot%\System32;%ProgramFiles%\Support Tools
set INCLUDE=%BASEDIR%\inc\wxp;%BASEDIR%\inc\crt
set LIB=%BASEDIR%\lib\wxp\i386;%BASEDIR%\lib\crt\i386
rem Extract the version number from the resource file
for /f "tokens=2-4 delims=, " %%a in ('type dskacl.rc ^| %SystemRoot%\System32\find.exe "PRODUCTVERSION"') do @set VER=%%a_%%b_%%c
nmake -nologo -fdskacl.mak "TARGET=wxp" "ARCH=i386" "VER=%VER%" %*
if /i "%1" equ "clean" goto :eof
rem Now build the 64-bit version
set PATH=%BASEDIR%\bin\win64\x86\amd64;%PATH%
set LIB=%BASEDIR%\lib\wnet\amd64;%BASEDIR%\lib\crt\amd64
nmake -nologo -fdskacl.mak "TARGET=wnet" "ARCH=amd64" "VER=%VER%" %*
endlocal
