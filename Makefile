#CC  = gcc -Wall -O3
CC  = gcc -Wall -g
#CC  = gcc -DNDEBUG
PROG = play
OBJ = $(PROG).o board.y.o connect.o genmove.o interact.o move.o myrandom.o ui.o

$(PROG): $(OBJ)
	$(CC) -o  $(PROG) $(OBJ)

$(PROG).o: $(PROG).c board.y.h connect.h genmove.h interact.h myrandom.h ui.h
	$(CC) -c  $(PROG).c

board.y.o: board.y.h board.y.c
	$(CC) -c board.y.c

connect.o: board.y.h connect.h connect.c
	$(CC) -c connect.c

genmove.o: board.y.h myrandom.h genmove.h genmove.c
	$(CC) -c genmove.c

interact.o: board.y.h connect.h genmove.h move.h ui.h interact.h interact.c
	$(CC) -c interact.c

move.o: board.y.h connect.h move.h move.c
	$(CC) -c move.c

myrandom.o: board.y.h myrandom.h myrandom.c 
	$(CC) -c myrandom.c

ui.o: board.y.h ui.h ui.c
	$(CC) -c ui.c

clean: 
	rm $(OBJ) $(PROG) 

tst: tst.c
	$(CC) -o $@ $<

estwin: estwin.c
	$(CC) -o $@ $<
