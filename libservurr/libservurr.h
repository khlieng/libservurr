#ifndef LIBSERVURR_H
#define LIBSERVURR_H

#define header "HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\n\n"
#define headerCSS "HTTP/1.1 200 OK\nContent-Type: text/css\n\n"
#define headerJSON "HTTP/1.1 200 OK\nContent-Type: application/json; charset=utf-8\n\n"
#define headerJPEG "HTTP/1.1 200 OK\nContent-Type: image/jpeg\n\n"
#define headerPNG "HTTP/1.1 200 OK\nContent-Type: image/png\n\n"
#define headerJS "HTTP/1.1 200 OK\nContent-Type: text/javascript\n\n"
#define headerWOFF "HTTP/1.1 200 OK\nContent-Type: application/font-woff\n\n"
#define headerPlain "HTTP/1.1 200 OK\nContent-Type: text/plain; charset=utf-8\n\n"

typedef struct {
	char * data;
	size_t len;
	char defer;
	void * handle;
	char * method;
	char * url;
	char * query;
} vurr_res_t;

typedef struct {
	void * data;
} vurr_event_t;

typedef void (* req_cb)(vurr_res_t *);
typedef void (* ev_cb)(vurr_event_t *);

void * map_get(void * map, char * key);
void map_set(void ** map, char * key, void * val);

void vurr_run(int port);
void vurr_get(char * url, req_cb cb);
void vurr_write(vurr_res_t * res);
void vurr_static(char * path);
void vurr_on(char * e, ev_cb cb);
void vurr_do(char * e, void * data);

typedef struct {
	void (* run)(int);
	void (* get)(char *, req_cb cb);
	void (* write)(vurr_res_t *);
} vurr_api;

static const vurr_api vurr = {
	vurr_run,
	vurr_get,
	vurr_write
};

#endif