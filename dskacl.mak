# Copyright (C) 2012 Roland Kaufmann. All rights reserved. See LICENSE.txt
SOURCES=dskacl.c dskacl.rc xclusive.c xclusive.rc dskacl.mak build.cmd
BINARIES=$(ARCH)\dskacl.exe $(ARCH)\xclusive.com $(ARCH)\xclusive.exe
SCRIPTS=showdsks.vbs launchvm.vbs
EXTRAS=LICENSE.txt README.html

all: dskacl-$(VER)-$(ARCH).cab dskacl-$(VER)-src.cab

clean:
	-for %a in (i386 amd64) do @rd /s /q %a 2>NUL
	-for %a in (i386 amd64 src) do @del /q dskacl-%a.cab 2>NUL

CFLAGS=-D_WIN32_WINNT=0x0501 -DUNICODE
# msvcrt in XP DDK merges .CRT and .rdata
LFLAGS=-machine:$(ARCH) -ignore:4078 -opt:nowin98

$(ARCH)\dskacl.obj: dskacl.c
	@if not exist $(ARCH) mkdir $(ARCH)
	cl -nologo -c $(CFLAGS) -Fo$@ $**

$(ARCH)\dskacl.res: dskacl.rc
	@if not exist $(ARCH) mkdir $(ARCH)
	rc /fo$@ $**

$(ARCH)\dskacl.exe: $(ARCH)\dskacl.obj $(ARCH)\dskacl.res
	link -nologo $(LFLAGS) -out:$@ $**

$(ARCH)\xclusive.obj: xclusive.c
	@if not exist $(ARCH) mkdir $(ARCH)
	cl -nologo -c $(CFLAGS) -Fo$@ $**

$(ARCH)\xclusive.res: xclusive.rc
	@if not exist $(ARCH) mkdir $(ARCH)
	rc /fo$@ $**	

$(ARCH)\xclusive.com: $(ARCH)\xclusive.obj $(ARCH)\xclusive.res
	link -nologo $(LFLAGS) -subsystem:console -entry:wWinMainCRTStartup -out:$@ $** user32.lib

$(ARCH)\xclusive.exe: $(ARCH)\xclusive.obj $(ARCH)\xclusive.res
	link -nologo $(LFLAGS) -subsystem:windows -entry:wWinMainCRTStartup -out:$@ $** user32.lib
	
dskacl-$(VER)-$(ARCH).cab: $(BINARIES) $(SCRIPTS) $(EXTRAS)
	cabarc n $@ $**

dskacl-$(VER)-src.cab: $(SOURCES) $(SCRIPTS) $(EXTRAS) startvm.vbs
	cabarc n $@ $**
