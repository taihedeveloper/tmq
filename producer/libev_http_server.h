//============================================================================
// Name        : libev_http_server.h
// Description : libevent http multi_thread class
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <event.h>
#include <evhttp.h>
#include <map>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <fstream>
#include <set>
#include "zookeeper.h"
#include "constant.h"
#include "rabbitmq_producer.h"

#include "mqserver_base.h"
#pragma once

class libev_http_server
{
    public:
        static std::map<pthread_t, rabbitmq_producer * > rmq_table;
        static std::map<pthread_t, zhandle_t * > zk_table;
    public:
        libev_http_server();
        virtual ~libev_http_server();
        static void* dispatch(void *arg);
        int bind_socket(int port, int http_num);
        int http_start(int32_t port, int32_t http_num, int32_t nthreads, std::string rmq_ip, int32_t rmq_port, std::string rmq_user, std::string rmq_pwd);
        static void genic_request(struct evhttp_request * req, void *argv);
        static std::string format_json(int error_no, std::string error_msg);
};
