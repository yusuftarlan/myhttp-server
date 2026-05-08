#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* Information level logging macro - prefixes messages with [INFO] */
#define LOG_INFO(fmt, ...) \
    fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)

/* Error level logging macro - prefixes messages with [ERROR] */
#define LOG_ERROR(fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)

/* Debug level logging macro - only enabled when DEBUG is defined at compile time */
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) \
    fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#endif
