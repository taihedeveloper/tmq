//============================================================================
// Name        : mqserver_producer.h
// Description : taihe mqserver producer agent class
//============================================================================

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <boost/format.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <list>
#include <signal.h>
#include "nshead.h"
#include "macro.h"
#include "mc_pack.h"
#include "Configure.h"
#include "io_lib.h"
#include "mqserver_base.h"
#include "rabbitmq_producer.h"
#include "zookeeper.h"
#include "constant.h"
#include "libev_http_server.h"

#pragma once

#define DEFAULT_MCPACK_LENGTH 1024*1024*1

typedef struct _mq_nshead_t
{
    unsigned short id;              ///<id
    unsigned short version;         ///<版本号
    ///(M)由apache产生的logid，贯穿一次请求的所有网络交互
    unsigned int   log_id;
    ///(M)客户端标识，建议命名方式：产品名-模块名，比如"sp-ui", "mp3-as"
    unsigned short command_no; /// 直接从原provider里拆出2个字节来做command_no
    char           provider[14];
    ///(M)特殊标识，标识一个包的起始
    unsigned int   magic_num;
    unsigned int   reserved;       ///<保留
    ///(M)head后请求数据的总长度
    unsigned int   body_len;
} mq_nshead_t;

typedef struct _svr_response_t
{
    char buf[DEFAULT_MCPACK_LENGTH];
    char tmpbuf[DEFAULT_MCPACK_LENGTH];
    mq_nshead_t head;
    mc_pack_t *data;
} s_vt;
