# Very basic Makefile for Linux (Debian10)
# Needs liballegro5-dev, libcanberra-gtk-module, libcanberra-gtk3-module

# we'll patch this up later, for now I just want to see if it builds...
# I hate to admit it, but I almost want to use cmake on this one...

CC=g++
CFLAGS=-O3 -Wall -Wno-stringop-truncation -g -DALLEGRO_LINUX -DALLEGRO_WITH_XWINDOWS
INC=-I.
LIB=-lallegro -lallegro_main -lallegro_dialog -lallegro_audio

src=CPUs/*.cpp EmulatorSupport/*.cpp Keyboard/*.cpp Memories/*.cpp PSGs/*.cpp Systems/TexasInstruments/*.cpp VDPs/*.cpp

.PHONY: clean

classic99:
	$(CC) $(CFLAGS) $(INC) $(src) $(LIB) -oclassic99v4

clean:
	-rm *.o

