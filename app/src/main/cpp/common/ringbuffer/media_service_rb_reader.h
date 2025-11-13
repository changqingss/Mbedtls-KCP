#ifndef _media_service_rb_reader_
#define _media_service_rb_reader_

// #include "media_service_wrap.h"

int media_service_rb_reader_register(char *devicename, int deviceid, int mediatype, void *arg, void **ppctx,
                                     long long int pretimes);

int media_service_rb_reader_unregister(int deviceid, int mediatype, void *pctx);
int media_service_rb_reader_proc(void *pctx, int msgtype, void **data, int *datalen);
int media_service_rb_reader_oldest(void **ppctx);
int media_service_rb_reader_latest(void **ppctx);
int media_service_rb_reader_discard_all_data(void **ppctx);
int media_service_rb_reader_move_to_time(void **ppctx, long long int pretimes);

#endif