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
	void * fields;
} http_req_t;

#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_404_NOT_FOUND "HTTP/1.1 404 Not Found\r\n"

typedef struct {
	int status_code;
	void * fields;
	char * data;
	int data_len;
} http_res_t;

char * readfile(char * filename);
char * readfile_size(char * filename, int * size_out);
int starts_with(char * haystack, char * needle);
list ls(const char * dir);
void freelist(list l);
int ends_with(char * haystack, char * needle);
char * without_path(char * filename);
http_req_t parse_http_request(char * request);
char * build_http_response(http_res_t response, size_t * out_len);
int equal(char * l, char * r);

#endif