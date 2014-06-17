#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libservurr.h"

#pragma comment(lib, "libservurr.lib")

void lefse(vurr_req_t * req, vurr_res_t * res) {
	res->data = "<h1>Lefse</h1>";
	vurr_res_set(res, "Content-Type", "text/html");
}

void arne(vurr_req_t * req, vurr_res_t * res) {
	res->data = (char *)malloc(5);
	res->len = 5;
	strcpy(res->data, "Arne!");
	
	vurr_res_set(res, "Content-Type", "text/html");
}

void bjarne(vurr_req_t * req, vurr_res_t * res) {
	res->data = (char *)malloc(7);
	res->len = 7;
	res->defer = 1;
	strcpy(res->data, "Bjarne!");

	vurr_write(res);
}

void on_file(vurr_event_t * ev) {
	puts((char *)ev->data);
}

void main(int argc, char * argv[]) {
	vurr_get("/arne", arne);
	vurr_get("/bjarne", bjarne);
	vurr_get("/lefse", lefse);
	
	vurr_static("data");
	
	vurr_on("file", on_file);

	vurr_run(80);
}