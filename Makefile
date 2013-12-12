CC = gcc
#CC = gcc-mp-4.7
#CFLAGS = -Wall -ggdb -DUSE_SDL $(shell sdl-config --cflags)
CFLAGS = -Wall -O3 -march=native
#LDFLAGS = $(shell sdl-config --libs)

OBJECTS = cpu.o main.o main_sdl.o mem.o prefs.o prefs_items.o sid.o
OBJECTS = cpu.o main.o main_sdl.o mem.o prefs.o prefs_items.o sid_hw.o
HEADERS = cpu.h cpu_macros.h cpu_opcodes.h debug.h main.h mem.h prefs.h psid.h sid.h sys.h fixedpointmath.h fixedpointmathcode.h fixedpointmathlut.h

BINNAME = tinysid

all: $(OBJECTS) $(HEADERS)
	$(CC) -o $(BINNAME) $(OBJECTS) $(LDFLAGS)

$(OBJECTS): $(HEADERS)

clean:
	rm -f $(OBJECTS) $(BINNAME)
