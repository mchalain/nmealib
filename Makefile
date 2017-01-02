CC = gcc 
 
TARGET = libnmea
BIN = lib/$(TARGET).a
MODULES = generate generator parse parser tok context time info gmath sentence  
SAMPLES = generate generator parse parse_file math
 
OBJ = $(MODULES:%=build/nmea_gcc/%.o) 
LINKOBJ = $(OBJ) $(RES)

SMPLS = $(SAMPLES:%=samples_%)
SMPLOBJ = $(SAMPLES:%=samples/%/main.o)

INCS = -I include 
LIBS = -lm -Llib -lnmea
 
.PHONY: all all-before all-after clean clean-custom doc debug
 
all: all-before $(BIN) samples all-after 

all-before:
	mkdir -p lib
	mkdir -p build/nmea_gcc

clean: clean-custom 
	rm -f $(LINKOBJ) $(BIN) $(SMPLOBJ) $(SMPLS)

doc:
	$(MAKE) -C doc
	
remake: clean all

debug: CFLAGS+=-g
debug: all

shared: CFLAGS+=-fPIC
shared: BIN+=lib/$(TARGET).so
shared:all

lib/$(TARGET).so: $(LINKOBJ)
	$(CC) -shared -Wl,-soname=$(TARGET) -o $@ $^ -lm

lib/$(TARGET).a: $(LINKOBJ)
	ar rsc $@ $^
	ranlib $@

build/nmea_gcc/%.o: src/%.c 
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

samples: $(SMPLS)

samples_%: samples/%/main.o
	$(CC) $< $(LIBS) -o build/$@

samples/%/main.o: samples/%/main.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@
