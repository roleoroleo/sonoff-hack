OBJECTS = record.o

all: record

record.o: record.c $(HEADERS)
	$(CC) -c $< -fPIC -O2 -o $@

record: $(OBJECTS)
	$(CC) $(OBJECTS) -fPIC -O2 -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f record
	rm -f $(OBJECTS)

distclean: clean
