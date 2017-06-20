//============================================================================
// Name        : rabbitmq_producer.cpp
// Description : rabbitmq_producer base class
//============================================================================

#include "rabbitmq_producer.h"

rabbitmq_producer::rabbitmq_producer()
{

}

rabbitmq_producer::~rabbitmq_producer()
{

}

void rabbitmq_producer::init_rabbitmq(std::string hostname, int port, std::string username, std::string password)
{
    this->hostname = hostname;
    this->port = port;
    this->username = username;
    this->password = password;
}

int rabbitmq_producer::link_rabbitmq()
{
    this->conn = amqp_new_connection();
    this->socket = amqp_tcp_socket_new(this->conn);
    if (!this->socket)
    {
        MQSERVER_FATAL("Rabbitmq: make a new socket fail.");
        return -1;
    }

    int status = amqp_socket_open(this->socket, this->hostname.c_str(), this->port);
    if (status)
    {
        MQSERVER_FATAL("Rabbitmq: open the socket fail.");
        return -1;
    }
    amqp_rpc_reply_t login_reply = amqp_login(this->conn, "tmq", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, this->username.c_str(), this->password.c_str());
    if (AMQP_RESPONSE_NORMAL != login_reply.reply_type)
    {
        MQSERVER_FATAL("Rabbitmq: input username and password fail.");
        return -1;
    }
    amqp_channel_open(conn, 1);
    amqp_rpc_reply_t rpc_reply = amqp_get_rpc_reply(this->conn);
    if (AMQP_RESPONSE_NORMAL != rpc_reply.reply_type)
    {
        MQSERVER_FATAL("Rabbitmq: get rpc reply fail.");
        return -1;
    }
    return 1;
}

int rabbitmq_producer::unlink_rabbitmq()
{
    amqp_rpc_reply_t channel_close_reply = amqp_channel_close(this->conn, 1, AMQP_REPLY_SUCCESS);
    if (AMQP_RESPONSE_NORMAL != channel_close_reply.reply_type)
    {
        MQSERVER_FATAL("Rabbitmq: channel close fail.");
        return -1;
    }
    amqp_rpc_reply_t connect_close_reply = amqp_connection_close(this->conn, AMQP_REPLY_SUCCESS);
    if (AMQP_RESPONSE_NORMAL != connect_close_reply.reply_type)
    {
        MQSERVER_FATAL("Rabbitmq: connection close fail.");
        return -1;
    }
    int destroy_connect_reply = amqp_destroy_connection(this->conn);
    if (destroy_connect_reply < 0)
    {
        MQSERVER_FATAL("Rabbitmq: destroy connection fail.");
        return -1;
    }
    return 1;
}

int rabbitmq_producer::send_msg_to_rabbitmq(std::string exchange, std::string routingkey, const char* message, size_t msg_size)
{
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = 2;
    amqp_bytes_t message_bytes;
    message_bytes.len = msg_size;
    message_bytes.bytes = (void *)message;
    int publish_reply = amqp_basic_publish(this->conn, 1, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(routingkey.c_str()), 0, 0, &props, message_bytes);
    if (publish_reply < 0) // 重试一次
    {
        this->unlink_rabbitmq();
        this->link_rabbitmq();
        int second_publish_reply = amqp_basic_publish(this->conn, 1, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(routingkey.c_str()), 0, 0, &props, message_bytes);
        if (second_publish_reply < 0)
        {
            MQSERVER_FATAL("Rabbitmq: send message to rabbitmq fail.");
            return -1;
        }
    }
    else
    {
        return 1;
    }
}
