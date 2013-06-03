#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#include "util.h"

char * readfile(char * filename) {
    FILE * file;
    long size;
    char * buffer;
    size_t result;

    file = fopen(filename, "rb");

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    buffer = (char *)malloc(sizeof(char) * size + 1);
    result = fread(buffer, 1, size, file);
    fclose (file);

	buffer[size] = '\0';

    return buffer;
}

char * readfile_size(char * filename, int * size_out) {
    FILE * file;
    long size;
    char * buffer;
    size_t result;

    file = fopen(filename, "rb");

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    buffer = (char *)malloc(sizeof(char) * size + 1);
    result = fread(buffer, 1, size, file);
    fclose (file);

	buffer[size] = '\0';
	*size_out = size;

    return buffer;
}

int starts_with(char * haystack, char * needle) {
	return strncmp(haystack, needle, strlen(needle)) == 0;
}

int ends_with(char * haystack, char * needle) {
	char * end = strstr(haystack, needle);
	if (end) {
		return (strlen(haystack) - (end - haystack)) == strlen(needle);
	}
	return 0;
}

#ifdef WIN32
list ls(const char * dir) {
	int c = 0, i = 0;
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;
	list result;
	char path[256];
	memset(&result, 0, sizeof(list));

	sprintf(path, "%s\\*.*", dir);

	if ((hFind = FindFirstFile(path, &fdFile)) == INVALID_HANDLE_VALUE) {
		printf("Path not found: %s\n", dir);
		return result;
	}

	do {
		if (strcmp(fdFile.cFileName, ".") != 0 &&
			strcmp(fdFile.cFileName, "..") != 0) {
			sprintf(path, "%s\\%s", dir, fdFile.cFileName);

			if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				c++;
			}
		}
	} while (FindNextFile(hFind, &fdFile));

	result.len = c;
	result.items = (char **)malloc(sizeof(char *) * result.len);
	sprintf(path, "%s\\*.*", dir);
	hFind = NULL;

	if ((hFind = FindFirstFile(path, &fdFile)) == INVALID_HANDLE_VALUE) {
		printf("Path not found: %s\n", dir);
		return result;
	}

	do {
		if (strcmp(fdFile.cFileName, ".") != 0 &&
			strcmp(fdFile.cFileName, "..") != 0) {
			sprintf(path, "%s\\%s", dir, fdFile.cFileName);

			if (!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				result.items[i] = (char *)malloc(sizeof(char) * strlen(path));
				strcpy(result.items[i], path);
				i++;
			}
		}
	} while (FindNextFile(hFind, &fdFile));

	FindClose(hFind);

	return result;
}
#endif

void freelist(list l) {
	int i;
	for (i = 0; i < l.len; i++) {
		printf("Freeing %d\n", i);
		free(l.items[i]);
	}
	free(l.items);
}

char * without_path(char * filename) {
	char * result = NULL;
	char * last = strchr(filename, '\\');
	while (last != NULL) {
		result = last + 1;
		last = strchr(result, '\\');
	}
	return result;
}

http_req_t parse_http_request(char * head) {
	char * copy = strdup(head);

	http_req_t request;
	request.method = strtok(copy, " ");
	request.url = strtok(NULL, " ");
	strtok(request.url, "?");
	request.query = strtok(NULL, "?");

	return request;
}

int equal(char * l, char * r) {
	return strcmp(l, r) == 0;
}