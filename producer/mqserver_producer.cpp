//============================================================================
// Name        : mqserver_producer.cpp
// Description : taihe mqserver producer agent class
//============================================================================

#include "mqserver_producer.h"

std::vector<s_vt * > _svt_vec;
std::vector<s_vt * > _svt_response_vec;
std::vector<rabbitmq_producer * > _rmq_pro_vec;
std::vector<rabbitmq_producer * > _rmq_http_vec;

string format_json(int error_no, string error_msg)
{
    boost::format f("{\"error_no\": \"%1%\",\"error_msg\": \"%2%\"}");
    f % error_no;
    f % error_msg;
    string ret = f.str();
    return ret;
}

void unexpected_signal(int sig, siginfo_t* info, void* context)
{
    UNUSED(context);
    signal(sig, SIG_IGN);
    MQSERVER_WARNING("[%ld] Unexpected signal %d caught, source pid is %d, code=%d.", pthread_self(), sig, info->si_pid, info->si_code);

    if (sig != 15 && sig != 2)
    {
        abort();
    }
    else
    {
        exit(2);
    }
}

void signal_setup()
{
    struct sigaction act;
    act.sa_flags =SA_SIGINFO;

    //ignored signals
    act.sa_sigaction = NULL;
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
    signal(SIGCLD, SIG_IGN);

#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
    signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
    //unexpected signal
    act.sa_sigaction = unexpected_signal;
    sigaction(SIGILL, &act, NULL);
    //sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
#ifdef SIGEMT
    sigaction(SIGEMT, &act, NULL);
#endif
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGSYS, &act, NULL);
    sigaction(SIGPWR, &act, NULL);
    //sigaction(SIGTERM, &act, NULL);
#ifdef SIGSTKFLT
    sigaction(SIGSTKFLT, &act, NULL);
#endif

    return;
}

string ushort2string(unsigned short num)
{
    stringstream stream;
    stream << num;
    string ret = stream.str();
    return ret;
}

int print_func(const int print_level, const char* p_info)
{
    return 1;
}

int work_func(const uint32_t peer_id, const uint32_t thread_idx, const char* buf, const int len)
{
    s_vt *request = _svt_vec[thread_idx];
    rabbitmq_producer *peer_producer = _rmq_pro_vec[thread_idx];
    memcpy(&request->head, buf, sizeof(request->head));
    memcpy(request->buf, buf + sizeof(request->head), len - sizeof(request->head));
    request->data = mc_pack_open_r(request->buf, DEFAULT_MCPACK_LENGTH, request->tmpbuf, sizeof(request->tmpbuf));
    if (0 != MC_PACK_PTR_ERR(request->data))
    {
        return -1;
    }

    unsigned short command_no;
    command_no = request->head.command_no;

    s_vt *response = _svt_response_vec[thread_idx];
    memcpy(&response->head, buf, sizeof(request->head));
    response->data = mc_pack_open_w(2, response->buf, DEFAULT_MCPACK_LENGTH, response->tmpbuf, sizeof(response->tmpbuf));
    if (0 != MC_PACK_PTR_ERR(response->data))
    {
        return -1;
    }

    if (command_no == 0) // 老版本会把command_no放到mcpack包内
    {
        uint32_t mc_command_no;
        MC_PACK_GET_UINT32(request->data, "command_no", &mc_command_no, 0);
        mc_pack_close(request->data);
        if (mc_command_no == 0)
        {
            MQSERVER_WARNING("command_no not exist.");
            MC_PACK_PUT_INT32(response->data, "error_no", PARAM_ERROR);
            MC_PACK_PUT_STR(response->data, "error_msg", "command_no not exist.");
            mc_pack_close(response->data);
            response->head.body_len = mc_pack_get_size(response->data);
            int body_len = response->head.body_len;
            int new_len = body_len + sizeof(response->head);
            char *new_buf = new char[new_len];
            memcpy(new_buf, &response->head, sizeof(response->head));
            memcpy(new_buf + sizeof(response->head), response->buf, body_len);
            libio_send_msg_to_io_thread(peer_id, new_buf, new_len);
            if (new_buf)
            {
                delete[] new_buf;
                new_buf = NULL;
            }
            return -1;
        }
		command_no = mc_command_no;
    }

    string cmd_str = ushort2string(command_no);
    if (peer_producer->send_msg_to_rabbitmq("amq.direct", cmd_str, request->buf, len - sizeof(request->head)) > 0)
    {
        MC_PACK_PUT_INT32(response->data, "error_no", SUCCESS);
        MC_PACK_PUT_STR(response->data, "error_msg", "Success.");
        mc_pack_close(response->data);
        response->head.body_len = mc_pack_get_size(response->data);
        int body_len = response->head.body_len;
        int new_len = body_len + sizeof(response->head);
        char *new_buf = new char[new_len];
        memcpy(new_buf, &response->head, sizeof(response->head));
        memcpy(new_buf + sizeof(response->head), response->buf, body_len);
        libio_send_msg_to_io_thread(peer_id, new_buf, new_len);
        if (new_buf)
        {
            delete[] new_buf;
            new_buf = NULL;
        }
        return 1;
    }
    else
    {
        MQSERVER_WARNING("Send data to rabbitmq failed.");
        MC_PACK_PUT_INT32(response->data, "error_no", SYSTEM_ERROR);
        MC_PACK_PUT_STR(response->data, "error_msg", "Send data to rabbitmq failed.");
        mc_pack_close(response->data);
        response->head.body_len = mc_pack_get_size(response->data);
        int body_len = response->head.body_len;
        int new_len = body_len + sizeof(response->head);
        char *new_buf = new char[new_len];
        memcpy(new_buf, &response->head, sizeof(response->head));
        memcpy(new_buf + sizeof(response->head), response->buf, body_len);
        libio_send_msg_to_io_thread(peer_id, new_buf, new_len);
        if (new_buf)
        {
            delete[] new_buf;
            new_buf = NULL;
        }
        return -1;
    }
}

void lib_io_start(int32_t listen_port, int32_t io_thread_num, int32_t work_thread_num)
{
    libio_create_listen_socket("0.0.0.0", listen_port);
    libio_set_io_thread(io_thread_num);
    libio_set_work_thread(work_thread_num, work_func);
    libio_set_print_callback(print_func);
    libio_set_peer_timeout(600000);
    libio_init_and_start();
}

int main()
{
    // 忽略信号
    signal_setup();

    // 读取配置文件
    comcfg::Configure conf;
    if (-1 == conf.load("./conf", "tmq_producer.conf", NULL))
    {
        return -1;
    }

    // 初始化log组件
    if (-1 == comlog_init(conf["Log"]))
    {
        return -1;
    }

    // 获取网络库监听端口、IO线程数以及工作线程数配置
    int32_t listen_port = conf["IOLibOption"]["Listen_Port"].to_int32();
    int32_t io_thread_num = conf["IOLibOption"]["IOThreadNum"].to_int32();
    int32_t work_thread_num = conf["IOLibOption"]["WorkThreadNum"].to_int32();

    // Http服务配置
    int32_t http_port = conf["HttpOption"]["Listen_Port"].to_int32();
    int32_t http_num = conf["HttpOption"]["Http_Num"].to_int32();
    int32_t http_thread = conf["HttpOption"]["Thread_Num"].to_int32();

    string rmq_ip = conf["Rabbitmq"]["IP"].to_cstr();
    int32_t rmq_port = conf["Rabbitmq"]["Port"].to_int32();
    string rmq_user = conf["Rabbitmq"]["Username"].to_cstr();
    string rmq_pwd = conf["Rabbitmq"]["Password"].to_cstr();

    for (int32_t i = 0; i < work_thread_num + 1; ++i)
    {
        s_vt *p_svt = (s_vt *)malloc(sizeof(s_vt));
        _svt_vec.push_back(p_svt);
        s_vt *p_res_svt = (s_vt *)malloc(sizeof(s_vt));
        _svt_response_vec.push_back(p_res_svt);
        rabbitmq_producer *producer_single = new rabbitmq_producer();
        producer_single->init_rabbitmq(rmq_ip, rmq_port, rmq_user, rmq_pwd);
        producer_single->link_rabbitmq();
        _rmq_pro_vec.push_back(producer_single);
    }

    // 启动网络事件监听
    lib_io_start(listen_port, io_thread_num, work_thread_num);

    // 启动Http服务
    libev_http_server libev_http_server_ex;
    libev_http_server_ex.http_start(http_port, http_num, http_thread, rmq_ip, rmq_port, rmq_user, rmq_pwd);

    while (1)
    {
        sleep(10);
    }

    for (int32_t i = 0; i < work_thread_num + 1; ++i)
    {
        _rmq_pro_vec[i]->unlink_rabbitmq();
    }

    return 0;
}
