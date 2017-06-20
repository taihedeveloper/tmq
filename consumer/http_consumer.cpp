/*
 * HttpConsumer.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: zhangpeng
 */

#include "http_consumer.h"
#include "common.h"

#include <boost/lexical_cast.hpp>

int http_callback(void *buffer, size_t size, size_t nmemb, void *stream);

int HttpConsumer::Init(st_consumer_config * config)
{
    http_conf_t* c = dynamic_cast<http_conf_t*>(config);
    if(NULL == c)
    {
        return -1;
    }
    _http_host = c->_host;
    _http_port = c->_port;
    _http_str = "http://" + _http_host + ":" + boost::lexical_cast<std::string>(_http_port);
    _connected = true;
    return 0;
}

int HttpConsumer::Send(const char * data, size_t length)
{
    if(!data || 0 == length)
    {
        TMQ_CONSUMER_WARNING("[HttpConsumer send data is null]");
        return -1;
    }

    int ret = 0;
    uint64_t http_code = 0;

    http_struct_t *pdata = (http_struct_t *)data;

    CURLcode CUrlRes;
    std::string m_HtmlBuff;
    struct curl_slist *chunk = NULL;

    //chunk = curl_slist_append(chunk, "Accept-Encoding: gzip, deflate");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; CIBA)");
    chunk = curl_slist_append(chunk, "Connection: Keep-Alive");
    chunk = curl_slist_append(chunk, "Expect:");

    std::string tmp_url = _http_str;
    tmp_url.append(pdata->url);
    curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(_curl, CURLOPT_VERBOSE,1);
    curl_easy_setopt(_curl, CURLOPT_TIMEOUT, 0); //CURLOPT_TIMEOUT 设置一定要大于CURLOPT_TCP_KEEPIDLE的设置CURLOPT_TIMEOUT为0不超时也行
    curl_easy_setopt(_curl, CURLOPT_URL, tmp_url.c_str());
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, http_callback);
    curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &m_HtmlBuff);
    if(pdata->post_data)
    {
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, pdata->post_len);
        curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, pdata->post_data);
    }

    CUrlRes = curl_easy_perform(_curl);
    if (CUrlRes == CURLE_OK)
    {
        curl_easy_getinfo (_curl, CURLINFO_RESPONSE_CODE, &http_code);
        uint64_t tmp_http_code = http_code /100;

        if ( tmp_http_code == 4 || tmp_http_code == 5)
        {
            ret = 1;
        }
    }
    if(ret)
    {
        TMQ_CONSUMER_NOTICE("call curl error res_code:%d, http_code:%lu", CUrlRes, http_code);
    }
    else
    {
        TMQ_CONSUMER_NOTICE("call curl success res_code:%d, http_code:%lu", CUrlRes, http_code);
    }

    curl_slist_free_all(chunk);
    return ret;
}

int HttpConsumer::Connected()
{
    if(_curl)
    {
        curl_easy_cleanup(_curl);
        _curl = NULL;
    }
    _curl = curl_easy_init();
    /* enable TCP keep-alive for this transfer */
    curl_easy_setopt(_curl, CURLOPT_TCP_KEEPALIVE, 1L);
    /* keep-alive idle time to 120 seconds */
    curl_easy_setopt(_curl, CURLOPT_TCP_KEEPIDLE, 120L);
    /* interval time between keep-alive probes: 60 seconds */
    curl_easy_setopt(_curl, CURLOPT_TCP_KEEPINTVL, 60L);
    /* interval set curl to disable signals */
    curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1);

    return 0;
}

HttpConsumer::HttpConsumer()
:_curl(NULL)
{
}

HttpConsumer::~HttpConsumer()
{
    if(_curl)
    {
        curl_easy_cleanup(_curl);
        _curl = NULL;
    }
}

int http_callback(void *buffer, size_t size, size_t nmemb, void *stream)
{
    size_t stDataLen = size* nmemb;
    char* pData = new char[stDataLen];
    if (NULL == pData)
    {
        return 0;
    }

    std::string* pHtmlDataBuff = (std::string*)stream;
    memcpy(pData, (char* )buffer, stDataLen);
    pHtmlDataBuff->append(pData, (int)stDataLen);
    delete[] pData;
    return stDataLen;
}

