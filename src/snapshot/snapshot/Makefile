OBJECTS = snapshot.o

all: snapshot

snapshot.o: snapshot.c $(HEADERS)
	$(CC) -c $< -fPIC -O2 -o $@

snapshot: $(OBJECTS)
	$(CC) $(OBJECTS) -fPIC -O2 -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f snapshot
	rm -f $(OBJECTS)

distclean: clean
