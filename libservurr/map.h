#ifndef MAP_H
#define MAP_H

void map_set(void ** map, char * key, void * val);
void * map_get(void * map, char * key);
void * map_new();
void map_free(void ** map);

#endif