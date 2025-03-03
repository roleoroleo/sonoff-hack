OBJECTS = ptz.o config.o
INCLUDE = -I./minilib
LIB-P = -L./minilib -lm -lptz
LIB-H = -L./minilib -lm -lhardware

all: libptz ptz_p ptz_h

ptz.o: ptz.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -Os -o $@

config.o: config.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -Os -o $@

ptz_p: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB-P) -fPIC -Os -o $@
	$(STRIP) $@

ptz_h: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB-H) -fPIC -Os -o $@
	$(STRIP) $@

libptz:
	$(MAKE) -C minilib

.PHONY: clean

clean:
	rm -f ptz_p ptz_h
	rm -f $(OBJECTS)
	$(MAKE) -C minilib clean

distclean: clean
