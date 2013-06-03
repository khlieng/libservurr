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

char * readfile(char * filename);
char * readfile_size(char * filename, int * size_out);
int starts_with(char * haystack, char * needle);
list ls(const char * dir);
void freelist(list l);
int ends_with(char * haystack, char * needle);
char * without_path(char * filename);
http_req_t parse_http_request(char * request);
int equal(char * l, char * r);

#endif