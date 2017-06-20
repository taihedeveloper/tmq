/*
 * rabbitmq_consumer.h
 *
 *  Created on: Mar 14, 2017
 *      Author: root
 */

#ifndef RABBITMQ_CONSUMER_H_
#define RABBITMQ_CONSUMER_H_

#include "common.h"
#include "config_api.h"

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <boost/foreach.hpp>

class RabbitmqConsumer
{
public:
    RabbitmqConsumer();
    virtual ~RabbitmqConsumer();
public:
    void ConsumerMQ(rabbitmq_config_ptr r_config);
    void ManagerMQ();
public:
    int  Init(const comcfg::Configure& conf);
    int  Start();
    int  HandleResult(result_conf_t& rest, char * data, size_t length);
private:
    std::map<uint64_t, rabbitmq_config_ptr> rabbitmq_configs_;
    boost::thread_group                     thread_groups_;
private:
    ConfigApi *config_api_;
    boost::atomic<uint16_t> finish_thread_count_;
private:

    std::string _rabbitmq_host;
    uint16_t    _rabbitmq_port;
    uint16_t    _server_id;
    std::string _rabbitmq_exchange;
    std::string _rabbitmq_username;
    std::string _rabbitmq_password;


};

#endif /* RABBITMQ_CONSUMER_H_ */
