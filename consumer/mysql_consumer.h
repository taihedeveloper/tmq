/*
 * MySQLConsumer.h
 *
 *  Created on: Mar 15, 2017
 *      Author: zhangpeng
 */

#ifndef MYSQL_CONSUMER_H_
#define MYSQL_CONSUMER_H_

#include "consumer.h"
#include "config_base.h"

#include <mysql.h>

class MySQLConsumer: public Consumer
{
public:
    MySQLConsumer();
    virtual ~MySQLConsumer();
public:
    int Init(st_consumer_config * config);
    int Send(const char * data, size_t lenght);
    int Connected();
    void UnInit();
private:
    MYSQL myCont;
private:
    std::string _mysql_host;
    uint16_t    _mysql_port;
    std::string _mysql_username;
    std::string _mysql_password;
    std::string _db_name;
};

#endif /* MYSQL_CONSUMER_H_ */
