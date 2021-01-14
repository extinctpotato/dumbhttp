#define _GNU_SOURCE
#define BUFSIZE 8024
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h> 
#include "http_status_codes.h"

struct cln {
	int cfd;
	struct sockaddr_in caddr;
	char *dir;
};

void* cthread(void* arg) {
	struct cln* c = (struct cln*) arg;

	char header_buf[BUFSIZE] = "";
	char header_safe[BUFSIZE] = "";
	char *d_crlf;
	char *sendstr = "HTTP/1.1 200 OK", *crlf = "\r\n\r\n";
	int rcv = 0, rcvd_pre = 0, rcvd = 0, done = 0, header_safe_len = 0;

	printf("conn: %s\n", inet_ntoa((struct in_addr)c->caddr.sin_addr));

	memset(header_safe, 0, sizeof(header_safe));

	while (!done) {
		printf("reading %i\n", BUFSIZE);
		memset(header_buf, 0, sizeof(header_buf));
		rcv = read(c->cfd, header_buf, BUFSIZE-1);

		if (rcv == 0) {
			break;
		} else if (rcv == -1) {
			break;
		}

		rcvd += rcv;

		d_crlf = strstr(header_buf, crlf);

		if(d_crlf != NULL) {
			printf("detected double crlf\n");
			int crlf_pos = d_crlf - header_buf + 2;
			strncpy(header_safe, header_buf, crlf_pos);
			done = 1;
		}

		printf("check if overflow\n");
		if (rcvd >= BUFSIZE) {
			printf("buffer overflow!\n");
			//while ((read(c->cfd, NULL, BUFSIZE-1)) != 0)
			break;
		}

		printf("received %i, total received: %i\n", rcv, rcvd);
	}
	
	printf("\033[0;32m");

	char *p = header_safe;

	while(*p) {
		switch(*p) {
			case '\r': printf("\\r");break;
			case '\n': printf("\\n\n");break;
			default: putchar(*p);break;
		}
		p++;
	}

	printf("\033[0m");
	//while ((read(c->cfd, NULL, BUFSIZE-1)) != 0);

	int w = 0, to_w = strlen(sendstr);

	while (w < to_w) {
		w += write(c->cfd, sendstr+w, to_w - w);
		printf("written: %i/%i\n", w, to_w);
	}

	close(c->cfd);

	free(c);
	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Provide port and root directory.\n");
		exit(1);
	}

	socklen_t slt;
	int sfd, on = 1;
	struct sockaddr_in saddr;

	pthread_t tid;

	char *dir = argv[2];
	int port = atoi(argv[1]);

	sfd = socket(AF_INET, SOCK_STREAM, 0);

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(port);

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));

	bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr));
	listen(sfd, 5);

	while(1) {
		struct cln* c = malloc(sizeof(struct cln));
		slt = sizeof(c->caddr);
		c->cfd = accept(sfd, (struct sockaddr*)&c->caddr, &slt);
		pthread_create(&tid, NULL, cthread, c);
		pthread_detach(tid);
	}
	close(sfd);

	printf("Done!");
	return EXIT_SUCCESS;
}
