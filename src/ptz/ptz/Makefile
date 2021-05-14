OBJECTS-B = ptz_b.o config.o
OBJECTS-C = ptz_c.o config.o
INCLUDE = -I./dummylib
LIB-B = -L./dummylib -lptz
LIB-C = -L./dummylib -lhardware

all: libptz ptz_b ptz_c

ptz_b.o: ptz_b.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -O2 -o $@

ptz_c.o: ptz_c.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -O2 -o $@

config.o: config.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -O2 -o $@

ptz_b: $(OBJECTS-B)
	$(CC) $(OBJECTS-B) $(LIB-B) -fPIC -O2 -o $@
	$(STRIP) $@

ptz_c: $(OBJECTS-C)
	$(CC) $(OBJECTS-C) $(LIB-C) -fPIC -O2 -o $@
	$(STRIP) $@

libptz:
	make -C dummylib

.PHONY: clean

clean:
	rm -f ptz_b ptz_c
	rm -f $(OBJECTS-B) $(OBJECTS-C)
	make -C dummylib clean

distclean: clean
