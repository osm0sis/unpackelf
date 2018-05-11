ifeq ($(CC),cc)
CC = gcc
endif
ifeq ($(windir),)
EXE =
RM = rm -f
else
EXE = .exe
RM = del
endif

CFLAGS = -ffunction-sections -O3

ifneq (,$(findstring darwin,$(CROSS_COMPILE)))
    UNAME_S := Darwin
else
    UNAME_S := $(shell uname -s)
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -Wl,-dead_strip
else
    LDFLAGS += -Wl,--gc-sections -s
endif

all: unpackelf$(EXE)

static:
	$(MAKE) LDFLAGS="$(LDFLAGS) -static"

unpackelf$(EXE):unpackelf.o
	$(CROSS_COMPILE)$(CC) -o $@ $^ $(LDFLAGS)

unpackelf.o:unpackelf.c
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< -Werror

clean:
	$(RM) unpackelf
	$(RM) *.~ *.exe *.o

