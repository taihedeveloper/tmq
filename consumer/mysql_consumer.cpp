/*
 * MySQLConsumer.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: zhangpeng
 */

#include "mysql_consumer.h"
#include "common.h"

int MySQLConsumer::Init(st_consumer_config* config)
{
    db_conf_t* c = dynamic_cast<db_conf_t*>(config);
    if(NULL == c)
    {
        return -1;
    }

    _mysql_host = c->_host;
    _mysql_port = c->_port;
    _mysql_username = c->_username;
    _mysql_password = c->_password;
    _db_name        = c->_db_name;
    return 0;
}

int MySQLConsumer::Send(const char * data, size_t lenght)
{
    if(mysql_ping(&this->myCont))
    {
        _connected = false;
        return -1;
    }

    int res = mysql_query(&this->myCont, data);
    if(res)
    {
        TMQ_CONSUMER_NOTICE("mysql error:%s", mysql_error(&this->myCont));
    }
    return res;
}

int MySQLConsumer::Connected()
{
    char value = 1;
    mysql_init(&this->myCont);
    mysql_options(&this->myCont, MYSQL_OPT_RECONNECT, (char *)&value);
    if (!mysql_real_connect(&this->myCont, _mysql_host.c_str(), _mysql_username.c_str(), _mysql_password.c_str(), _db_name.c_str(), _mysql_port, NULL, 0))
    {
        _connected = false;
        return -1;
    }
    else
    {
        _connected = true;
    }
    return 0;
}

void MySQLConsumer::UnInit()
{
    mysql_close(&this->myCont);
}

MySQLConsumer::MySQLConsumer()
{
}

MySQLConsumer::~MySQLConsumer()
{
    // TODO Auto-generated destructor stub
}

