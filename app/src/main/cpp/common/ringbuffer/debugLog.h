#ifndef __ringbuffer_debuglog_h__
#define __ringbuffer_debuglog_h__
#include <errno.h>
#include <sys/time.h>


enum DebugLevel
{
    DEBUG_LEVEL_ERROR,
    DEBUG_LEVEL_WARNING,
    DEBUG_LEVEL_INFO,
    DEBUG_LEVEL_MAX,
};

#define DEBUG_ERROR(module, format...) 
#define DEBUG_WARNING(module, format...)
#define DEBUG_INFO(module, format...)
#define DEBUG_API_ERROR(module, apiName...)


#define DEBUG_FLOW_0(fmt, ...)
#define DEBUG_FLOW_1(fmt, ...)
#define DEBUG_FLOW_5(fmt, ...)

#endif /* DEBUGLOG_H_ */

