#ifndef LIBSERVURR_H
#define LIBSERVURR_H

typedef struct {
	char * data;
	size_t len;
	char defer;
	void * handle;
	char * method;
	char * url;
} vurr_res_t;

typedef void (* req_cb)(vurr_res_t *);

void * map_get(void * map, char * key);
void map_set(void ** map, char * key, void * val);

void vurr_run(int port);
void vurr_get(char * url, req_cb cb);
void vurr_write(vurr_res_t * res);
void vurr_static(char * path);

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