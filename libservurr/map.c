#include <Judy.h>

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

void map_free(void ** map) {
	Word_t bytes;
	JSLFA(bytes, *map);
}