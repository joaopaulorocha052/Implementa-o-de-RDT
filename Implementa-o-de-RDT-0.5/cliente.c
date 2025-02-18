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
	size_t bytesRead;
	int status;


	if (argc != 3) {
		printf("uso: %s <ip_servidor> <porta_servidor> \n", argv[0]);
		return 0;
	}

	arq = fopen("hina_is_the_best_big.jpg", "rb");
	if (arq == NULL){
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

	p_saddr = &saddr;

	// status = send_file(s, "yapping.txt", p_saddr);

	// if(status < 0) printf("Erro ao enviar o arquivo\n");
	unsigned char *file_chunk = malloc(sizeof(unsigned char) * MAX_CHUNK_SIZE);
	//file_chunk[MAX_CHUNK_SIZE - 2] = '\0';
	// printf("%d\n", sizeof(file_chunk)/sizeof(char)*MAX_MSG_LEN);
	// printf("abacaxi\n");
	// printf("%d %d\n", sizeof(file_chunk), sizeof(unsigned char));
	// fflush(stdout);
	while (!feof(arq)) {
		bytesRead = fread(file_chunk, sizeof(unsigned char), MAX_CHUNK_SIZE, arq);
		printf("%d\n", bytesRead);

		rdt_send(s, file_chunk, bytesRead, p_saddr);	
	}

	printf("Terminei\n");

	// if(op == 0){
	// 	printf("ERRO OU EOF");
	// }
	

	// _print_pkt();


	return 0;
}