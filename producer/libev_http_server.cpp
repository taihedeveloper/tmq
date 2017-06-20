//============================================================================
// Name        : libev_http_server.cpp
// Description : libevent http multi_thread class
//============================================================================

#include "libev_http_server.h"

std::map<pthread_t, rabbitmq_producer * > libev_http_server::rmq_table;
std::string libev_http_server::format_json(int error_no, std::string error_msg)
{
    boost::format f("{\"error_no\": \"%1%\",\"error_msg\": \"%2%\"}");
    f % error_no;
    f % error_msg;
    std::string ret = f.str();
    return ret;
}

libev_http_server::libev_http_server()
{

}

libev_http_server::~libev_http_server()
{

}

void* libev_http_server::dispatch(void *arg)
{
    event_base_dispatch((struct event_base*)arg);
    return NULL;
}

int libev_http_server::bind_socket(int32_t port, int32_t http_num)
{
    int nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd < 0)
    {
        MQSERVER_FATAL("Open http socket fail.");
        return -1;
    }

    int one = 1;
    int sock_ret = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
    UNUSED(sock_ret);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    int bind_ret = bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
    if (bind_ret < 0)
    {
        MQSERVER_FATAL("Bind http address fail.");
        return -1;
    }
    int listen_ret = listen(nfd, http_num);
    if (listen_ret < 0)
    {
        MQSERVER_FATAL("Listen http number open fail.");
        return -1;
    }

    int flags;
    if ((flags = fcntl(nfd, F_GETFL, 0)) < 0 || fcntl(nfd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        MQSERVER_FATAL("Http fcntl operator fail.");
        return -1;
    }

    return nfd;
}

int libev_http_server::http_start(int32_t port, int32_t http_num, int32_t nthreads, std::string rmq_ip, int32_t rmq_port, std::string rmq_user, std::string rmq_pwd)
{
    int nfd = bind_socket(port, http_num);
    if (nfd < 0)
    {
        return -1;
    }
    pthread_t ths[nthreads];
    for (int i = 0; i < nthreads; i++)
    {
        struct event_base *base = event_init();
        if (NULL == base)
        {
            MQSERVER_FATAL("Init new event_base fail.");
            return -1;
        }
        struct evhttp *httpd = evhttp_new(base);
        if (NULL == httpd)
        {
            MQSERVER_FATAL("eevhttp_new fail.");
            return -1;
        }
        int acc_ret = evhttp_accept_socket(httpd, nfd);
        if (acc_ret != 0)
        {
            return -1;
        }

        int timeout = 60;
        evhttp_set_timeout(httpd, timeout);
        evhttp_set_gencb(httpd, libev_http_server::genic_request, this);
        int thread_ret = pthread_create(&ths[i], NULL, libev_http_server::dispatch, base);
        if (thread_ret != 0)
        {
            MQSERVER_FATAL("Create pthread fail.");
            return -1;
        }
        rabbitmq_producer *producer_single = new rabbitmq_producer();
        producer_single->init_rabbitmq(rmq_ip, rmq_port, rmq_user, rmq_pwd);
        producer_single->link_rabbitmq();
        rmq_table[ths[i]] = producer_single;
    }
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(ths[i], NULL);
    }
    return 0;
}

void libev_http_server::genic_request(struct evhttp_request * req, void *argv)
{
    UNUSED(argv);
    pthread_t self_id = pthread_self();
    rabbitmq_producer *thread_rmq = rmq_table[self_id];

    struct evbuffer *buf;
    buf = evbuffer_new();

    std::string trans_str;

    // 先解析get请求参数
    const char *uri = evhttp_request_uri(req);
    char *decoded_uri = evhttp_decode_uri(uri);
    std::string uri_str = decoded_uri;
    if (!uri_str.compare("/favicon.ico")) // 处理默认请求
    {
        evbuffer_add_printf(buf, "%s", "");
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }

    std::string check_str = uri_str.substr(0, 9);
    std::string ret_str;
    if (check_str.compare("/mqserver") != 0)
    {
        ret_str = format_json(REQUEST_ILLEGAL, "Request illegal.");
        evbuffer_add_printf(buf, ret_str.c_str());
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }

    struct evkeyvalq params;
    evhttp_parse_query(decoded_uri, &params);
    const char *command_no = evhttp_find_header(&params, "command_no");

    if(!command_no)
    {
        ret_str = format_json(REQUEST_ILLEGAL, "command_no not found.");
        evbuffer_add_printf(buf, ret_str.c_str());
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }

    int buffer_data_len = EVBUFFER_LENGTH(req->input_buffer);
    if (buffer_data_len == 0)
    {
        ret_str = format_json(REQUEST_ILLEGAL, "Request illegal.");
        evbuffer_add_printf(buf, ret_str.c_str());
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }

    char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);
    std::string cmd_str = command_no;

    if (thread_rmq->send_msg_to_rabbitmq("amq.direct", cmd_str, post_data, buffer_data_len) > 0)
    {
        ret_str = format_json(SUCCESS, "Success.");
        evbuffer_add_printf(buf, ret_str.c_str());
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }
    else
    {
        ret_str = format_json(SYSTEM_ERROR, "Send data to rabbitmq failed.");
        evbuffer_add_printf(buf, ret_str.c_str());
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }
}
