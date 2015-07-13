CFLAGS := -std=gnu99 -D_GNU_SOURCE -O2 -Wall -g -Wno-unused-function -Wno-unused-result
CC := gcc-4.9.2
BIN := epoll sha transfer

r: transfer
	./$<

all: $(BIN)

%: %.o lib.h lib.a
	$(CC) -o $@ $(CFLAGS) -Wl,--as-needed $< lib.a -lcrypto

%.o: %.c lib.h config.h
	$(CC) -c -o $@ $(CFLAGS) $<

lib.a: lib-sock.o lib-misc.o
	ar cru $@ $^

.PHONY: .gitignore clean tag tags

tag tags:
	ctags -R *.c *.h

.gitignore:
	echo -e '.*swp\n*.o\n*.a\ntags\nconfig.h' > $@
	for i in $(BIN); do echo $$i >> $@; done
	
clean:
	rm -f lib.a *.o epoll sha transfer
