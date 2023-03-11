OBJECTS = ipc_cmd.o udp.o

all: ipc_cmd

ipc_cmd.o: ipc_cmd.c $(HEADERS)
	$(CC) -c $< -fPIC -Os -o $@

udp.o: udp.c $(HEADERS)
	$(CC) -c $< -fPIC -Os -o $@

ipc_cmd: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -fPIC -Os -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f ipc_cmd
	rm -f $(OBJECTS)
