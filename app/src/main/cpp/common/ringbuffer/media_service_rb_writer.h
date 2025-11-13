#ifndef _media_service_rb_writer_
#define _media_service_rb_writer_

// #include "media_service_wrap.h"

int media_service_rb_writer_register(char *devicename, int deviceid, int mediatype, void *arg, void **ppctx);

int media_service_rb_writer_unregister(int deviceid, int mediatype, void *pctx);

int media_service_rb_writer_trigger(void *pctx, void *data, int datalen);

#endif