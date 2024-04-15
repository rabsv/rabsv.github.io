OUT = guible

CSTD   = c99
CLIBS  = -lm -lSDL2 -lSDL2_ttf
CFLAGS = -O2 -std=$(CSTD) -Wall -Wextra -Werror -pedantic -Wno-deprecated-declarations -g \
         -D_POSIX_C_SOURCE=200809L

build: guible.h guible.c main.c
	$(CC) main.c $(CFLAGS) $(CLIBS) -o $(OUT)
