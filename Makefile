CC  = g++ -Wall -g
#CC  = gcc -Wall -g
PROG = play
OBJ = $(PROG).o board.o connect.o genmove.o interact.o move.o shuff.o ui.o node.o

$(PROG): $(OBJ)
	$(CC) -o  $(PROG) $(OBJ)

$(PROG).o: $(PROG).cpp board.h connect.h genmove.h interact.h shuff.h ui.h
	$(CC) -c  $(PROG).cpp

board.o: board.h board.cpp
	$(CC) -c board.cpp

connect.o: board.h connect.h connect.cpp
	$(CC) -c connect.cpp

genmove.o: board.h shuff.h genmove.h genmove.cpp
	$(CC) -c genmove.cpp

interact.o: board.h connect.h genmove.h move.h ui.h interact.h interact.cpp
	$(CC) -c interact.cpp

move.o: board.h connect.h move.h move.cpp
	$(CC) -c move.cpp

shuff.o: board.h shuff.h shuff.cpp 
	$(CC) -c shuff.cpp

ui.o: board.h ui.h ui.cpp
	$(CC) -c ui.cpp

node.o: board.h node.h node.cpp
	$(CC) -c node.cpp

clean: 
	rm $(OBJ) $(PROG) 

tst: tst.cpp
	$(CC) -o $@ $<

estwin: estwin.cpp
	$(CC) -o $@ $<
