#ifndef LIBSERVURR_H
#define LIBSERVURR_H

#include "map.h"

typedef struct {
	char * method;
	char * url;
	char * query;
	void * fields;
} vurr_req_t;

typedef struct {
	char * data;
	size_t len;
	char defer;
	void * handle;
	void * fields;
	char * head_start;
	vurr_req_t * req;
} vurr_res_t;

typedef struct {
	void * data;
} vurr_event_t;

typedef void (* req_cb)(vurr_req_t *, vurr_res_t *);
typedef void (* ev_cb)(vurr_event_t *);

void vurr_static(char * path);
void vurr_write(vurr_res_t * res);
void vurr_get(char * url, req_cb cb);
void vurr_post(char * url, req_cb cb);
void vurr_put(char * url, req_cb cb);
void vurr_delete(char * url, req_cb cb);
void vurr_on(char * e, ev_cb cb);
void vurr_do(char * e, void * data);
void vurr_res_set(vurr_res_t * res, char * key, char * value);
uv_loop_t * vurr_loop();
void vurr_run(int port);

#endif