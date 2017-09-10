# See README for compiler options
SDKTARGETSYSROOT:=/usr/local/oecore-x86_64/sysroots/armv7ahf-vfp-neon-oe-linux-gnueabi
CROSS_COMPILE:=/usr/local/oecore-x86_64/sysroots/x86_64-oesdk-linux/usr/bin/arm-oe-linux-gnueabi/arm-oe-linux-gnueabi-
CC:=$(CROSS_COMPILE)gcc -march=armv7-a -mfloat-abi=hard -mfpu=neon --sysroot=$(SDKTARGETSYSROOT)
LIBRARY_PATH:=/usr/local/oecore-x86_64/sysroots/armv7ahf-vfp-neon-oe-linux-gnueabi/usr/lib
INCLUDE_PATH:=/usr/local/oecore-x86_64/sysroots/armv7ahf-vfp-neon-oe-linux-gnueabi/usr/include
#PC Compile
#CFLAGS= -Ofast  -pthread -g -O0  -D WITH_RTL -D WITH_ALSA -D WITH_SNDFILE -D WITH_UHD `pkg-config --cflags librtlsdr`
#LDLIBS= -lm -pthread  -lasound -lsndfile -luhd `pkg-config --libs librtlsdr`
#ARM Compile
CFLAGS= -Ofast  -pthread -g -O2  -D WITH_UHD -I$(LIBRARY_PATH)
LDLIBS= -lm -pthread -luhd -L$(LIBRARY_PATH)
all:acarsdec
acarsdec:	acarsdec.o acars.o msk.o rtl.o air.o output.o alsa.o soundfile.o uhd.o
	$(CC) acarsdec.o acars.o msk.o rtl.o air.o output.o alsa.o soundfile.o uhd.o -o $@ $(LDLIBS)

acarsserv:	acarsserv.o dbmgn.o
	$(CC) -Ofast acarsserv.o dbmgn.o -o $@ -lsqlite3
acarsdec.o:	acarsdec.c acarsdec.h
acars.o:	acars.c acarsdec.h syndrom.h
msk.o:	msk.c acarsdec.h
output.o:	output.c acarsdec.h
rtl.o:	rtl.c acarsdec.h
uhd.o: uhd.c acarsdec.h
acarsserv.o:	acarsserv.h
dbmgm.o:	acarsserv.h

clean:
	@\rm -f *.o acarsdec acarsserv
