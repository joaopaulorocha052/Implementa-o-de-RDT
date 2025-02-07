#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "rdt.h"


int main(int argc, char **argv) {
	FILE * arq;
	size_t op;
	int status;


	if (argc != 3) {
		printf("uso: %s <ip_servidor> <porta_servidor> \n", argv[0]);
		return 0;
	}

	char linha[MAX_MSG_LEN];
	arq = fopen("yapping.txt", "rt");
	if (arq < 0){
        printf("Erro na leitura");
        return ERROR;
    }


	int s;
	struct sockaddr_in saddr;
	struct sockaddr_in *p_saddr;
	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	saddr.sin_port = htons(atoi(argv[2]));
	saddr.sin_family = AF_INET;
	inet_aton(argv[1], &saddr.sin_addr);

	// p_saddr = &saddr;

	// status = send_file(s, "yapping.txt", p_saddr);

	// if(status < 0) printf("Erro ao enviar o arquivo\n");
	char *file_chunk = malloc(MAX_CHUNK_SIZE*sizeof(char));

	while (!feof(arq))
	{
        op = fread(file_chunk, MAX_CHUNK_SIZE, sizeof(unsigned char), arq);

		if(op == 0){
			printf("ERRO OU EOF");
		}
		
		//=> rdt_send(socket, file_chunk, dst);
	}

	_print_pkt();


	return 0;
}