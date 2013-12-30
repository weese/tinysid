# Directories
#C32_DIR= /usr/local/lib/c32
#C32_DIR= /Volumes/WINDOWS/Programme/Microchip/MPLAB\ C32\ Suite
C32_DIR= /Volumes/BOOTCAMP/Program\ Files\ \(x86\)/Microchip/MPLAB\ C32\ Suite
OBJ_DIR= objs
DEP_DIR= deps

# Compiler Config:
CC= wine ${C32_DIR}/bin/pic32-gcc.exe
CC=pic32-gcc
#CC=pic32mx-gcc-4.5.1
INCLUDES= -I${C32_DIR}/include
CFLAGS= -mprocessor=32MX795F512L -O3 -I${INCLUDES} -MMD
LIBS= 
BIN2HEX= wine ${C32_DIR}/bin/pic32-bin2hex.exe
BIN2HEX=pic32-bin2hex
#BIN2HEX=pic32-bin2hex
# Environment:
export WINEDEBUG:= 

.SECONDEXPANSION:

OBJECTS = cpu.o main.o main_sdl.o mem.o prefs.o prefs_items.o sid.o
OBJECTS = cpu.o main.o main_sdl.o mem.o prefs.o prefs_items.o sid_hw.o delay.o
OBJECTS = test.o clock.o
HEADERS = cpu.h cpu_macros.h cpu_opcodes.h debug.h main.h mem.h prefs.h psid.h sid.h sys.h fixedpointmath.h fixedpointmathcode.h fixedpointmathlut.h

ROBJECTS := $(patsubst ${OBJ_DIR}/%.o,%.o,${OBJECTS})

# Binaries to build 
BINNAME = tinysid

.phony: all
all: ${BINNAME}.hex

$(OBJECTS): $(HEADERS)

.phony: clean
clean:
	rm -f *.o *.elf *.map *.hex

${BINNAME}.elf: ${OBJECTS}
	${CC} ${CFLAGS} ${OBJECTS} ${LIBS} -o${BINNAME}.elf -Map=${BINNAME}.map

${BINNAME}.hex: ${BINNAME}.elf
	${BIN2HEX} $(@:.hex=.elf)
