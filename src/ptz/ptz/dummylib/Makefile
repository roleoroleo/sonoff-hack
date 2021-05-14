all:
	$(CC) -fPIC -shared -o libptz.so libptz.c
	$(CC) -fPIC -shared -o libhardware.so libptz.c

clean:
	rm -rf libptz.so
	rm -rf libhardware.so
