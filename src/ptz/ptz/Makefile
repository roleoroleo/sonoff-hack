OBJECTS-P = ptz_p.o config.o
OBJECTS-H = ptz_h.o config.o
INCLUDE = -I./dummylib
LIB-P = -L./dummylib -lptz
LIB-H = -L./dummylib -lhardware

all: libptz ptz_p ptz_h

ptz_p.o: ptz_p.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -Os -o $@

ptz_h.o: ptz_h.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -Os -o $@

config.o: config.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -Os -o $@

ptz_p: $(OBJECTS-P)
	$(CC) $(OBJECTS-P) $(LIB-P) -fPIC -Os -o $@
	$(STRIP) $@

ptz_h: $(OBJECTS-H)
	$(CC) $(OBJECTS-H) $(LIB-H) -fPIC -Os -o $@
	$(STRIP) $@

libptz:
	make -C dummylib

.PHONY: clean

clean:
	rm -f ptz_p ptz_h
	rm -f $(OBJECTS-P) $(OBJECTS-H)
	make -C dummylib clean

distclean: clean
