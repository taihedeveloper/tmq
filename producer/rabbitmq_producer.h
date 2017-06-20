//============================================================================
// Name        : rabbitmq_producer.h
// Description : rabbitmq_producer base class
//============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include <stdint.h>
#include "amqp_tcp_socket.h"
#include "amqp_framing.h"

#include "mqserver_base.h"
#pragma once

class rabbitmq_producer
{
public:
    rabbitmq_producer();
    virtual ~rabbitmq_producer();

public:
    amqp_socket_t *socket;
    amqp_connection_state_t conn;
    std::string hostname;
    int port;
    std::string username;
    std::string password;

public:
    void init_rabbitmq(std::string hostname, int port, std::string username, std::string password);
    int link_rabbitmq();
    int unlink_rabbitmq();
    int send_msg_to_rabbitmq(std::string exchange, std::string routingkey, const char * message, size_t len);
};
