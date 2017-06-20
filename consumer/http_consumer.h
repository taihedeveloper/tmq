/*
 * HttpConsumer.h
 *
 *  Created on: Mar 15, 2017
 *      Author: zhangpeng
 */

#ifndef HTTP_CONSUMER_H_
#define HTTP_CONSUMER_H_

#include "consumer.h"

#include <curl/curl.h>
#include <curl/easy.h>

class HttpConsumer: public Consumer
{
public:
    HttpConsumer();
    virtual ~HttpConsumer();
public:
    int Init(st_consumer_config * config);
    int Send(const char * data, size_t lenght);
    int Connected();
private:
    CURL *_curl;
private:
    std::string _http_host;
    uint16_t    _http_port;
    std::string _http_str;
};

#endif /* HTTP_CONSUMER_H_ */
