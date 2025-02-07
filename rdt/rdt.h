#ifndef RDT_H
#define RDT_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>

#define MAX_MSG_LEN 1000
#define MAX_BUFF_LEN 50
#define MAX_CHUNK_SIZE 50000 // bytes => 50 (size_send) * 1000 (payload size)
#define ERROR -1
#define TRUE 1
#define FALSE 0
#define SUCCESS 0
#define WINDOWSIZE 4 // Janela estática
#define ACKNOLEDGE 3
#define SENT 2
#define AUTHORIZED 1
#define DENIED 0

typedef uint16_t hsize_t;
typedef uint16_t hcsum_t;
typedef uint16_t hseq_t;
typedef uint8_t  htype_t;

#define PKT_ACK 0
#define PKT_DATA 1


struct hdr {
	hseq_t  pkt_seq;
	hsize_t pkt_size;
	htype_t pkt_type;
	hsize_t total_pkts;
	hcsum_t csum;
};

typedef struct hdr hdr;

struct pkt {
	hdr h;
	unsigned char msg[MAX_MSG_LEN];
};
typedef struct pkt pkt;

int send_file(int sock_fd, char * filename, struct sockaddr_in *destination);
unsigned short checksum(unsigned short *, int);
int iscorrupted(pkt *);
int make_pkt(pkt *, htype_t, hseq_t, void *, int);
int has_ackseq(pkt *, hseq_t);
int rdt_send(int, void *, int, struct sockaddr_in *);
int has_dataseqnum(pkt *, hseq_t);
int rdt_recv(int, void *, int, struct sockaddr_in *);
void teste(char *chunk);
void _print_pkt();
int divide_file_chunk(char * chunk);
#endif