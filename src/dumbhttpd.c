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

struct headers {
	char *method;
	char *path;
	char *ver;
	long content_length;
};

void vprint(char* str) {
	char *p = str;

	while(*p) {
		switch(*p) {
			case '\r': printf("\\r");break;
			case '\n': printf("\\n\n");break;
			default: putchar(*p);break;
		}
		p++;
	}
}

void parse_headers(char* hstr, struct headers* h) {
	h->content_length = 0;

	char hstr_local[BUFSIZE] = "";
	char first_line[BUFSIZE] = "";
	char *line = NULL;
	char *saveptr, *saveptr_first_line, *saveptr_line;
	int line_counter = 0;

	strcpy(hstr_local, hstr);

	line = strtok_r(hstr_local, "\r\n", &saveptr);
	
	while (line != NULL)
	{
		char *header_key = NULL, *header_val = NULL;

		if (!line_counter) {
			strcpy(first_line, line);
			h->method = strtok_r(first_line, " ", &saveptr_first_line);
			h->path   = strtok_r(NULL, " ", &saveptr_first_line);
			h->ver    = strtok_r(NULL, " ", &saveptr_first_line);
			line = strtok_r(NULL, "\r\n", &saveptr);
			line_counter++;
			continue;
		}

		printf("parsing: %s\n", line);

		header_key = strtok_r(line, ": ", &saveptr_line);
		header_val = saveptr_line+1;

		if (strcmp(header_key, "Content-Length") == 0) {
			h->content_length = atol(header_val);
		}

		printf("key: %s, val: %s\n", header_key, header_val);

		line = strtok_r(NULL, "\r\n", &saveptr);
		line_counter++;
	}
}

void* cthread(void* arg) {
	struct cln* c = (struct cln*) arg;
	struct headers* h = malloc(sizeof(struct headers));

	char buf[BUFSIZE] = "";
	char headers[BUFSIZE] = "";
	char body[BUFSIZE] = "";
	char *d_crlf;
	char *sendstr = "HTTP/1.1 200 OK", *crlf = "\r\n\r\n";
	int rcv = 0, rcvd_pre = 0, rcvd = 0, done = 0, headers_len = 0;
	int crlf_pos;

	printf("conn: %s\n", inet_ntoa((struct in_addr)c->caddr.sin_addr));

	memset(headers, 0, sizeof(headers));

	while (!done) {
		printf("reading %i\n", BUFSIZE);
		memset(buf, 0, sizeof(buf));
		rcv = read(c->cfd, buf, BUFSIZE-1);

		if (rcv == 0) {
			break;
		} else if (rcv == -1) {
			break;
		}

		rcvd += rcv;

		d_crlf = strstr(buf, crlf);

		if(d_crlf != NULL) {
			printf("detected double crlf\n");
			crlf_pos = d_crlf - buf + 2;
			strncpy(headers, buf, crlf_pos);
			crlf_pos += 2;
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
	vprint(headers);
	printf("\033[0m");

	strcpy(body, buf+crlf_pos);

	parse_headers(headers, h);
	printf("%s, %s, %s, %ld\n", h->method, h->path, h->ver, h->content_length);

	memset(buf, 0, sizeof(buf));
	int content_length = h->content_length - strlen(body);
	rcv = 0;
	rcvd = 0;
	
	while (rcvd != content_length) {
		rcv = read(c->cfd, buf, BUFSIZE-1);
		rcvd += rcv;
	}

	printf("\033[0;31m");
	vprint(body);
	printf("\033[0m\n");

	//while ((read(c->cfd, NULL, BUFSIZE-1)) != 0);

	int w = 0, to_w = strlen(sendstr);

	while (w < to_w) {
		w += write(c->cfd, sendstr+w, to_w - w);
		printf("written: %i/%i\n", w, to_w);
	}

	close(c->cfd);

	free(c);
	free(h);
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
