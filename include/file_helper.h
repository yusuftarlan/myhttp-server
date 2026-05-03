#ifndef FILE_HELPER_H
#define FILE_HELPER_H
#include "base.h"
#include "http.h"

const char *setMIMEtype(const char *filePath);
int prepfileSize(FILE *file, HttpResponse *httpResponse);
#endif