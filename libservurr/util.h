#ifndef UTIL_H
#define UTIL_H

typedef struct {
	int len;
	char ** items;
} list;

typedef struct {
	char * method;
	char * url;
	char * query;
} http_req_t;

char * u_readFile(char * filename);
char * u_readFileS(char * filename, int * size_out);
int u_startsWith(char * haystack, char * needle);
list u_ls(const char * dir);
void u_freelist(list l);
int u_endsWith(char * haystack, char * needle);
char * u_withoutPath(char * filename);
http_req_t u_parseHttpRequest(char * request);
int u_equal(char * l, char * r);

#endif