
# CFLAGS=-DSILENT_VALGRIND
CFLAGS = -Wall

OBJECTS = 
OBJECTS += main.o
OBJECTS += config.o
OBJECTS += error.o
OBJECTS += discriminant.o
OBJECTS += pathdb.o
OBJECTS += ionice.o
OBJECTS += digest.o
OBJECTS += options.o
OBJECTS += state.o
OBJECTS += htable.o
OBJECTS += memory.o
OBJECTS += string.o

CFLAGS = -g
SSL_LDFLAGS = -L/usr/lib/x86_64-linux-gnu -lssl -lcrypto
LDFLAGS = $(SSL_LDFLAGS)

all: filededup

clean:
	-/bin/rm -f $(OBJECTS) filededup test-htable test-htable.o

filededup: $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(LDFLAGS)

tests: test-htable

test-htable: htable.o test-htable.o memory.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

help.ci: help.txt
	sed -e 's,",\\",g' -e 's,.*,"&\\n",' help.txt >help.ci

main.o: main.c help.ci
	$(CC) $(CFLAGS) -c -o $@ main.c

