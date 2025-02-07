#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "rdt.h"



int biterror_inject = FALSE;
hseq_t _snd_seqnum = 1;
hseq_t _rcv_seqnum = 1;

struct timeval timeout = {
    .tv_sec = 4,
    .tv_usec = 0,
};
double dev = 0;
double estrtt = 4;
int vezes = 0;
pkt *send_buffer[MAX_BUFF_LEN];
pkt recv_buffer[MAX_BUFF_LEN];
int size_send = 0;
int size_recv = 0;

int send_bufferization(pkt *p) {
    if (size_send < MAX_BUFF_LEN) {    
        send_buffer[size_send] = p;
        size_send += 1;
        return SUCCESS;
    }
    else return ERROR;
}

int debuffer_send() {
    int i;
    for (i = 0; i < size_send; i ++) {
        free(send_buffer[i]);
    }
    return SUCCESS;
}

int divide_file_chunk(char * chunk){
    
    char * op;
    char line[MAX_MSG_LEN];


    for(int i=0; i<strlen(chunk); i++){

        memcpy(line, &chunk[i], sizeof(line));


        pkt *p = malloc(sizeof(pkt));
        if (make_pkt(p, PKT_DATA, size_send, line, strlen(line)) < 0) return ERROR; // => size_send como _seq_num do pacote
        if (send_bufferization(p) < 0) return ERROR;

        i += strlen(line) - 1;

    }


    return SUCCESS;
}



void _print_pkt(){

    for (int i = 0; i < size_send; i++){

        printf("\nPACOTE %d\n", i);
        printf("\n\nSeq: %d, Size: %d, Csum: %d\n\n", send_buffer[i]->h.pkt_seq, send_buffer[i]->h.pkt_size, send_buffer[i]->h.csum);
        printf("\n%s\n\n\n", send_buffer[i]->msg);
        
    }

}


unsigned short checksum(unsigned short *buf, int nbytes){
	register long sum;
	sum = 0;
	while (nbytes > 1) {
		sum += *(buf++);
		nbytes -= 2;
	}
	if (nbytes == 1)
		sum += *(unsigned short *) buf;
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return (unsigned short) ~sum;
}

int iscorrupted(pkt *pr){
	pkt pl = *pr;
	pl.h.csum = 0;
	unsigned short csuml;
	csuml = checksum((void *)&pl, pl.h.pkt_size);
	if (csuml != pr->h.csum){
		return TRUE;
	}
	return FALSE;
}

int make_pkt(pkt *p, htype_t type, hseq_t seqnum, void *msg, int msg_len) {
	if (msg_len > MAX_MSG_LEN) {
		printf("make_pkt: tamanho da msg (%d) maior que limite (%d).\n",
		msg_len, MAX_MSG_LEN);
		return ERROR;
	}
	p->h.pkt_size = sizeof(hdr);
	p->h.csum = 0;
	p->h.pkt_type = type;
	p->h.pkt_seq = seqnum;
	if (msg_len > 0) {
		p->h.pkt_size += msg_len;
		memset(p->msg, 0, MAX_MSG_LEN);
		memcpy(p->msg, msg, msg_len);
	}
	p->h.csum = checksum((unsigned short *)p, p->h.pkt_size);
	return SUCCESS;
}

int has_ackseq(pkt *p, hseq_t seqnum) {
	if (p->h.pkt_type != PKT_ACK || p->h.pkt_seq != seqnum)
		return FALSE;
	return TRUE;
}

void temp_dinamico(double samplertt) {
    double a = 0.125;
    double b = 0.25;
    //printf("sample = %lf\n", samplertt);
    
    estrtt = (1 - a)*estrtt + a*samplertt;
    dev = (1 - b)*dev + b* abs(samplertt - estrtt);

    double sup = (estrtt + 4*dev);
    //printf("sup = %f\n", sup);
    // printf("dev = %f\n", dev);
    // printf("estrtt = %f\n", estrtt);
    double usec;
    int sec = sup;
    
    if (sup < sec){
        usec = (sup + 1) - sec;
    }
    else {
        usec = sup - sec;
    }

    timeout.tv_sec = sec;
    //printf("sec = %d -> ", sec);
    sec = usec * 1000000;
    timeout.tv_usec = sec;
    //printf("usec = %d\n", sec);

}

int send_window(int base, int *auth, int sockfd, struct sockaddr_in *dst) {
    int i, ns;
    for (i = base; i < base + WINDOWSIZE; i ++) {
        if (auth[i] == AUTHORIZED) {
            ns = sendto(sockfd, &send_buffer[i], send_buffer[i]->h.pkt_size, 0,
                        (struct sockaddr *)dst, sizeof(struct sockaddr_in));
            if (ns < 0) {
                perror("rdt_send: sendto(PKT_DATA):");
                return ERROR;
            }
            auth[i] = SENT;
        }
    }
    return SUCCESS;
}

int rdt_send(int sockfd, void *buf, int buf_len, struct sockaddr_in *dst) {
    struct timeval tsa, tsp;
    int i, base = 0;
    if(buf_len < MAX_CHUNK_SIZE){

        printf("BUFFER MENOR QUE CHUNK");

    }
    int *auth = malloc(sizeof(int)*size_send);
    memset(auth, DENIED, sizeof(int)*size_send);
    for (i = 0; i < WINDOWSIZE; i++) auth[i] = AUTHORIZED;
    int control = 1;
    double samplertt = 0;
    pkt ack;
	struct sockaddr_in dst_ack;
	int ns, nr, addrlen;
    while(base <= size_send) {
        gettimeofday(&tsp, 0);
        if (send_window(base, auth, sockfd, dst) < 0) return ERROR;
        waiting:
            if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
                perror("setsockopt(..., SO_RCVTIMEO ,...");
            }

            addrlen = sizeof(struct sockaddr_in);
            nr = recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&dst_ack,
                (socklen_t *)&addrlen);
            gettimeofday(&tsa, 0);
            if (control == 1 && nr > 0 && has_ackseq(&ack, _snd_seqnum)) {
                samplertt = (tsa.tv_usec - tsp.tv_usec);
                //printf("primeiro = %lf", samplertt);
                samplertt = samplertt/(float)1000000;
                //printf("\tsegundo = %lf", samplertt);
                samplertt += (tsa.tv_sec - tsp.tv_sec);
                //printf("\tterceiro = %lf\n", samplertt);
                control = 0;
                // printf("%lf\n", samplertt);
                temp_dinamico(samplertt);
                }
            
            if (nr < 0) {
                perror("rdt_send: recvfrom(PKT_ACK)\n");
                // return ERROR;
                control = 0;
                timeout.tv_sec *= 2;
                timeout.tv_usec *= 2;
                estrtt *= 2;
                continue;
            }
            if (iscorrupted(&ack)){
                printf("rdt_send: iscorrupted\n");
                //control = 0;
                goto waiting;
            }
            if (!has_ackseq(&ack, _snd_seqnum)){
                printf("rdt_send: has_dataseqnum\n");
                //control = 0;
                goto waiting;
            }
    }
        return buf_len;
}

int has_dataseqnum(pkt *p, hseq_t seqnum) {
	if (p->h.pkt_seq != seqnum || p->h.pkt_type != PKT_DATA) {
        printf("Pacote: %d  seqnum: %d\n", p->h.pkt_seq, seqnum);
        fflush(stdout);
		return FALSE;
    }
	return TRUE;
}

int rdt_recv(int sockfd, void *buf, int buf_len, struct sockaddr_in *src) {
	pkt p, ack;
	int nr, ns;
	int addrlen;
	memset(&p, 0, sizeof(hdr));

        if (make_pkt(&ack, PKT_ACK, _rcv_seqnum - 1, NULL, 0) < 0)
                return ERROR;

    rerecv:
        addrlen = sizeof(struct sockaddr_in);
        nr = recvfrom(sockfd, &p, sizeof(pkt), 0, (struct sockaddr*)src,
            (socklen_t *)&addrlen);
        if (nr < 0) {
            perror("recvfrom():");
            return ERROR;
        }
        if (iscorrupted(&p) || !has_dataseqnum(&p, _rcv_seqnum)) {
            printf("rdt_recv: iscorrupted || has_dataseqnum \n");
            // enviar ultimo ACK (_rcv_seqnum - 1)
            ns = sendto(sockfd, &ack, ack.h.pkt_size, 0,
                (struct sockaddr*)src, (socklen_t)sizeof(struct sockaddr_in));
            if (ns < 0) {
                perror("rdt_rcv: sendto(PKT_ACK - 1)");
                return ERROR;
            }
            goto rerecv;
        }
        int msg_size = p.h.pkt_size - sizeof(hdr);
        if (msg_size > buf_len) {
            printf("rdt_rcv(): tamanho insuficiente de buf (%d) para payload (%d).\n", 
                buf_len, msg_size);
            return ERROR;
        }
        memcpy(buf, p.msg, msg_size);
        // enviar ACK

        if (make_pkt(&ack, PKT_ACK, p.h.pkt_seq, NULL, 0) < 0)
                    return ERROR;
        int sl = rand()%5 + 1;
        printf("sleep = %d\n", sl);
        usleep(sl*100000);
        ns = sendto(sockfd, &ack, ack.h.pkt_size, 0,
                    (struct sockaddr*)src, (socklen_t)sizeof(struct sockaddr_in));
        if (ns < 0) {
                    perror("rdt_rcv: sendto(PKT_ACK)");
                    return ERROR;
            }
        _rcv_seqnum++;
        return p.h.pkt_size - sizeof(hdr);
}

int rdt_send_old(int sockfd, void *buf, int buf_len, struct sockaddr_in *dst) {
	// Temporizador estático {
    //  struct timeval timeout;
	//  timeout.tv_sec = 20; // segundos
	//  timeout.tv_usec = 0; // microssegundos
    // }
    struct timeval tsa, tsp;
    int control = 1;
    double samplertt = 0;
    pkt p, ack;
	struct sockaddr_in dst_ack;
	int ns, nr, addrlen;
	if (make_pkt(&p, PKT_DATA, _snd_seqnum, buf, buf_len) < 0)
		return ERROR;
	if (biterror_inject) {
		memset(p.msg, 0, MAX_MSG_LEN);
	}
    // timeout para recebimento
	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("setsockopt(..., SO_RCVTIMEO ,...");
	}
    printf("Pacote =  %d; Timeout = %ld %ld\n", _snd_seqnum, timeout.tv_sec, timeout.tv_usec);

    resend:
        gettimeofday(&tsp, 0);
        ns = sendto(sockfd, &p, p.h.pkt_size, 0,
                (struct sockaddr *)dst, sizeof(struct sockaddr_in));
        if (ns < 0) {
            perror("rdt_send: sendto(PKT_DATA):");
            return ERROR;
        }
    
    waiting:
        if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		    perror("setsockopt(..., SO_RCVTIMEO ,...");
	    }

        addrlen = sizeof(struct sockaddr_in);
        nr = recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&dst_ack,
            (socklen_t *)&addrlen);
        gettimeofday(&tsa, 0);
        if (control == 1 && nr > 0 && has_ackseq(&ack, _snd_seqnum)) {
            samplertt = (tsa.tv_usec - tsp.tv_usec);
            //printf("primeiro = %lf", samplertt);
            samplertt = samplertt/(float)1000000;
            //printf("\tsegundo = %lf", samplertt);
            samplertt += (tsa.tv_sec - tsp.tv_sec);
            //printf("\tterceiro = %lf\n", samplertt);
            control = 0;
            // printf("%lf\n", samplertt);
            temp_dinamico(samplertt);
            }
        
        if (nr < 0) {
            perror("rdt_send: recvfrom(PKT_ACK)\n");
            // return ERROR;
            control = 0;
            timeout.tv_sec *= 2;
            timeout.tv_usec *= 2;
            estrtt *= 2;
            goto resend;
        }
        if (iscorrupted(&ack)){
            printf("rdt_send: iscorrupted\n");
            //control = 0;
            goto waiting;
        }
        if (!has_ackseq(&ack, _snd_seqnum)){
            printf("rdt_send: has_dataseqnum\n");
            //control = 0;
            goto waiting;
        }
        _snd_seqnum++;
        return buf_len;
}

// int send_file_old(int sock_fd, char * filename, struct sockaddr_in *destination){
    
//     char * op;
//     char line[MAX_MSG_LEN];

//     FILE * arq = fopen(filename, "rt");
    

//     if(!arq){
//         printf("Erro na leitura do arquivo\n");
//         return ERROR;
//     }

//     while(!feof(arq)){

//         op = fgets(line, MAX_MSG_LEN, arq);

//         if(op) {
//             pkt p = malloc(sizeof(pkt));
//             if (make_pkt(&p, PKT_DATA, _snd_seqnum, op, sizeof(op)) < 0) return ERROR;
//             if (send_bufferization(&p) < 0) return ERROR;
//         }

//     }

//     rdt_send(sock_fd, destination);

//     debuffer_send();

//     return SUCCESS;

    

// }