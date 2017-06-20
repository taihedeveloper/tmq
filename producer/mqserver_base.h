#ifndef MQSERVER_BASE_H
#define MQSERVER_BASE_H

#include "com_log.h"
#include "comlogplugin.h"

#define SAFE_MALLOC(type, count) (type*)malloc(sizeof(type) * (count))

#define SAFE_FREE(ptr) \
{ \
    if ((ptr) != NULL) \
    { \
        free((void *)ptr); \
        (ptr) = NULL; \
    } \
}

#define UNUSED(x) ((void)(x))

#define MQSERVER_DEBUG(fmt, arg...) do {                   \
        com_writelog(COMLOG_DEBUG, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define MQSERVER_TRACE(fmt, arg...) do {                   \
        com_writelog(COMLOG_TRACE, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define MQSERVER_NOTICE(fmt, arg...) do {                  \
        com_writelog(COMLOG_NOTICE, "[%s:%u]"fmt, \
                     __FILE__, __LINE__, ## arg); \
} while (0);

#define MQSERVER_WARNING(fmt, arg...) do {                 \
        com_writelog(COMLOG_WARNING, "[%s:%u]"fmt,\
                     __FILE__, __LINE__, ## arg); \
} while(0);

#define MQSERVER_FATAL(fmt, arg...) do {                   \
        com_writelog(COMLOG_FATAL, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#endif
