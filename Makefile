# This makefile is a bit hackish. I wrote it early in the AM. 
# Fohgiveuhness, please!!

ifndef $(CFG)
	CFG=Debug
endif

TARGET=libgcm
PROJECTHOME=$(shell pwd)
LIBDIR=/usr/lib
HEADERDIR=/usr/include

ifeq "$(CFG)" "Debug"
	OUTDIR=Debug
	LIBTARGET=$(TARGET)-dbg.so
	LINKLIB=gcm-dbg
# CFLAGS really should be -Werror...
	CFLAGS=-g -ggdb -gstabs -Wall -c
	LDFLAGS=-shared -fPIC -o $(OUTDIR)/$(LIBTARGET)
endif

ifeq "$(CFG)" "Release"
	OUTDIR=Release
	LIBTARGET=$(TARGET).so
	LINKLIB=gcm
	CFLAGS=-O2 -fomit-frame-pointer -pipe -c
	LDFLAGS=-shared -fPIC -o $(OUTDIR)/$(LIBTARGET)
endif

LIBSRC=source/gcminfo.cpp
TESTSRC=source/gcmbrowser.cpp

LIBOBJ=$(OUTDIR)/gcminfo.o
TESTOBJ=$(OUTDIR)/gcmbrowser.o

CC = gcc
CXX = g++
LD = $(CXX)


$(OUTDIR)/%.o : source/%.cpp
	$(CXX) $(CFLAGS) -o $@ $<

all: lib bin

lib: $(LIBOBJ) $(TESTOBJ)
	$(LD) $(LDFLAGS) $(LIBOBJ)

bin:
	$(LD) -o $(OUTDIR)/gcmbrowser \
		$(LIBOBJ) $(TESTOBJ)


.PHONY: clean
clean:
	rm $(OUTDIR)/*.o
	rm $(OUTDIR)/gcmbrowser
	rm $(OUTDIR)/$(LIBTARGET)

.PHONY: rebuild
rebuild:
	make clean
	make CFG=$(CFG)


.PHONY: install
install:
	$(INSTALL) $(OUTDIR)/$(LIBTARGET) $(LIBDIR)/$(TARGET)
	$(INSTALL) source/gcminfo.h $(HEADERDIR)/libgcm.h
