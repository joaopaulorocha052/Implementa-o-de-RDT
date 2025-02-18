RDT = ./rdt
BIN = ./bin

all: rdt_2 client server

server: rdt_2
	gcc -I$(RDT) -g -Wall -o $(BIN)/server server.c $(BIN)/rdt_2.2.o

client: rdt_2
	gcc -I$(RDT) -g -Wall -o $(BIN)/cliente cliente.c $(BIN)/rdt_2.2.o

rdt_2:
	mkdir -p $(BIN)
	gcc -g -c $(RDT)/rdt_2.2.c -o $(BIN)/rdt_2.2.o

clean:
	rm -r $(BIN)/*

copy:
	cp /home/thiago/Documentos/Prog/Redes/hina_is_the_best_big.jpg ./bin
	cp -r /home/thiago/Documentos/Prog/Redes/Implementa-o-de-RDT-0.5/bin /tmp/pycore.1
