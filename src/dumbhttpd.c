#define _GNU_SOURCE
#define BUFSIZE  8024
#define H_GET	 193456677
#define H_POST	 6384404715
#define H_PUT	 193467006
#define H_HEAD	 6384105719
#define H_DELETE 6952134985656
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
#include "misc.h"

struct cln {
	int cfd;
	struct sockaddr_in caddr;
	char *dir;
};

struct headers {
	char *method;
	char *path;
	char *ver;
	char *connection;
	char *content_type;
	char *server;
	long content_length;
};

void make_headers(int s, struct headers* h, char* resp, int resp_size) {
	char *cursor = resp;
	snprintf(cursor, resp_size, "HTTP/1.1 %i %s\r\n", s, HS_reasonPhrase(s));
	cursor += strlen(cursor);
	snprintf(cursor, resp_size, "Connection: Close\r\n");
	cursor += strlen(cursor);
	snprintf(cursor, resp_size, "Server: dumbhttp\r\n");
	cursor += strlen(cursor);
	snprintf(cursor, resp_size, "Content-Type: text/html; charset=UTF-8\r\n");
	cursor += strlen(cursor);

	if (h->content_length != 0) {
		snprintf(cursor, resp_size, "Content-Length: %li\r\n", h->content_length);
		cursor += strlen(cursor);
	}

	snprintf(cursor, resp_size, "\r\n");
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

		// Key has no colon, while val is incremented
		// to get rid of a spurious space.
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
	char sendstr[BUFSIZE] = "";
	char *d_crlf;
	char *crlf = "\r\n\r\n";
	int rcv = 0, rcvd = 0, done = 0;
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
			// Find the position but fast-forward by two
			// in order to catch the last CR LF as well.
			crlf_pos = d_crlf - buf + 2;
			strncpy(headers, buf, crlf_pos);
			crlf_pos += 2;
			done = 1;
		}

		printf("check if overflow\n");
		if (rcvd >= BUFSIZE) {
			printf("buffer overflow!\n");
			break;
		}

		printf("received %i, total received: %i\n", rcv, rcvd);
	}
	
	printf("\033[0;32m");
	vprint(headers);
	printf("\033[0m");

	strcpy(body, buf+crlf_pos);

	parse_headers(headers, h);
	unsigned long method = hash(h->method);
	printf("%s, %s, %s, %ld, %li\n", h->method, h->path, h->ver, h->content_length, method);

	// We might have already received some body in the previous calls,
	// so let's receive whatever that remains in the TCP buffer.
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

	// Content-Length might be >0 because client might have set it.
	// Reset it back to zero as it might be reused for GET.
	h->content_length = 0;

	int response_code = 200;
	int del;
	char path[100] = "";
	char data[BUFSIZE] = "";
	FILE *sf, *rf;

	if (strcmp(h->path, "/") == 0) h->path = "/index.html";
	snprintf(path, sizeof(path), "%s%s", c->dir, h->path);
	printf("path: %s\n", path);
	sf = fopen(path, "r");

	switch(method) {
		case H_HEAD:
		case H_GET:
			if (sf == NULL) {
				response_code = 404;
			} else {
				fseek(sf, 0, SEEK_END);
				h->content_length = ftell(sf);
				fseek(sf, 0, SEEK_SET);
			}
			break;
		case H_POST:
			// Make the POST operation idempotent.
			if (access(path, F_OK) == 0) {
				response_code = 409;
				break;
			}
		case H_PUT:
			rf = fopen(path, "w");
			fputs(body, rf);
			fclose(rf);
			break;
		case H_DELETE:
			del = remove(path);
			response_code = 204;
			if (del == -1) response_code = 404;
			break;
		default:
			response_code = 500;

	}

	make_headers(response_code, h, sendstr, sizeof(sendstr));

	int w = 0, to_w = strlen(sendstr);

	while (w < to_w) {
		w += write(c->cfd, sendstr+w, to_w - w);
		printf("written headers: %i/%i\n", w, to_w);
	}

	w = 0; to_w = 0;
	int b_w_total = 0;

	if (h->content_length > 0 && method == H_GET) {
		while (fgets(data, BUFSIZE, sf) != NULL) {
			to_w = strlen(data);
			while (w < to_w) {
				w += write(c->cfd, data+w, to_w - w);
				b_w_total += w;
			}
#ifdef DEBUG
			printf("writing body: %i/%li\n", b_w_total, h->content_length);
#endif
			w = 0, to_w = 0;
		}
		bzero(data, BUFSIZE);
		fclose(sf);
	}

	printf("written body: %i/%li\n", b_w_total, h->content_length);

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

	printf("hashes: %i %li %i %li %li\n", H_GET, H_POST, H_PUT, H_HEAD, H_DELETE);

	while(1) {
		struct cln* c = malloc(sizeof(struct cln));
		slt = sizeof(c->caddr);
		c->cfd = accept(sfd, (struct sockaddr*)&c->caddr, &slt);
		c->dir = dir;
		pthread_create(&tid, NULL, cthread, c);
		pthread_detach(tid);
	}
	close(sfd);

	printf("Done!");
	return EXIT_SUCCESS;
}
