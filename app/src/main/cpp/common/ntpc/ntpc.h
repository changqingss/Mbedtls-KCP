#ifndef NTPCLIENT_H
#define NTPCLIENT_H

// #define ENABLE_NTP_DEBUG
// #define ENABLE_NTP_REPLAY

/* when present, debug is a true global */
#ifdef ENABLE_NTP_DEBUG
extern int debug;
#else
#define debug 0
#endif

/* global tuning parameter */
extern double ntp_min_delay;

#include "../log/log_conf.h"

#define NTP_INFO(fmt, ...) LOG_I("NTPC", fmt, ##__VA_ARGS__)
#define NTP_ERR(fmt, ...) LOG_E("NTPC", fmt, ##__VA_ARGS__)

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);
int ntpc(const char* hostname);

#endif
