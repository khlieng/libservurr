#include <uv.h>
#include <Judy.h>

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
} file_t;

#define header "HTTP/1.1 200 OK\nContent-Type: text/html; charset=utf-8\n\n"
#define headerCSS "HTTP/1.1 200 OK\nContent-Type: text/css\n\n"
#define headerJSON "HTTP/1.1 200 OK\nContent-Type: application/json; charset=utf-8\n\n"
#define headerJPEG "HTTP/1.1 200 OK\nContent-Type: image/jpeg\n\n"
#define headerPNG "HTTP/1.1 200 OK\nContent-Type: image/png\n\n"
#define headerJS "HTTP/1.1 200 OK\nContent-Type: text/javascript\n\n"
#define headerWOFF "HTTP/1.1 200 OK\nContent-Type: application/font-woff\n\n"
#define headerPlain "HTTP/1.1 200 OK\nContent-Type: text/plain; charset=utf-8\n\n"

#define MAXPENDING 128

static uv_loop_t * loop;
static void * routes = NULL;
static void * files = NULL;

void map_set(void ** map, char * key, void * val) {
	PWord_t v;
	JSLI(v, *map, key);
	*v = (int)val;
}

void * map_get(void * map, char * key) {
	PWord_t v;
	JSLG(v, map, key);
	if (v) {
		return (void *)*v;
	}
	return NULL;
}

void * map_new() {
	return NULL;
}

static uv_buf_t alloc_buffer(uv_handle_t * handle, size_t suggested_size) {
	return uv_buf_init((char *)calloc(1, suggested_size), suggested_size);
}

static void on_close(uv_handle_t * handle) {
	free(handle);
}

static void write_done(uv_write_t * req, int status) {
	write_req_t * wr = (write_req_t *)req;

	uv_close((uv_handle_t *)req->handle, on_close);

	if (wr->buf.base != NULL && wr->free) {
		free(wr->buf.base);
	}
	free(wr);
}

static void read(uv_stream_t * client, ssize_t nread, uv_buf_t buf) {
	write_req_t * req = (write_req_t *)malloc(sizeof(write_req_t));
	http_req_t request;
	char defer = FALSE;

	memset(req, 0, sizeof(write_req_t));
	req->free = TRUE;
	
	if (nread == -1) {
		uv_close((uv_handle_t *)client, NULL);
		return;
	}
	
	request = u_parseHttpRequest(buf.base);

	if (u_equal(request.method, "GET")) {
		file_t * file;

		if (u_equal(request.url, "/")) {
			if (file = (file_t *)map_get(files, "index.html")) {
				req->buf.base = file->data;
				req->buf.len = file->len;
				req->free = FALSE;
			}
			else {
				char * resp = "Hello libservurr!";
				req->buf.base = (char *)malloc(sizeof(char) * strlen(resp));
				req->buf.len = strlen(resp);
				strcpy(req->buf.base, resp);
			}
		}
		else if (file = (file_t *)map_get(files, request.url + 1)) {
			req->buf.base = file->data;
			req->buf.len = file->len;
			req->free = FALSE;
		}
		else {
			vurr_res_t * res = (vurr_res_t *)malloc(sizeof(*res));
			req_cb cb = (req_cb)map_get(routes, request.url);
			memset(res, 0, sizeof(vurr_res_t));
			res->handle = client;
			res->method = request.method;
			res->url = request.url;

			if (cb) {
				cb(res);
				if (!(defer = res->defer)) {
					if (res->data) {
						if (res->len == 0) {
							req->buf.len = strlen(res->data);
							req->buf.base = (char *)malloc(sizeof(char) * req->buf.len);
							strcpy(req->buf.base, res->data);
						}
						else {
							req->buf.len = res->len;
							req->buf.base = res->data;
						}
					}
					else {
						uv_close((uv_handle_t *)client, NULL);
					}
					free(res);
				}
			}
			else {
				req_cb wc_cb = (req_cb)map_get(routes, "*");
				if (wc_cb) {
					wc_cb(res);
					if (res->data) {
						req->buf.base = res->data;
						req->buf.len = res->len;
						req->free = FALSE;
					}
					else {
						uv_close((uv_handle_t *)client, NULL);
					}
				}
				free(res);
			}
		}

		if (!defer) {
			uv_write((uv_write_t *)req, client, &req->buf, 1, write_done);
		}
		else {
			free(req);
		}
	}
	else {
		uv_close((uv_handle_t *)client, NULL);
	}
	
	if (!defer) {
		free(request.method); // Start of buf.base copy
	}
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

static int build_file(file_t * fileinfo, char * head, size_t file_size) {
	fileinfo->len = sizeof(char) * strlen(head) + file_size;
	fileinfo->data = (char *)malloc(fileinfo->len);
	strcpy(fileinfo->data, head);

	return strlen(head);
}

void vurr_static(char * path) {
	uv_tcp_t server;
	struct sockaddr_in bind_addr;
	uv_thread_t spotify_thread;
	uv_thread_t vurr_thread;
	uv_timer_t timer;
	char * file, * filename;
	int file_size, i;
	size_t len;
	list dir_files;

	dir_files = u_ls(path);
	printf("%d files:\n", dir_files.len);
	for (i = 0; i < dir_files.len; i++) {
		file_t * fileinfo = (file_t *)malloc(sizeof(file_t));
		int offset;

		printf("%s\n", dir_files.items[i]);
		
		file = u_readFileS(dir_files.items[i], &file_size);
		filename = u_withoutPath(dir_files.items[i]);
		len = strlen(filename);
		
		if (u_endsWith(filename, ".html")) {
			offset = build_file(fileinfo, header, file_size);
		}
		else if (u_endsWith(filename, ".css")) {
			offset = build_file(fileinfo, headerCSS, file_size);
		}
		else if (u_endsWith(filename, ".js")) {
			offset = build_file(fileinfo, headerJS, file_size);
		}
		else if (u_endsWith(filename, ".png")) {
			offset = build_file(fileinfo, headerPNG, file_size);
		}
		else if (u_endsWith(filename, ".jpg")) {
			offset = build_file(fileinfo, headerJPEG, file_size);
		}
		else if (u_endsWith(filename, ".woff")) {
			offset = build_file(fileinfo, headerWOFF, file_size);
		}
		else {
			offset = build_file(fileinfo, headerPlain, file_size);
		}

		memcpy(fileinfo->data + offset, file, file_size);
		map_set(&files, filename, fileinfo);

		free(file);
	}
}

void vurr_write(vurr_res_t * res) {
	write_req_t * req = (write_req_t *)malloc(sizeof(write_req_t));
	
	req->req.handle = (uv_stream_t *)res->handle;
	req->buf.base = res->data;
	req->buf.len = res->len;
	req->free = TRUE;

	free(res->method);
	free(res);

	uv_write((uv_write_t *)req, req->req.handle, &req->buf, 1, write_done);
}

void vurr_get(char * url, req_cb cb) {
	map_set(&routes, url, cb);
}

void vurr_run(int port) {
	uv_tcp_t server;
	struct sockaddr_in bind_addr;
	
	loop = uv_loop_new();

	uv_tcp_init(loop, &server);
	bind_addr = uv_ip4_addr("0.0.0.0", port);
	uv_tcp_bind(&server, bind_addr);
	uv_listen((uv_stream_t *)&server, MAXPENDING, handle_request);

	uv_run(loop, UV_RUN_DEFAULT);
}