#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define LOGOUT stdout

#define infof(fmt, ...)                      \
	do {                                     \
		fprintf(LOGOUT, fmt, ##__VA_ARGS__); \
		fflush(LOGOUT);                      \
	} while (0)

#define errorf(fmt, ...)                     \
	do {                                     \
		fprintf(LOGOUT, fmt, ##__VA_ARGS__); \
		fflush(LOGOUT);                      \
	} while (0)

#define exitf(fmt, ...)             \
	do {                            \
		errorf(fmt, ##__VA_ARGS__); \
		exit(EXIT_FAILURE);         \
	} while (0)

#define fatalf(fmt, ...)            \
	do {                            \
		errorf(fmt, ##__VA_ARGS__); \
		abort();                    \
	} while (0)

#ifdef DEBUG
#define debugf(fmt, ...)                     \
	do {                                     \
		fprintf(LOGOUT, fmt, ##__VA_ARGS__); \
		fflush(LOGOUT);                      \
	} while (0)
#else
#define debugf(fmt, ...) \
	do {                 \
	} while (0)
#endif
