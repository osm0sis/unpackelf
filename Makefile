CC = gcc
ifeq ($(windir),)
EXE =
RM = rm -f
else
EXE = .exe
RM = del
endif

CFLAGS = -ffunction-sections -O3
LDFLAGS = -Wl,--gc-sections

all: unpackelf$(EXE)

static: unpackelf-static$(EXE)

unpackelf$(EXE):unpackelf.o
	$(CROSS_COMPILE)$(CC) -o $@ $^ $(LDFLAGS) -s

unpackelf-static$(EXE):unpackelf.o
	$(CROSS_COMPILE)$(CC) -o $@ $^ $(LDFLAGS) -static -s

unpackelf.o:unpackelf.c
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< -Werror

clean:
	$(RM) unpackelf unpackelf-static unpackelf.o unpackelf.exe unpackelf-static.exe
	$(RM) Makefile.~

