PREFIX=/usr
LIBUPNP_PREFIX=/usr
#LIBIPTC_PREFIX=/usr

CC=/opt/cross-pi-gcc/bin/arm-linux-gnueabihf-gcc

INCLUDES= -I/opt/cross-pi-libs/include 

LIBS= -lpthread -lupnp -lixml -lthreadutil -L/opt/cross-pi-libs/lib -L/opt/cross-pi-gcc/arm-linux-gnueabihf/include

FILES= main.o gatedevice.o pmlist.o util.o config.o

CFLAGS += -Wall -g -O2

ifdef HAVE_LIBIPTC
#ifdef LIBIPTC_PREFIX
#LIBS += -L$(LIBIPTC_PREFIX)/lib
#INCLUDES += -I$(LIBIPTC_PREFIX)/include
#endif

LIBS += -liptc
INCLUDES += -DHAVE_LIBIPTC
FILES += iptc.o
endif

all:
	echo "run make build-rpi || make all-rpi"

build-rpi: w48upnpd
	echo "Fertig"

all-rpi: w48upnpd
	echo "Fertig"


w48upnpd: $(FILES)
	$(CC) $(CFLAGS) $(FILES) $(LIBS) -o $@
	@echo "make $@ finished on `date`"

%.o:	%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.o w48upnpd *.deb

install: w48upnpd
	install -d /etc/w48upnpd
	install etc/fboxdesc.xml /etc/w48upnpd
	install etc/fboxSCPD.xml  /etc/w48upnpd
	install etc/ligd.gif  /etc/w48upnpd
	install w48upnpd $(PREFIX)/sbin
	install w48upnpd.8 $(PREFIX)/share/man/man8
	if [ ! -f /etc/w48upnpd/w48upnpd.conf ]; then install etc/w48upnpd.conf /etc/w48upnpd/; fi
	install w48upnpd.service /etc/systemd/system
	if [ ! -f /etc/systemd/system/upnpd.service ]; then systemctl  enable w48upnpd.service; fi


