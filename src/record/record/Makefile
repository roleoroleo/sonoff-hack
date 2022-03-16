OBJECTS = record.o

all: record

record.o: record.c $(HEADERS)
	$(CC) -c $< -fPIC -Os -o $@

record: $(OBJECTS)
	$(CC) $(OBJECTS) -fPIC -Os -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f record
	rm -f $(OBJECTS)

distclean: clean
