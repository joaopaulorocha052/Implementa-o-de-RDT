RDT = ./rdt
BIN = ./bin

all: rdt_2 client server

server: rdt_2
	gcc -I$(RDT) -Wall -o $(BIN)/server server.c $(BIN)/rdt_2.2.o

client: rdt_2
	gcc -I$(RDT) -Wall -o $(BIN)/cliente cliente.c $(BIN)/rdt_2.2.o

rdt_2:
	gcc -Wall -c $(RDT)/rdt_2.2.c -o $(BIN)/rdt_2.2.o

clean:
	rm -r $(BIN)/*
