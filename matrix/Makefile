PROG:=../matrix.bin
CPP_FILES:=$(wildcard *.c)
OBJ_FILES:=$(patsubst %.c,%.o,$(CPP_FILES))
CPPFLAGS:=-O3
LIBS:=-lncurses

$(PROG): $(OBJ_FILES)
	$(CC) -o $(PROG) $(notdir $(OBJ_FILES)) $(LIBS)

%.o: %.cpp
	$(CC) -c $< $(CPPFLAGS)

clean:
	$(RM) *.o $(PROG)
