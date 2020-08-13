all: mqtt-sonoff
	mv ./src/mqtt-sonoff ./

clean: mqtt-sonoff
	rm -f mqtt-sonoff

install: mqtt-sonoff

mqtt-sonoff:
	$(MAKE) -C src/ $(MAKECMDGOALS)

.PHONY: mqtt-sonoff
