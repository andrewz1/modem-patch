#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define infof(fmt, ...)                      \
	do {                                     \
		fprintf(stdout, fmt, ##__VA_ARGS__); \
		fflush(stdout);                      \
	} while (0)

#define errorf(fmt, ...)                     \
	do {                                     \
		fprintf(stderr, fmt, ##__VA_ARGS__); \
		fflush(stderr);                      \
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
		fprintf(stdout, fmt, ##__VA_ARGS__); \
		fflush(stdout);                      \
	} while (0)
#else
#define debugf(fmt, ...) \
	do {                 \
	} while (0)
#endif

#define die()                                                            \
	do {                                                                 \
		errorf("died at %s:%d[%s]\n", __FILE__, __LINE__, __FUNCTION__); \
		abort();                                                         \
	} while (0)

#define dief(fmt, ...)              \
	do {                            \
		errorf(fmt, ##__VA_ARGS__); \
		die();                      \
	} while (0)

static inline char *prM(char *buf, const void *p)
{
	const uint8_t *m = p;
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
	return buf;
}

static inline char *prIp4(char *buf, const void *p)
{
	inet_ntop(AF_INET, p, buf, INET_ADDRSTRLEN);
	return buf;
}

static inline char *prIp6(char *buf, const void *p)
{
	inet_ntop(AF_INET6, p, buf, INET6_ADDRSTRLEN);
	return buf;
}

static inline char *prIp(char *buf, int af, const void *p)
{
	inet_ntop(af, p, buf, af == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN);
	return buf;
}
