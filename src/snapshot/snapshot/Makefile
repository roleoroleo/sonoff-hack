OBJECTS_1 = snapshot.o
OBJECTS_2 = add_water.o convert2jpg.o imggrabber.o water_mark.o
FFMPEG = ffmpeg-4.0.4
FFMPEG_DIR = ./$(FFMPEG)
INC_FF = -I$(FFMPEG_DIR)
LIB_FF = $(FFMPEG_DIR)/libavcodec/libavcodec.a $(FFMPEG_DIR)/libavutil/libavutil.a -lpthread -lm
JPEGLIB = jpeg-9c
JPEGLIB_DIR = ./$(JPEGLIB)
INC_J = -I$(JPEGLIB_DIR)
LIB_J = $(JPEGLIB_DIR)/.libs/libjpeg.a
OPTS = -Os -ffunction-sections -fdata-sections

all: snapshot libs imggrabber

%.o : %.c $(HEADERS)
	$(CC) -c $< $(OPTS) $(INC_J) $(INC_FF) -fPIC -o $@

snapshot: $(OBJECTS_1)
	$(CC) -Wl,--gc-sections $(OBJECTS_1) -fPIC -Os -o $@
	$(STRIP) $@

libs:
	@$(build_ffmpeg)
	@$(build_jpeglib)

imggrabber: $(OBJECTS_2)
	$(CC) -Os -Wl,--gc-sections $(OBJECTS_2) $(LIB_J) $(LIB_FF) -fPIC -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f snapshot
	rm -f imggrabber
	rm -f $(OBJECTS_1) $(OBJECTS_2)
	rm -rf $(FFMPEG)

distclean: clean
	rm -rf SDK
	rm -rf jpeg-9c

define build_ffmpeg
    # get archive
    if [ ! -f SDK/ffmpeg.tar.bz2 ]; then \
        mkdir -p SDK; \
        wget -O ./SDK/ffmpeg.tar.bz2.tmp "http://ffmpeg.org/releases/$(FFMPEG).tar.bz2"; \
        mv ./SDK/ffmpeg.tar.bz2.tmp ./SDK/ffmpeg.tar.bz2; \
    fi

    # untar
    if [ ! -f $(FFMPEG)/README.md ]; then \
         tar jxvf ./SDK/ffmpeg.tar.bz2; \
    fi

    # build
    if [ ! -f $(FFMPEG)/libavcodec/libavcodec.a ] || [ ! -f $(FFMPEG)/libavutil/libavutil.a ]; then \
         cd $(FFMPEG); \
        ./configure --enable-cross-compile --cross-prefix=$(CROSSPREFIX) --arch=armel --target-os=linux --prefix=$(CROSSPATH) --enable-small --disable-ffplay --disable-ffprobe --disable-doc  --disable-decoders --enable-decoder=h264 --disable-encoders --disable-demuxers --enable-demuxer=h264 --disable-muxers --disable-protocols --disable-parsers --enable-parser=h264 --disable-filters --disable-bsfs --disable-indevs --disable-outdevs --extra-cflags="-Os -ffunction-sections -fdata-sections" && \
         make; \
         cd ..;\
    fi
endef

define build_jpeglib

    # get archive
    if [ ! -f SDK/jpeg.tar.gz ]; then \
        mkdir -p SDK; \
        wget -O ./SDK/jpeg.tar.gz.tmp "https://www.ijg.org/files/jpegsrc.v9c.tar.gz"; \
        mv ./SDK/jpeg.tar.gz.tmp ./SDK/jpeg.tar.gz; \
    fi

    # untar
    if [ ! -f $(JPEGLIB)/README ]; then \
         tar zxvf ./SDK/jpeg.tar.gz; \
    fi

    # build
    if [ ! -f $(JPEGLIB)/.libs/libjpeg.a ]; then \
        cd $(JPEGLIB); \
        ./configure --host=$(CROSS) --enable-shared=yes && \
        make CFLAGS+="-Os -ffunction-sections -fdata-sections"; \
        cd ..;\
    fi
endef
