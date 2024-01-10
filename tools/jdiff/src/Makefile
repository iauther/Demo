#
# Simple Makefile for GCC/Linux
#

.DEFAULT: default

OBJS=JDebug.o JDiff.o JPatcht.o JDefs.o JHashPos.o JMatchTable.o JFileOut.o JFile.o JFileIStream.o \
     JFileAhead.o JFileAheadIStream.o JFileAheadStdio.o JOutAsc.o JOutBin.o JOutRgn.o main.o 

default:	linux
all: 		linux 
full:		clean linux
native:		jdiff
linux:		jdiff 
debug:		jdifd 

CC=gcc
CPP=g++
CFLAGS=$(NATIVE) -m64 -O2 -Wall -static

linux:DBG=-s
debug:DBG=-g -D_DEBUG
native:CFLAGS=$(CFLAGS) -march=native

%.o: %.cpp
	$(CPP) $(CFLAGS) $(DBG) -o $@ -c $<

clean:
	rm -f jdiff jpatch jdifd jptcd *.exe $(OBJS)

jdiff: $(OBJS)
	$(CPP) $(CFLAGS) $(DBG) $(OBJS) -o jdiff

jpatch: jpatch.cpp
	$(CPP) $(CFLAGS) $(DBG) -o jpatch jpatch.cpp

jdifd: $(OBJS)
	$(CPP) $(CFLAGS) $(DBG) $(OBJS) -o jdifd

jpatcd: jpatch.cpp
	$(CPP) $(CFLAGS) $(DBG) -o jptcd jpatch.cpp


