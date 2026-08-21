USE_DEBUG = NO
USE_64BIT = NO
USE_UNICODE = NO
USE_CLANG = YES
# sadly, cygwin mingw does not support gdiplus...
USE_CYGWIN = NO

# the legacy version of qualify.cpp, does not depend upon c++ string class
USE_LEGACY = NO

include der_libs\tool_select.mak

CFLAGS=-Wall -O3 -mwindows -Weffc++ 
CFLAGS += -Wno-write-strings
OBJS=dialog.o dlgres.o console.attr.o regif.o ezfont.o 
BINS=console_attr.exe consattr.exe
HELP=console_attr.chm

SRCS=dialog.cpp console.attr.cpp regif.cpp ezfont.cpp 

all: $(BINS)

clean:
	rm -f $(BINS) *.o

install:
	cp -f $(BINS) *.chm "\Program Files\Misc Binaries"

dist:
	rm -f *.o *.obj *.zip
	zip -D console.palette.chgr.src.zip *
	zip -r console.palette.chgr.src.zip help
	zip -r console.palette.chgr.src.zip palettes
	zip -D console.palette.chgr.zip $(BINS) $(HELP)
	zip -r console.palette.chgr.zip palettes

depend:
	makedepend -I. $(SRCS)

#***********************************************************
console_attr.exe: $(OBJS)
	g++ $(CFLAGS) $(OBJS) -o $@ -lcomctl32 -lhtmlhelp

#  This crashes with mingw, but works with vc6
#  Note: -x works fine, only the Windows update fails
consattr.exe: setconsoleinfo.cpp
	g++ -Wall -O3 -Wno-write-strings -s $< -o $@ -lcomctl32

dialog.o: dialog.cpp 
	g++ $(CFLAGS) -c $<

console.attr.o: console.attr.cpp
	g++ $(CFLAGS) -c $<

regif.o: regif.cpp 
	g++ $(CFLAGS) -c $<

ezfont.o: ezfont.cpp 
	g++ $(CFLAGS) -c $<

statbar.o: statbar.cpp 
	g++ $(CFLAGS) -c $<

dlgres.o: dlgres.rc resource.h
	windres -O COFF $< -o $@

# DO NOT DELETE

dialog.o: resource.h console.attr.h ezfont.h regif.hpp
console.attr.o: console.attr.h
regif.o: regif.hpp
ezfont.o: ezfont.h
