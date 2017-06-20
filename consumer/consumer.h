/*
 * consumer.h
 *
 *  Created on: Mar 15, 2017
 *      Author: root
 */

#ifndef CONSUMER_H_
#define CONSUMER_H_

#include <string>
#include <boost/atomic.hpp>

struct st_consumer_config;

class Consumer
{
public:
    Consumer();
    virtual ~Consumer();
public:
    virtual int Init(st_consumer_config * config) = 0;
    virtual int Send(const char * data, size_t lenght) = 0;
    virtual int Connected() = 0;
    virtual bool IsConnected();
public:
    void dump_file(std::string file_path);
protected:
    boost::atomic<bool> _connected;
};

#endif /* CONSUMER_H_ */
