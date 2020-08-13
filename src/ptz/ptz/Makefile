OBJECTS = ptz.o
INCLUDE = -I./dummylib
LIB = -L./dummylib -lptz

all: libptz ptz

ptz.o: ptz.c $(HEADERS)
	$(CC) $(INCLUDE) -c $< -fPIC -O2 -o $@

ptz: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB) -fPIC -O2 -o $@
	$(STRIP) $@

libptz:
	make -C dummylib

.PHONY: clean

clean:
	rm -f ptz
	rm -f $(OBJECTS)
	make -C dummylib clean

distclean: clean
