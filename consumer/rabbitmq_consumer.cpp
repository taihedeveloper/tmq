/*
 * rabbitmq_consumer.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: root
 */

#include "rabbitmq_consumer.h"
#include "mysql_consumer.h"
#include "http_consumer.h"

#include <boost/foreach.hpp>

extern CLock                     g_unit_list_mutex;
extern std::list<unit_conf_t>    g_unit_list;
extern CLock                     g_command_task_list_mutex;
extern std::list<command_task>   g_command_task_list;

void RabbitmqConsumer::ConsumerMQ(rabbitmq_config_ptr r_config)
{
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    amqp_bytes_t queuename;

    const char *exchangetype = "direct";

    conn = amqp_new_connection();

    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        TMQ_CONSUMER_FATAL("open amqp_tcp_socket_new error");
        return;
    }

    int status = amqp_socket_open(socket, r_config->rabbitmq_host.c_str(), r_config->rabbitmq_port);
    if (status) {
        TMQ_CONSUMER_FATAL("open amqp_socket_open error");
        return;
    }

    HandleAMQPError(amqp_login(conn, r_config->vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, r_config->username.c_str(), r_config->password.c_str()), "Logging in");
    amqp_channel_open(conn, 1);
    HandleAMQPError(amqp_get_rpc_reply(conn), "Opening channel");

    {
        amqp_exchange_declare(conn,1, amqp_cstring_bytes(r_config->exchange.c_str()),amqp_cstring_bytes(exchangetype),0, 1, 0, 0, amqp_empty_table);
        amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_cstring_bytes(r_config->bindkey.c_str()), 0, 1, 0, 0, amqp_empty_table);
        HandleAMQPError(amqp_get_rpc_reply(conn), "Declaring queue");
        queuename = amqp_bytes_malloc_dup(r->queue);
        if (queuename.bytes == NULL) {
            TMQ_CONSUMER_WARNING("Out of memory while copying queue name");
            return;
        }
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(r_config->exchange.c_str()), amqp_cstring_bytes(r_config->bindkey.c_str()), amqp_empty_table);
    HandleAMQPError(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_qos(conn,1,0,100,0);

    std::string consumer_tag = "simple-consumer";
    amqp_basic_consume(conn, 1, queuename, amqp_cstring_bytes(consumer_tag.c_str()), 0, 0, 0, amqp_empty_table);
    HandleAMQPError(amqp_get_rpc_reply(conn), "Consuming");

    int wait_times = 0;

    while(r_config->is_run)
    {
        result_conf_t res_conf;
        amqp_frame_t frame;
        int result;
        amqp_basic_deliver_t *d;
        amqp_basic_properties_t *p;
        size_t body_target;
        size_t body_received;
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        amqp_maybe_release_buffers(conn);
        //result = amqp_simple_wait_frame(conn, &frame);
        result = amqp_simple_wait_frame_noblock(conn, &frame, &tv);
        if(AMQP_STATUS_TIMEOUT == result)
        {
            TMQ_CONSUMER_NOTICE("Result %d, command_no %s\n", result, r_config->bindkey.c_str());
            if(!r_config->is_work && ++wait_times >= 10)
            {
                TMQ_CONSUMER_NOTICE("[AMQP][routing key:%s]break queue,", r_config->bindkey.c_str());
                break;
            }
            continue;
        }

        wait_times = 0;

        if(result)
            TMQ_CONSUMER_DEBUG("[AMQP]Frame type %d, channel %d\n", frame.frame_type, frame.channel);

        if (frame.frame_type != AMQP_FRAME_METHOD)
        {
            TMQ_CONSUMER_NOTICE("[AMQP]Frame type %d, not equals AMQP_FRAME_METHOD."
                    " channel %d\n", frame.frame_type, frame.channel);
            continue;
        }

        if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
        {
            TMQ_CONSUMER_DEBUG("[AMQP]Frame id %d, not equals AMQP_BASIC_DELIVER_METHOD."
                    " channel %d\n", frame.payload.method.id, frame.channel);
            continue;
        }

        d = (amqp_basic_deliver_t *) frame.payload.method.decoded;
        printf("Delivery %u, exchange %.*s routingkey %.*s\n",(unsigned) d->delivery_tag,
            (int) d->exchange.len, (char *) d->exchange.bytes,
            (int) d->routing_key.len, (char *) d->routing_key.bytes);

        result = amqp_simple_wait_frame(conn, &frame);

        if (AMQP_FRAME_HEADER != frame.frame_type)
        {
          if (AMQP_FRAME_METHOD == frame.frame_type &&
                (AMQP_CHANNEL_CLOSE_METHOD == frame.payload.method.id ||
                 AMQP_CONNECTION_CLOSE_METHOD == frame.payload.method.id))
            {
            } else
            {
            }
            TMQ_CONSUMER_NOTICE("[AMQP]AMQP_FRAME_HEADER not equal to frame.frame_type go to error_out!");
            goto error_out1;
        }

        p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
        if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG)
        {
            printf("Content-type: %.*s\n",
            (int) p->content_type.len, (char *) p->content_type.bytes);
        }

        body_target = frame.payload.properties.body_size;
        body_received = 0;

        int sleep_seconds = 0;
        while (body_received < body_target)
        {
            result = amqp_simple_wait_frame(conn, &frame);
            if (result < 0)
                break;

            if (frame.frame_type != AMQP_FRAME_BODY) {
                TMQ_CONSUMER_NOTICE("[AMQP]Expected body!");
                abort();
            }

            body_received += frame.payload.body_fragment.len;
            assert(body_received <= body_target);

            int i;
            for(i = 0; i<frame.payload.body_fragment.len; i++)
            {
                printf("%c",*((char*)frame.payload.body_fragment.bytes+i));
            }
            printf("\n");
        }

        if (body_received != body_target)
        {
            /* Can only happen when amqp_simple_wait_frame returns <= 0 */
            /* We break here to close the connection */
            TMQ_CONSUMER_NOTICE("[AMQP] body_received not equal to body_target break!");
            break;
        }

        result_conf_t rets;
        config_api_->get_message(rets, (char *)frame.payload.body_fragment.bytes, \
                frame.payload.body_fragment.len, r_config->encode_type);

        int res = HandleResult(rets, (char *)frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
        if(res)
        {
            switch (r_config->encode_type)
            {
                case ENC_MCPACK:
                {
                    std::vector<std::string> kvs;
                    mc_pack_for_each((char *)frame.payload.body_fragment.bytes, frame.payload.body_fragment.len, kvs, mcpack_for_each_kv_func);
                    std::string kvs_str;
                    BOOST_FOREACH(std::string kv, kvs)
                    {
                        kvs_str.append(kv);
                        kvs_str.append(",");
                    }

                    if(kvs_str.length() > 1)
                    {
                        kvs_str = kvs_str.substr(0, kvs_str.length() - 1);
                    }

                    TMQ_CONSUMER_NOTICE("[AMQP] Handle result error, mcpack data:%s", kvs_str.c_str());
                }
                    break;
                case ENC_JSON:
                {
                    TMQ_CONSUMER_NOTICE("[AMQP] Handle result error, json data:%s", (char *)frame.payload.body_fragment.bytes);
                }
                    break;
                default:
                    break;
            }
            /*The default retry count is 3, the mark that has been retried is 0 */
            int64_t retry = rets._retry_counts;
            do
            {
                sleep(1);
                res = HandleResult(rets, (char *)frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
                TMQ_CONSUMER_NOTICE("[TMQ] consumer retry:command_no %s", r_config->bindkey.c_str());
            }while(res && --retry && r_config->is_skip);
        }
        amqp_basic_ack(conn, 1, d->delivery_tag, 0);

        if(r_config->is_skip)
        {
            r_config->is_skip = false;
        }

  }

error_out1:
    amqp_queue_delete(conn, 1, amqp_cstring_bytes(r_config->bindkey.c_str()), true, true);
    HandleAMQPError(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
    HandleAMQPError(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");

    if(amqp_destroy_connection(conn) < 0)
    {
        //log
        TMQ_CONSUMER_FATAL("[AMQP]amqp_destroy_connection ERROR");
    }

    if(!r_config->is_work)
    {
        command_task task;
        char *ptr = NULL;
        task.command_no = strtoul(r_config->bindkey.c_str() , &ptr, 10);
        task.server_id  = r_config->consumer_server_id;
        task.oper_type  = DELETE_COMMAND;
        task.command_type = r_config->command_type;

        g_command_task_list_mutex.lock();
        g_command_task_list.push_back(task);
        g_command_task_list_mutex.unlock();
    }
    else if(!r_config->is_run)
    {
        finish_thread_count_++;
    }

    TMQ_CONSUMER_NOTICE("Finish handle tmq queue:%s", r_config->bindkey.c_str());

}

int RabbitmqConsumer::Init(const comcfg::Configure& conf)
{
    const comcfg::ConfigUnit& rconf =  conf["RabbitmqConf"];

    _rabbitmq_host     = rconf["RabbitmqHost"].to_cstr();
    _rabbitmq_port     = rconf["RabbitmqPort"].to_uint16();
    _server_id         = rconf["ServerID"].to_uint16();
    _rabbitmq_exchange = rconf["RabbitmqExchange"].to_cstr();
    _rabbitmq_username = rconf["RabbitmqUsername"].to_cstr();
    _rabbitmq_password = rconf["RabbitmqPassword"].to_cstr();

    if(config_api_->load(conf, _server_id))
    {
        TMQ_CONSUMER_FATAL("[RabbitmqConsumer]init ConfigApi error");
        BOOST_ASSERT(0);
        return -1;
    }

    std::vector<unit_conf_t> unit_confs;
    config_api_->get_command_list(unit_confs);

    std::vector<unit_conf_t>::iterator iter = unit_confs.begin();

    for(; iter != unit_confs.end(); iter++)
    {
        rabbitmq_config_ptr rc(new rabbitmq_config_t);
        rc->bindkey            = boost::lexical_cast<std::string>(iter->_command_no);
        rc->consumer_server_id = _server_id;
        rc->encode_type        = iter->_command_encode_type;
        rc->exchange           = _rabbitmq_exchange;
        rc->rabbitmq_host      = _rabbitmq_host;
        rc->rabbitmq_port      = _rabbitmq_port;
        rc->username           = _rabbitmq_username;
        rc->password           = _rabbitmq_password;
        rc->vhost              = iter->_vhost;
        rc->command_type       = iter->_command_type;
        rabbitmq_configs_.insert(std::make_pair(iter->_command_no, rc));
    }

    return 0;
}

int RabbitmqConsumer::Start()
{
    boost::thread_group group;

    std::map<uint64_t, rabbitmq_config_ptr>::iterator iter = rabbitmq_configs_.begin();

    for(;iter != rabbitmq_configs_.end(); iter++)
    {
        boost::thread *p = new boost::thread(boost::bind(&RabbitmqConsumer::ConsumerMQ, this, iter->second));
        thread_groups_.add_thread(p);
        iter->second->work_thread = p;
    }

    thread_groups_.create_thread(boost::bind(&RabbitmqConsumer::ManagerMQ, this));

    return 0;
}

int RabbitmqConsumer::HandleResult(result_conf_t& rest, char *data, size_t length)
{
    int res = 0;
    switch(rest._command_type)
    {
    case T_MYSQL:
    {
        TMQ_CONSUMER_DEBUG("[command no:%d, type:T_MYSQL, template_size:%d]", rest._command_no, rest._tpl_confs.size());
        std::vector<std::pair<void*, std::string> >::iterator itor = rest._tpl_confs.begin();
        for(; itor != rest._tpl_confs.end(); itor++)
        {
            db_conf_t * p = (db_conf_t *)itor->first;
            if(!p->_consumer)
            {
                p->_consumer = new MySQLConsumer();
                p->_consumer->Init(p);
                p->_consumer->Connected();
            }

            if(!p->_consumer->IsConnected())
            {
                TMQ_CONSUMER_WARNING("[ip:%s, port:%u, user:%s, db_name:%s] not connected", p->_host.c_str(), p->_port, p->_username.c_str(), p->_db_name.c_str());
                p->_consumer->Connected();
                continue;
            }
            else
            {
                res = p->_consumer->Send(itor->second.c_str(), itor->second.size());
                TMQ_CONSUMER_DEBUG("[command no:%d, type:T_MYSQL, send content%s]", rest._command_no, itor->second.c_str());
            }
        }
    }
    break;
    case T_HTTP:
    {
        std::vector<std::pair<void*, std::string> >::iterator itor = rest._tpl_confs.begin();
        TMQ_CONSUMER_DEBUG("[command no:%d, type:T_HTTP, template_size:%d]", rest._command_no, rest._tpl_confs.size());
        for(; itor != rest._tpl_confs.end(); itor++)
        {
            http_conf_t * p = (http_conf_t *)itor->first;
            if(!p->_consumer)
            {
                p->_consumer = new HttpConsumer();
                p->_consumer->Init(p);
                p->_consumer->Connected();
            }

            http_struct_t hs;
            hs.post_data = data;
            hs.post_len  = length;
            hs.url       = itor->second;
            res = p->_consumer->Send((char *)&hs, sizeof(hs));
            TMQ_CONSUMER_DEBUG("[command no:%d, type:T_HTTP, send content%s]", rest._command_no, itor->second.c_str());
        }
    }
    break;
    default:
        break;
    }
    return res;
}

void RabbitmqConsumer::ManagerMQ()
{
    std::list<unit_conf_t> unit_list;
    while(1)
    {
        g_unit_list_mutex.lock();
        unit_list.swap(g_unit_list);
        g_unit_list_mutex.unlock();
        if(unit_list.empty())
        {
            sleep(1);
            continue;
        }

        std::list<unit_conf_t>::iterator iter = unit_list.begin();
        for(; iter != unit_list.end(); iter++)
        {
            if(DELETE_ALL == iter->_oper_type)
            {
                uint16_t thread_size = rabbitmq_configs_.size();
                std::map<uint64_t, rabbitmq_config_ptr>::iterator r_itor = rabbitmq_configs_.begin();
                while(r_itor != rabbitmq_configs_.end())
                {
                    r_itor->second->is_run = false;
                    if(r_itor->second->work_thread)
                    {
                        thread_groups_.remove_thread(r_itor->second->work_thread);
                        delete r_itor->second->work_thread;
                        r_itor->second->work_thread = NULL;
                    }
                    rabbitmq_configs_.erase(r_itor++);
                }

                int count = 0;

                while(count++ < 5)
                {
                    if(finish_thread_count_ == thread_size)
                    {
                        exit(1);
                    }
                    sleep(1);
                }

                exit(1);

            }
            std::map<uint64_t, rabbitmq_config_ptr>::iterator r_itor = rabbitmq_configs_.find(iter->_command_no);
            if(r_itor != rabbitmq_configs_.end())
            {
                if(ADD_COMMAND == iter->_oper_type) continue;

                if(SKIP_COMMAND == iter->_oper_type)
                {
                    r_itor->second->is_skip = true;
                    continue;
                }

                r_itor->second->is_work = false;
                if(r_itor->second->work_thread)
                {
                    thread_groups_.remove_thread(r_itor->second->work_thread);
                    delete r_itor->second->work_thread;
                    r_itor->second->work_thread = NULL;
                }

                rabbitmq_configs_.erase(r_itor);
            }
            else
            {
                if(DELETE_COMMAND == iter->_oper_type) continue;
                rabbitmq_config_ptr rc(new rabbitmq_config_t);
                rc->bindkey            = boost::lexical_cast<std::string>(iter->_command_no);
                rc->consumer_server_id = _server_id;
                rc->encode_type        = iter->_command_encode_type;
                rc->exchange           = _rabbitmq_exchange;
                rc->rabbitmq_host      = _rabbitmq_host;
                rc->rabbitmq_port      = _rabbitmq_port;
                rc->username           = _rabbitmq_username;
                rc->password           = _rabbitmq_password;
                rc->vhost              = iter->_vhost;
                rc->command_type       = iter->_command_type;

                boost::thread *p = new boost::thread(boost::bind(&RabbitmqConsumer::ConsumerMQ, this, rc));
                thread_groups_.add_thread(p);
                rc->work_thread = p;
                rabbitmq_configs_.insert(std::make_pair(iter->_command_no, rc));
            }
        }
        unit_list.clear();
    }
}


RabbitmqConsumer::RabbitmqConsumer()
:config_api_(new ConfigApi()),
 finish_thread_count_(0)
{
}

RabbitmqConsumer::~RabbitmqConsumer()
{
    thread_groups_.join_all();
    // TODO Auto-generated destructor stub
    if(config_api_)
    {
        delete config_api_;
        config_api_ = NULL;
    }
}
