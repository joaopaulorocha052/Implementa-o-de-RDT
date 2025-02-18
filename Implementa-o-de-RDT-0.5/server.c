#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>  // para close()

#include "rdt.h"

#define BUF_SIZE 1024

int main(int argc, char **argv) {
    FILE *escrita;
    escrita = fopen("saida", "wt");
    int written = 0;

    if (argc != 2) {
        printf("uso: %s <porta_servidor>\n", argv[0]);
        return 0;
    }

    int s;
    struct sockaddr_in saddr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    // Criação do socket UDP
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        perror("Falha ao criar o socket");
        return 1;
    }

    // Configuração do endereço do servidor
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[1]));
    saddr.sin_addr.s_addr = INADDR_ANY;  // Escutando em todas as interfaces

    // Associa o socket à porta
    if (bind(s, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
        printf("Oh dear, something went wrong with read()! %s\n", strerror(errno));
        perror("Falha ao fazer bind");
        return 1;
    }

    printf("Servidor esperando mensagens na porta %s...\n", argv[1]);
    int i = 0;
    while (TRUE) {
        // Recebe a mensagem do cliente
        memset(buffer, 0, BUF_SIZE);
        int msg_size = rdt_recv(s, buffer, BUF_SIZE, &client_addr);
        if (msg_size < 0) {
            printf("Erro ao receber dados\n");
            break;  // Continuar tentando receber mensagens
        }

        // Exibe a mensagem recebida
        //printf("Mensagem recebida: %s\n", buffer);
        //printf("%s", linha);
        written += fwrite(buffer, msg_size, sizeof(unsigned char), escrita);
        i ++;
        printf("i = %d\n", i);

        // Aqui você pode processar a mensagem como necessário

        // Enviar uma resposta (se necessário)
        // char resposta[] = "Mensagem recebida com sucesso!";
        // sendto(s, resposta, sizeof(resposta), 0, (struct sockaddr *)&client_addr, client_len);
    }
    fclose(escrita);
    close(s);
    return 0;
}
