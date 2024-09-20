CFLAGS=-O2 -std=c89 -g
LDFLAGS=-lX11
PREFIX=/usr/local

all:	xflappy
xflappy: main.o game.o menu.o util.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f *.o
	rm -f xflappy
	rm -f xflappy.tar.gz

install:
	mkdir -p $(PREFIX)/bin
	cp xflappy $(PREFIX)/bin/xflappy

uninstall:
	rm -f $(PREFIX)/bin/xflappy

# This might depend on GNU tar, not sure whether other tar implementations
# have --exclude
# Using /tmp instead of directly creating the file is necessary because
# otherwise tar issues a warning that "xflappy/" changed while being read.
# (It changed because we created a .tar.gz archive in that directory.)
dist:	clean
	tar -C .. --exclude='*.tar.gz' --exclude='.git*' \
	    -czf /tmp/xflappy.tar.gz xflappy/
	mv /tmp/xflappy.tar.gz .
