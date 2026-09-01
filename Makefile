# makefile for console_attr.exe
# SHELL=cmd.exe
USE_DEBUG = NO
USE_64BIT = NO
USE_UNICODE = NO
USE_CLANG = YES

include der_libs\tool_select.mak 

ifeq ($(USE_DEBUG),YES)
CFLAGS = -Wall -g -c
LFLAGS = -g -mwindows
else
CFLAGS = -Wall -O3 -c
LFLAGS = -s -mwindows
endif
CFLAGS += -Weffc++
CFLAGS += -Wno-write-strings

ifeq ($(USE_UNICODE),YES)
CFLAGS += -DUNICODE -D_UNICODE
LiFLAGS += -dUNICODE -d_UNICODE
LFLAGS += -dUNICODE -d_UNICODE
endif

ifeq ($(USE_CLANG),YES)
CFLAGS += -DUSING_CLANG
endif
LiFLAGS += -Ider_libs
CFLAGS += -Ider_libs
IFLAGS += -Ider_libs

# This is required for *some* versions of makedepend
IFLAGS += -DNOMAKEDEPEND

ifeq ($(USE_STATIC),YES)
LFLAGS += -static
endif

CPPSRC=dialog.cpp console.attr.cpp config.cpp ClearIcon.cpp about.cpp \
der_libs/common_funcs.cpp \
der_libs/common_win.cpp \
der_libs/claude_browsers.cpp \
der_libs/hyperlinks.cpp 
# der_libs/qualify.cpp 

OBJS = $(CPPSRC:.cpp=.o) dlgres.o

LIBS=-lcomdlg32 -lole32 -lgdi32 -lhtmlhelp -luuid

DBASE := console.palette.chgr
BASE = console_attr
BINS = $(BASE).exe

# Automatically parse the latest version block
VERSION := $(shell grep -oE '\[[0-9]+\.[0-9]+\]' CHANGELOG.md | head -n 1 | tr -d '[]')
DIST_ZIP := $(DBASE)_V$(VERSION).zip

# Force these action-only targets to always run
.PHONY: dist release update

#**************************************************************************
%.o: %.cpp
	$(TOOLS)/$(GNAME) $(CFLAGS) $< -o $@

all: $(BINS)

clean:
	rm -f $(OBJS) $(BINS) *~ *.zip

dist:
	rm -f *.zip
	zip $(DIST_ZIP) *.exe console_attr.chm Readme.md LICENSE.txt palettes CHANGELOG.md
	zip -r $(DIST_ZIP) palettes\*

# Your new automated release workflow
release: dist
	@cmd /C "@echo Preparing GitHub release for v$(VERSION)..."
	sed -n '/## \['$(VERSION)'\]/,/## \[/p' CHANGELOG.md | sed '$$d' > temp_notes.md
	gh release create v$(VERSION) ./$(DIST_ZIP) ./CHANGELOG.md --notes-file temp_notes.md
	rm temp_notes.md
	@cmd /C "@echo Release v$(VERSION) successfully uploaded to GitHub!"
	
# Your corrected, bulletproof update-in-place pipeline
update: dist
	@cmd /C "@echo Updating assets for existing release v$(VERSION)..."
	gh release upload v$(VERSION) ./$(DIST_ZIP) ./CHANGELOG.md --clobber
	@cmd /C "@echo Release v$(VERSION) assets successfully updated on GitHub!"

wc:
	wc -l $(CPPSRC)

clint:
	cmd /C "python ..\ClaudeLint.py --exclude der_libs"
	
cppc:
	cmd /C "cppcheck --project=compile_commands.json --check-level=exhaustive --enable=all --std=c++14 --suppressions-list=./.suppress.cppcheck"

check:
	cmd /C "d:\llvm\bin\clang-tidy.exe $(CPPSRC)"

depend: 
	makedepend $(IFLAGS) $(CPPSRC)

#**************************************************************************
$(BINS): $(OBJS)
	$(TOOLS)/$(GNAME) $(OBJS) $(LFLAGS) -o $(BINS) $(LIBS) 

dlgres.o: dlgres.rc resource.h
	$(TOOLS)\$(WRNAME) $< -O coff -o $@

# DO NOT DELETE

dialog.o: resource.h der_libs/common.h der_libs/commonw.h console.attr.h
dialog.o: der_libs/claude_browsers.h version.h config.h
console.attr.o: der_libs/common.h console.attr.h
config.o: der_libs/common.h console.attr.h config.h
ClearIcon.o: der_libs/common.h der_libs/commonw.h console.attr.h config.h
about.o: resource.h version.h der_libs/hyperlinks.h
der_libs/common_funcs.o: der_libs/common.h
der_libs/common_win.o: der_libs/common.h der_libs/commonw.h
der_libs/claude_browsers.o: der_libs/common.h der_libs/commonw.h
der_libs/claude_browsers.o: der_libs/claude_browsers.h
der_libs/hyperlinks.o: der_libs/iface_32_64.h der_libs/hyperlinks.h
