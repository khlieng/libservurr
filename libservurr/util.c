#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#include "util.h"
#include "map.h"

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

http_req_t parse_http_request(char * req) {
	int len = strstr(req, "\r\n\r\n") - req + 4;
	char * copy = (char *)malloc(sizeof(char) * (len + 1));
	http_req_t request;
	char * lines[64];
	char * line;
	int count = 0, i;
	
	printf("copying http header, length: %d\n", len);
	request.fields = map_new();
	strncpy(copy, req, len);
	puts("splitting into lines");
	lines[0] = strtok(copy, "\r\n");
	while (line = strtok(NULL, "\r\n")) {
		count++;
		lines[count] = line;
	}
	//count++;
	puts("parsing request line");
	request.method = strtok(lines[0], " ");
	request.url = strtok(NULL, " ");
	strtok(request.url, "?");
	request.query = strtok(NULL, "?");

	//printf("Lines: %d, Last field: %s\n", count, lines[count - 1]);

	/*puts("Parsing fields...");
	for (i = 1; i < count; i++) {
		char * key, * value;
		//printf("%d\n", i);
		key = strtok(lines[i], ":");
		value = strtok(NULL, ":");
		while (value[0] == ' ') {
			value++;
		}
		map_set(&request.fields, key, value);
	}
	puts("After parsing fields");*/
	return request;
}

#define HTTP_MAX_HEADER_SIZE 4096
char * build_http_response(http_res_t res, size_t * out_len) {
	char * response;
	int len = 0;
	char * field;

	response = (char *)malloc(sizeof(char) * (HTTP_MAX_HEADER_SIZE + res.data_len));

	if (res.status_code == 404) {
		strcpy(response, HTTP_404_NOT_FOUND);
	}
	else {
		strcpy(response, HTTP_200_OK);
	}

	strcat(response, "Content-Type: ");
	if (field = (char *)map_get(res.fields, "Content-Type")) {
		strcat(response, field);
		strcat(response, "\r\n");
	}
	else {
		strcat(response, "text/plain");
		strcat(response, "\r\n");
	}

	if (field = (char *)map_get(res.fields, "Content-Length")) {
		printf("field: %s\n", field);
		strcat(response, "Content-Length: ");
		strcat(response, field);
		strcat(response, "\r\n");
	}

	strcat(response, "\r\n");
	len = strlen(response);

	if (res.data) {
		if (res.data_len > 0) {
			memcpy(len + response, res.data, res.data_len);
			len += res.data_len;
		}
		else {
			strcat(response, res.data);
			len += strlen(res.data);
		}
	}

	*out_len = len;
	return response;
}

int equal(char * l, char * r) {
	return strcmp(l, r) == 0;
}