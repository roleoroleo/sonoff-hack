OBJECTS = ptz.o config.o
INCLUDE = -I./dummylib
LIB-P = -L./dummylib -lptz
LIB-H = -L./dummylib -lhardware

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
	make -C dummylib

.PHONY: clean

clean:
	rm -f ptz_p ptz_h
	rm -f $(OBJECTS)
	make -C dummylib clean

distclean: clean
