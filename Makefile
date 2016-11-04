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

unpackelf$(EXE):unpackelf.o
	$(CROSS_COMPILE)$(CC) -o $@ $^ $(LDFLAGS) -static -s

unpackelf.o:unpackelf.c
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< -Werror

clean:
	$(RM) unpackelf unpackelf.o unpackelf.exe
	$(RM) Makefile.~

