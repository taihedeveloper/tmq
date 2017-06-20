/*
 * common.h
 *
 *  Created on: Mar 14, 2017
 *      Author: zhangpeng
 */

#ifndef COMMON_H_

#include <string>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <string>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/atomic.hpp>

#include <Configure.h>
#include <ConfigUnit.h>
#include <ConfigReloader.h>

#include "mc_pack.h"
#include "clock.h"

#include <com_log.h>
#include <comlogplugin.h>

#define SUCCESS             10000
#define PARAM_ERROR         10001
#define SYSTEM_ERROR        10002
#define REQUEST_ILLEGAL     10003

const int32_t DEFAULT_MC_PACK_LENGTH = 1024 * 1024;

class Consumer;

#define TMQ_CONSUMER_NOTICE(fmt, arg...) do {                  \
        com_writelog(COMLOG_NOTICE, "[%s:%u]"fmt, \
                     __FILE__, __LINE__, ## arg); \
} while (0);

#define TMQ_CONSUMER_DEBUG(fmt, arg...) do {                   \
        com_writelog(COMLOG_DEBUG, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define TMQ_CONSUMER_TRACE(fmt, arg...) do {                   \
        com_writelog(COMLOG_TRACE, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define TMQ_CONSUMER_WARNING(fmt, arg...) do {                 \
        com_writelog(COMLOG_WARNING, "[%s:%u]"fmt,\
                     __FILE__, __LINE__, ## arg); \
} while(0);

#define TMQ_CONSUMER_FATAL(fmt, arg...) do {                   \
        com_writelog(COMLOG_FATAL, "[%s:%u]"fmt,  \
                     __FILE__, __LINE__, ## arg);  \
} while (0);

#define MC_PACK_GET_UINT64(p1,p2,p3,p4) \
  do { int _ret = mc_pack_get_uint64((p1), (p2), (p3)); \
    if ( 0 != _ret ) { \
      (*(p3))=(p4); }\
  }while(0)

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

#define JSON_GET_INT32(p1,p2,p3,p4) \
    do { \
        json_object* obj = json_object_object_get((p1), (p2)); \
        if (obj == NULL) \
        { \
            (*(p3))=(p4); \
        } \
        (*(p3)) = json_object_get_int(obj); \
    }while(0)

#define JSON_GET_DOUBLE(p1,p2,p3,p4) \
    do { \
        json_object* obj = json_object_object_get((p1), (p2)); \
        if (obj == NULL) \
        { \
            (*(p3))=(p4); \
        } \
        (*(p3)) = json_object_get_double(obj); \
    }while(0)

#define JSON_GET_STR(p1,p2,p3,p4) \
    do { \
        json_object* obj = json_object_object_get((p2), (p3)); \
        if (obj == NULL) \
        { \
            (p4)[0]='\0'; \
        } \
        else \
        { \
            p1 = json_object_get_string(obj); \
            snprintf((p4), sizeof(p4), "%s", (p1)); \
        } \
    }while(0)

enum CONSUMER_TYPE
{
    HTTP_CONSUMER = 1,
    MYSQL_CONSUMER = 2
};

enum COMMAND_ENCODE_TYPE
{
    ENC_MCPACK = 0,
    ENC_JSON = 1,
};

enum COMMAND_OPERATOR_TYPE
{
    ADD_COMMAND     = 1,
    DELETE_COMMAND  = 2,
    ADD_CONSUMER    = 3,
    DELETE_CONSUMER = 4,
    DELETE_ALL      = 5,
    SKIP_COMMAND    = 6
};

enum COMMAND_TYPE
{
    T_HTTP = 1,
    T_MYSQL = 2,
};

typedef struct st_rabbitmq_config
{
    std::string username;
    std::string password;
    std::string vhost;
    std::string rabbitmq_host;
    uint16_t    rabbitmq_port;
    uint16_t    consumer_server_id;
    std::string bindkey;
    std::string exchange;
    boost::atomic<bool> is_work;
    boost::atomic<bool> is_run;
    boost::atomic<bool> is_skip;
    COMMAND_ENCODE_TYPE encode_type;
    COMMAND_TYPE command_type;
    boost::thread * work_thread;
    st_rabbitmq_config()
    :is_work(true),//与消息队列相关的配置
     is_run(true),//与消费线程相关的配置
     is_skip(false), //跳过某个数据阻塞,某些消费队列配置一直阻塞，但是需要手动跳过
     work_thread(NULL)
    {
    }
} rabbitmq_config_t;

typedef boost::shared_ptr<rabbitmq_config_t> rabbitmq_config_ptr;

typedef struct st_http_struct
{
    std::string url;
    char *post_data;
    size_t post_len;
    st_http_struct():
        url(""),
        post_data(NULL),
        post_len(0)
    {
    }
}http_struct_t;



typedef struct st_command_task
{
    uint64_t command_no;
    uint32_t server_id;
    COMMAND_OPERATOR_TYPE oper_type;
    COMMAND_TYPE command_type;
    uint32_t consumer_id;
}command_task;

struct st_consumer_config
{
    std::string   _host;
    uint16_t      _port;
    COMMAND_TYPE  _consumer_type;
    Consumer      *_consumer;
    st_consumer_config(){_consumer = NULL;}
    virtual ~st_consumer_config(){if(_consumer){delete _consumer; _consumer == NULL;}}
};

//http配置
typedef struct http_conf_t:st_consumer_config
{
    int32_t     _max_retrytime;
    uint64_t    _last_update_time;
    std::string _url;
}http_conf_t;

//mysql配置
typedef struct db_conf_t:st_consumer_config
{
    std::string _db_name;
    std::string _username;
    std::string _password;
    std::string _charset;
    int32_t _max_connection;
    int32_t _min_connection;
    uint64_t _last_update_time;
}db_conf_t;

typedef int (*mcpack_for_each_kv)(mc_pack_item_t *item, std::vector<std::string>& kvs);

void static HandleAMQPError(amqp_rpc_reply_t x, char const *context)
{
    switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return;

    case AMQP_RESPONSE_NONE:
      TMQ_CONSUMER_NOTICE("%s: missing RPC reply type!\n", context);
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      TMQ_CONSUMER_NOTICE("%s: %s\n", context, amqp_error_string2(x.library_error));
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id) {
      case AMQP_CONNECTION_CLOSE_METHOD: {
        amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
        TMQ_CONSUMER_NOTICE("%s: server connection error %uh, message: %.*s\n",
                context,
                m->reply_code,
                (int) m->reply_text.len, (char *) m->reply_text.bytes);
        break;
      }
      case AMQP_CHANNEL_CLOSE_METHOD: {
        amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
        TMQ_CONSUMER_NOTICE("%s: server channel error %uh, message: %.*s\n",
                context,
                m->reply_code,
                (int) m->reply_text.len, (char *) m->reply_text.bytes);
        break;
      }
      default:
          TMQ_CONSUMER_NOTICE("%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
        break;
      }
      break;
    }

  exit(1);
}

static int get_mc_pack_str_value(const mc_pack_t * data_mcpack, const char *name, char *value, size_t len)
{
    mc_pack_item_t t;
    int ret = mc_pack_get_item(data_mcpack, name, &t);
    if(!ret)
    {
        switch (t.type)
        {
            case MC_IT_I32:
            {
                int32_t v;
                memcpy((char *)&v, t.value, sizeof(int32_t));
                snprintf((value), len, "%d", (v));
            }
            break;
            case MC_IT_I64:
            {
                int64_t v;
                memcpy((char *)&v, t.value, sizeof(int64_t));
                snprintf((value), len, "%ld", (v));
            }
            break;
            case MC_IT_U32:
            {
                uint32_t v;
                memcpy((char *)&v, t.value, sizeof(uint32_t));
                snprintf((value), len, "%u", (v));
            }
            break;
            case MC_IT_U64:
            {
                uint64_t v;
                memcpy((char *)&v, t.value, sizeof(uint64_t));
                snprintf((value), len, "%lu", (v));
            }
            break;
            case MC_IT_TXT:
            {
                snprintf((value), len, "%s", (t.value));
            }
            break;
            default:
            {
                ret = -1;
            }
            break;
        }
    }

    if(ret < 0)
    {
        (value)[0] = '\0';
    }
    return ret;
}

static int get_mc_pack_item_str_value(const mc_pack_item_t * item, char *value, size_t len)
{
    int ret = 0;
    if(item)
    {
        switch (item->type)
        {
            case MC_IT_I32:
            {
                int32_t v;
                memcpy((char *)&v, item->value, sizeof(int32_t));
                snprintf((value), len, "%d", (v));
            }
            break;
            case MC_IT_I64:
            {
                int64_t v;
                memcpy((char *)&v, item->value, sizeof(int64_t));
                snprintf((value), len, "%ld", (v));
            }
            break;
            case MC_IT_U32:
            {
                uint32_t v;
                memcpy((char *)&v, item->value, sizeof(uint32_t));
                snprintf((value), len, "%u", (v));
            }
            break;
            case MC_IT_U64:
            {
                uint64_t v;
                memcpy((char *)&v, item->value, sizeof(uint64_t));
                snprintf((value), len, "%lu", (v));
            }
            break;
            case MC_IT_TXT:
            {
                snprintf((value), len, "%s", (item->value));
            }
            break;
            default:
            {
                ret = -1;
            }
            break;
        }
    }

    if(ret < 0)
    {
        (value)[0] = '\0';
    }
    return ret;
}

static int mcpack_for_each_kv_func(mc_pack_item_t *item, std::vector<std::string>& kvs)
{
    if ( !item )
    {
        TMQ_CONSUMER_WARNING("[TMQ][mcpack_for_each_kv_func]item is NULL")
        return -1;
    }

    char value[1024] = { 0 };
    int ret = get_mc_pack_item_str_value(item, value, 1024);
    std::string kv(item->key);
    kv.append(":");
    kv.append(value);
    kvs.push_back(kv);
    return ret;
}

static int mc_pack_for_each(char *data, size_t length, std::vector<std::string>& kvs, mcpack_for_each_kv cb)
{

    mc_pack_item_t t;
    char tmp_buf[DEFAULT_MC_PACK_LENGTH] = { 0 };
    mc_pack_t* data_mcpack = mc_pack_open_r(data, DEFAULT_MC_PACK_LENGTH, tmp_buf, sizeof(tmp_buf));
    int ret = MC_PACK_PTR_ERR(data_mcpack);
    if (0 != ret)
    {
        TMQ_CONSUMER_FATAL("[mc_pack_for_each]get_message mcpack data pointer is error...");
        return (-1);
    }
    ret = mc_pack_valid(data_mcpack);
    if (1 != ret)
    {
        TMQ_CONSUMER_FATAL("[mc_pack_for_each]get_message mcpack data is error...");
        return (-1);
    }

    ret = mc_pack_first_item(data_mcpack, &t);
    if(ret != 0 && ret != MC_PE_NOT_FOUND)
    {
        TMQ_CONSUMER_FATAL("[mc_pack_for_each]get_message mcpack data is error...");
        return ret;
    }
    else if (ret == MC_PE_NOT_FOUND)
    {
        TMQ_CONSUMER_FATAL("[mc_pack_for_each]MC_PE_NOT_FOUNDr...");
        return 0;
    }

    do
    {
        cb(&t, kvs);
    } while(!mc_pack_next_item(&t, &t));

    mc_pack_close(data_mcpack);
    return ret;
}

#define COMMON_H_
#endif /* COMMON_H_ */
