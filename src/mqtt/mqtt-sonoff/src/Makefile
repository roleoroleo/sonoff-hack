-include ../config.mak

TARGET = mqtt-sonoff

LIBS = -L../lib \
            -lcjson \
            -lmosquitto \
            -lsqlite3 \
            -lrt \
            -lpthread

CFLAGS = -Os \
			-Wall \
            -I../include 

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@
	$(STRIP) $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)
