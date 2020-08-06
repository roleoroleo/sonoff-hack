OBJECTS = ptz.o
LIB = -L. -lptz

all: ptz

ptz.o: ptz.c $(HEADERS)
	$(CC) -c $< -fPIC -O2 -o $@

ptz: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB) -fPIC -O2 -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f ptz
	rm -f $(OBJECTS)

distclean: clean
