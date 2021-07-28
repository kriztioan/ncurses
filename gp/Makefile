PROG:=../gp.bin
PLATFORM:=$(shell uname -s)
CPP_FILES:=$(wildcard *.c)
OBJ_FILES:=$(patsubst %.c,%.o,$(CPP_FILES))
CPPFLAGS:=-w -O3
LIBS:=-lncursesw

ifeq ($(PLATFORM),Darwin)
	CPPFLAGS+=-D_XOPEN_SOURCE_EXTENDED
	LIBS:=-lncurses
endif

$(PROG): $(OBJ_FILES)
	$(CC) -o $(PROG) $(notdir $(OBJ_FILES)) $(LIBS)

%.o: %.cpp
	$(CC) -c $< $(CPPFLAGS)

clean:
	$(RM) *.o $(PROG)
