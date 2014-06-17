#include <uv.h>
#include <stdio.h>

#include "libservurr.h"
#include "util.h"

#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Judy.lib")

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
	char free;
} write_req_t;

typedef struct {
	int len;
	char * data;
	char mime_type[256];
} file_t;

typedef struct {
	void * data;
	ev_cb cb;
} vurr_event_data_t;

#define MAXPENDING 128

static uv_loop_t * loop;
static uv_async_t async;
static void * routes = NULL;
static void * files = NULL;
static void * events = NULL;

static uv_buf_t alloc_buffer(uv_handle_t * handle, size_t suggested_size) {
	return uv_buf_init((char *)calloc(1, suggested_size), suggested_size);
}

static void on_close(uv_handle_t * handle) {
	puts("handle closed\n");
	free(handle);
}

static void write_done(uv_write_t * req, int status) {
	write_req_t * wr = (write_req_t *)req;
	puts("write_done");
	uv_close((uv_handle_t *)req->handle, on_close);

	if (wr->buf.base != NULL && wr->free) {
		free(wr->buf.base);
	}
	free(wr);
}

static void read(uv_stream_t * client, ssize_t nread, uv_buf_t buf) {
	write_req_t * req = (write_req_t *)malloc(sizeof(write_req_t));
	char defer = FALSE;
	http_req_t request;
	http_res_t response;
	size_t len;

	memset(req, 0, sizeof(write_req_t));
	memset(&response, 0, sizeof(http_res_t));
	req->free = TRUE;
	
	if (nread == -1) {
		uv_close((uv_handle_t *)client, NULL);
		return;
	}
	
	printf("parsing request:\n\n%s\n\n", buf.base);
	request = parse_http_request(buf.base);

	if (equal(request.method, "GET")) {
		file_t * file;

		if (equal(request.url, "/")) {
			if (file = (file_t *)map_get(files, "index.html")) {
				char len_s[16];
				response.data = file->data;
				response.data_len = file->len;
				itoa(file->len, len_s, 10);

				map_set(&response.fields, "Content-Type", "text/html");
				map_set(&response.fields, "Content-Length", len_s);
				
				req->buf.base = build_http_response(response, &len);
				req->buf.len = len;

				map_free(&response.fields);
			}
			else {
				response.data = "Hello libservurr!";

				req->buf.base = build_http_response(response, &len);
				req->buf.len = len;
			}
		}
		else if (file = (file_t *)map_get(files, request.url + 1)) {
			char len_s[16];
			response.data = file->data;
			response.data_len = file->len;
			itoa(file->len, len_s, 10);

			map_set(&response.fields, "Content-Type", file->mime_type);
			map_set(&response.fields, "Content-Length", len_s);
			
			req->buf.base = build_http_response(response, &len);
			req->buf.len = len;

			map_free(&response.fields);
		}
		else {
			vurr_req_t * vurr_req = (vurr_req_t *)malloc(sizeof(*vurr_req));
			vurr_res_t * res = (vurr_res_t *)malloc(sizeof(*res));
			req_cb cb = (req_cb)map_get(routes, request.url);

			memset(res, 0, sizeof(*res));
			res->handle = client;
			res->req = vurr_req;
			vurr_req->method = request.method;
			vurr_req->url = request.url;
			vurr_req->query = request.query;
			vurr_req->fields = request.fields;

			if (cb) {
				printf("[%s] calling request callback", request.url);
				cb(vurr_req, res);
				if (!(defer = res->defer)) {
					if (res->data) {
						response.data = res->data;
						response.data_len = res->len;
						response.fields = res->fields;
						
						req->buf.base = build_http_response(response, &len);
						req->buf.len = len;

						map_free(&response.fields);
						if (res->len != 0) {
							free(res->data);
						}
					}
					else {
						uv_close((uv_handle_t *)client, NULL);
					}
					free(vurr_req);
					free(res);
				}
			}
			else {
				req_cb wc_cb = (req_cb)map_get(routes, "*");
				if (wc_cb) {
					wc_cb(vurr_req, res);
					if (res->data) {
						response.data = res->data;
						response.data_len = res->len;
						response.fields = res->fields;

						req->buf.base = build_http_response(response, &len);
						req->buf.len = len;
						
						if (res->len != 0) {
							free(res->data);
						}
					}
					else {
						uv_close((uv_handle_t *)client, NULL);
					}
				}
				free(vurr_req);
				free(res);
			}
		}

		if (!defer) {
			uv_write((uv_write_t *)req, client, &req->buf, 1, write_done);
		}
		else {
			puts("deferred response");
			free(req);
		}
	}
	else if (equal(request.method, "POST")) {

	}
	else {
		uv_close((uv_handle_t *)client, NULL);
	}
	
	if (!defer) {
		free(request.method); // Start of buf.base copy
	}		
	map_free(&request.fields);
	free(buf.base);
}

static void handle_request(uv_stream_t * server, int status) {
	uv_tcp_t * client;

	if (status == -1) {
		return;
	}

	client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);
	if (uv_accept(server, (uv_stream_t *)client) == 0) {
		uv_read_start((uv_stream_t *)client, alloc_buffer, read);
	}
	else {
		uv_close((uv_handle_t *)client, NULL);
	}
}

static void on_event(uv_async_t * async, int status) {
	vurr_event_data_t * event_data = (vurr_event_data_t *)async->data;
	vurr_event_t * ev = (vurr_event_t *)malloc(sizeof(*ev));
	ev->data = event_data->data;
	event_data->cb(ev);
	free(event_data);
	free(ev);
}

void vurr_static(char * path) {
	char * file, * filename;
	int file_size, i;
	list dir_files;

	dir_files = ls(path);
	
	for (i = 0; i < dir_files.len; i++) {
		file_t * fileinfo = (file_t *)malloc(sizeof(file_t));
		int offset;

		filename = without_path(dir_files.items[i]);		
		fileinfo->data = readfile_size(dir_files.items[i], &file_size);
		fileinfo->len = file_size;
		
		if (ends_with(filename, ".html")) {
			strcpy(fileinfo->mime_type, "text/html");
		}
		else if (ends_with(filename, ".css")) {
			strcpy(fileinfo->mime_type, "text/css");
		}
		else if (ends_with(filename, ".js")) {
			strcpy(fileinfo->mime_type, "text/javascript");
		}
		else if (ends_with(filename, ".png")) {
			strcpy(fileinfo->mime_type, "image/png");
		}
		else if (ends_with(filename, ".jpg")) {
			strcpy(fileinfo->mime_type, "image/jpeg");
		}
		else if (ends_with(filename, ".woff")) {
			strcpy(fileinfo->mime_type, "application/font-woff");
		}
		else {
			strcpy(fileinfo->mime_type, "text/plain");
		}
		map_set(&files, filename, fileinfo);
	}
}

void vurr_write(vurr_res_t * res) {
	if (res->data) {
		write_req_t * req = (write_req_t *)malloc(sizeof(write_req_t));
		http_res_t response;
		size_t len;
		char len_s[16];
		if (!res) puts("res == NULL");
		itoa(res->len, len_s, 10);
		vurr_res_set(res, "Content-Length", len_s);
		printf("[%s] Content-Length: %s\n", res->req->url, len_s);

		response.data = res->data;
		response.data_len = res->len;
		response.fields = res->fields;
		req->buf.base = build_http_response(response, &len);
		req->buf.len = len;
		req->req.handle = (uv_stream_t *)res->handle;
		req->free = TRUE;

		printf("[%s] vurr_write nullchecks", res->req->url);
		if (!req->req.handle) puts("req.handle == NULL");
		if (!res->req) puts("res->req == NULL");
		if (!res->req->method) puts("res->req->method == NULL");
		if (!res) puts("res == NULL");

		printf("[%s] response size: %d\n", res->req->url, len);
		
		free(res->req->method);
		free(res->req);
		free(res->data);
		free(res);
		map_free(&response.fields);

		puts("calling uv_write");
		
		uv_write((uv_write_t *)req, req->req.handle, &req->buf, 1, write_done);
	}
	else {
		uv_close((uv_handle_t *)res->handle, NULL);
	}
}

void vurr_get(char * url, req_cb cb) {
	map_set(&routes, url, cb);
}

void vurr_post(char * url, req_cb cb) {
	map_set(&routes, url, cb);
}

void vurr_on(char * e, ev_cb cb) {
	map_set(&events, e, cb);
}

void vurr_do(char * e, void * data) {
	ev_cb cb = (ev_cb)map_get(events, e);
	if (cb) {
		vurr_event_data_t * event_data = (vurr_event_data_t *)malloc(sizeof(*event_data));
		event_data->cb = cb;
		event_data->data = data;
		async.data = event_data;
		uv_async_send(&async);
	}
}

void vurr_res_set(vurr_res_t * res, char * key, char * value) {
	map_set(&res->fields, key, value);
}

uv_loop_t * vurr_loop() {
	return loop;
}

void vurr_run(int port) {
	uv_tcp_t server;
	struct sockaddr_in bind_addr;
	
	loop = uv_loop_new();

	uv_tcp_init(loop, &server);
	bind_addr = uv_ip4_addr("0.0.0.0", port);
	uv_tcp_bind(&server, bind_addr);
	uv_listen((uv_stream_t *)&server, MAXPENDING, handle_request);

	uv_async_init(loop, &async, on_event);

	uv_run(loop, UV_RUN_DEFAULT);
}