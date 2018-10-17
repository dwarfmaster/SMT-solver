OBJS=objs/main.o objs/theory.o objs/SMT.o
PROG=a.prog
CXXFLAGS=-g
LDFLAGS=
CC=g++
WRAPPER=nix-shell --pure --command

all : $(PROG)

$(PROG) : objs $(OBJS)
	$(WRAPPER) '$(CC) $(CXXFLAGS)    -o $@ $(OBJS) $(LDFLAGS)'

objs/%.o: src/%.cpp
	$(WRAPPER) '$(CC) $(CXXFLAGS) -c -o $@ $<'

objs:
	mkdir objs

clean:
	@touch objs $(PROG)
	rm -r objs
	rm $(PROG)

rec:clean all

.PHONY:all clean rec


