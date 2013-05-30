#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libservurr.h"

#pragma comment(lib, "libservurr.lib")

void arne(vurr_res_t * res) {
	res->data = (char *)malloc(5);
	res->len = 5;
	strcpy(res->data, "Arne!");
}

void bjarne(vurr_res_t * res) {
	res->data = (char *)malloc(7);
	res->len = 7;
	res->defer = 1;
	strcpy(res->data, "Bjarne!");

	vurr_write(res);
}

void main(int argc, char * argv[]) {
	vurr_get("/arne", arne);
	vurr_get("/bjarne", bjarne);
	vurr_run(1881);
}