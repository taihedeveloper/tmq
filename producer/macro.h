#ifndef ZDS_MACRO
#define ZDS_MACRO

#define  TYPE_APP          0
#define  TYPE_WEB          1
#define  TYPE_WEBSOCKET    2

#define  TYPE_APP_PREFIX         "APP:"   //app
#define  TYPE_WEB_PREFIX         "WEB:"   //http
#define  TYPE_WEBSOCKET_PREFIX   "WBSKT:"  //websocket

#define MAX_MSG_LEN     5000

#define PROTO_WEBSOKET_CLIENT                       111
#define PROTO_HTTP_CLIENT                           222
#define PROTO_WEBSOKET_FROM_BACKSERVER              333
#define PROTO_HTTP_FROM_BACKSERVER                  444
#define PROTO_APP_MSG                               555
#define PROTO_APP_FROM_BACKSERVER_UID_BROADCAST     666
#define PROTO_APP_FROM_BACKSERVER_BROADCAST         777
#define PROTO_UNICAST_FROM_BACKSERVER               888

#define UNUSED(x) ((void)(x))

#define MC_PACK_GET_UINT32(p1,p2,p3,p4) \
	do { int _ret = mc_pack_get_uint32((p1), (p2), (p3)); \
		if ( 0 != _ret ) { \
			(*(p3))=(p4); }\
	}while(0)

#define MC_PACK_PUT_UINT32(p1,p2,p3) \
	do{ int _ret = mc_pack_put_uint32((p1), (p2), (p3)); \
		if ( 0 !=_ret) { \
			/*return -1;*/ } \
	}while(0)

#define MC_PACK_PUT_INT32(p1,p2,p3) \
	do{ int _ret = mc_pack_put_int32((p1), (p2), (p3)); \
		if ( 0 !=_ret) { \
			/*return -1;*/ } \
	}while(0)

#define MC_PACK_GET_INT32(p1,p2,p3,p4) \
	do { int _ret = mc_pack_get_int32((p1), (p2), (p3)); \
		if ( 0 != _ret ) { \
			(*(p3))=(p4); }\
	}while(0)

#define MC_PACK_GET_STR(p1,p2,p3,p4) \
	do{ p1 = mc_pack_get_str((p2), (p3)); \
		if ( 0 != MC_PACK_PTR_ERR(p1)) {\
		(p4)[0]='\0';} \
		else snprintf((p4), sizeof(p4),"%s", (p1)); \
	}while(0)

#define MC_PACK_PUT_STR(p1,p2,p3) \
	do{ int _ret = mc_pack_put_str((p1), (p2), (p3)); \
		if ( 0 !=_ret) { \
			/*return -1;*/ } \
	}while(0)

#define MC_PACK_PUT_OBJECT(p1,p2,p3) \
	do { p1 = mc_pack_put_object(p2, p3); \
		if (0 != MC_PACK_PTR_ERR(p1)) { \
			return -1; } \
	}while(0)

#define MC_PACK_PUT_ARRAY(p1,p2,p3) \
	do { p1 = mc_pack_put_array(p2, p3); \
		if (0!=MC_PACK_PTR_ERR(p1)) { \
			return -1; } \
	}while(0)

/////////////////////////////////////////////////
#define BACKSVR_NOTICE(fmt, arg...) do {                  \
        com_writelog(COMLOG_NOTICE, "[%s:%u]"fmt, \
                     __FILE__, __LINE__, ## arg); \
} while (0);

#define BACKSVR_DEBUG(fmt, arg...) do {                   \
        com_writelog(COMLOG_DEBUG, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define BACKSVR_TRACE(fmt, arg...) do {                   \
        com_writelog(COMLOG_TRACE, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define BACKSVR_WARNING(fmt, arg...) do {                 \
        com_writelog(COMLOG_WARNING, "[%s:%u]"fmt,\
                     __FILE__, __LINE__, ## arg); \
} while(0);

#define BACKSVR_FATAL(fmt, arg...) do {                   \
        com_writelog(COMLOG_FATAL, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#endif
