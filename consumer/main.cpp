#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "config_api.h"
#include "rabbitmq_consumer.h"

#include <io_lib.h>

#include <Configure.h>
#include <ConfigUnit.h>
#include <ConfigReloader.h>

#include "../http_server/http_server.h"

namespace
{
    const char* CONFIG_DIR = "./conf";
    const char* CONFIG_FILE = "tmq_consumer.conf";
    const char* LIBIO_CONF_SEG = "IOLibOption";
}

#define DEFAULT_MCPACK_LENGTH 1024*1024*1

typedef struct _Svr_response_t
{
    char buf[DEFAULT_MCPACK_LENGTH];
    char tmpbuf[DEFAULT_MCPACK_LENGTH];
    t_nshead_t head;
    mc_pack_t *data;
} s_vt;


CLock              svt_list_mutex;
std::list<s_vt*>   svt_list;

CLock                     g_command_task_list_mutex;
std::list<command_task>   g_command_task_list;

extern CLock                    g_unit_list_mutex;
extern std::list<unit_conf_t>   g_unit_list;

const char * signal_str[] =
        {       "other", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT/SIGIOT",
                "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM",
                "SIGTERM", "SIGSTKFLT", "SIGCHLD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU",
                "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO", "SIGPWR", "SIGSYS"
        };

void signal_catch(int sig)
{
    TMQ_CONSUMER_NOTICE("Receive signal:%s", signal_str[sig]);

    if (    sig == SIGINT ||
            sig == SIGHUP ||
            sig == SIGQUIT ||
            sig == SIGILL ||
            sig == SIGSEGV ||
            sig == SIGPWR ||
            sig == SIGSTKFLT ||
            sig == SIGFPE ||
            sig == SIGTERM)
    {
        exit(-1);
    }

    if(sig ==  SIGUSR1 )
    {
        unit_conf_t u;
        u._oper_type = DELETE_ALL;

        g_unit_list_mutex.lock();
        g_unit_list.push_back(u);
        g_unit_list_mutex.unlock();
    }
}

void setup_signal(void)
{
    struct sigaction act;
    act.sa_handler = signal_catch;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGILL, &act, NULL);
    sigaction(SIGTRAP, &act, NULL);
    sigaction(SIGABRT, &act, NULL);
    sigaction(SIGTRAP, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGKILL, &act, NULL);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGSTKFLT, &act, NULL);
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGCONT, &act, NULL);
    sigaction(SIGSTOP, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);
    sigaction(SIGTTIN, &act, NULL);
    sigaction(SIGTTOU, &act, NULL);
    sigaction(SIGURG, &act, NULL);
    sigaction(SIGXCPU, &act, NULL);
    sigaction(SIGXFSZ, &act, NULL);
    sigaction(SIGVTALRM, &act, NULL);
//    sigaction(SIGPROF, &act, NULL);
    sigaction(SIGWINCH, &act, NULL);
    sigaction(SIGIO, &act, NULL);
    sigaction(SIGPWR, &act, NULL);
    sigaction(SIGSYS, &act, NULL);
}



s_vt* get_a_svt()
{
    s_vt *p_svt =NULL;
    int svt_list_size=0;

    svt_list_mutex.lock();

    svt_list_size = svt_list.size();
    if(svt_list_size > 0)
    {
        p_svt = svt_list.front();
        svt_list.pop_front();
    }
    else
    {
        p_svt = (s_vt*)malloc(sizeof(s_vt));
        assert(p_svt!=NULL);
    }

    svt_list_mutex.unlock();

    return p_svt;

}


int free_a_svt(s_vt * p_svt)
{
    assert(p_svt!=NULL);
    svt_list_mutex.lock();
    svt_list.push_back(p_svt);
    svt_list_mutex.unlock();
    return 1;
}

int work_func(const uint32_t peer_id,const uint32_t thead_idx,const char * buf,const int len)
{
    s_vt *request = get_a_svt();
    memcpy(&request->head,buf,sizeof(request->head));
    memcpy(request->buf,buf+sizeof(request->head),len-sizeof(request->head));
    request->data = mc_pack_open_r(request->buf, DEFAULT_MCPACK_LENGTH,request->tmpbuf, sizeof(request->tmpbuf));
    int ret = MC_PACK_PTR_ERR(request->data);
    if (0 != ret)
    {
       free_a_svt(request);
       return -1;
    }
    ret = mc_pack_valid(request->data);
    if ( 1 != ret )
    {
       free_a_svt(request);
       return -1;
    }

    TMQ_CONSUMER_DEBUG("request head is %u", request->head.id);

    if(request->head.id == ADD_COMMAND)
    {
        mc_uint64_t command_no;
        mc_uint32_t server_id;
        mc_uint32_t command_type;
        const char * p = NULL;
        MC_PACK_GET_UINT64(request->data, "command_no", &command_no, 0);
        MC_PACK_GET_UINT32(request->data, "server_id", &server_id, 0);
        MC_PACK_GET_UINT32(request->data, "command_type", &command_type, 0);

        command_task task;
        task.command_no = command_no;
        task.server_id  = server_id;
        task.oper_type  = (COMMAND_OPERATOR_TYPE)request->head.id;
        task.command_type = (COMMAND_TYPE)command_type;

        g_command_task_list_mutex.lock();
        g_command_task_list.push_back(task);
        g_command_task_list_mutex.unlock();
    }
    else if(request->head.id == DELETE_COMMAND)
    {
        mc_uint64_t command_no;
        mc_uint32_t command_type;
        unit_conf_t u;
        MC_PACK_GET_UINT64(request->data, "command_no", &command_no, 0);
        MC_PACK_GET_UINT32(request->data, "command_type", &command_type, 0);
        u._command_no = command_no;
        u._oper_type = DELETE_COMMAND;
        u._command_type = (COMMAND_TYPE)command_type;

        g_unit_list_mutex.lock();
        g_unit_list.push_back(u);
        g_unit_list_mutex.unlock();
    }
    else if(request->head.id == ADD_CONSUMER || request->head.id == DELETE_CONSUMER)
    {
        mc_uint64_t command_no;
        mc_uint32_t server_id;
        int         consumer_id;
        int         consumer_type;
        const char * p = NULL;
        MC_PACK_GET_UINT64(request->data, "command_no", &command_no, 0);
        MC_PACK_GET_UINT32(request->data, "server_id", &server_id, 0);
        MC_PACK_GET_INT32(request->data, "command_type", &consumer_type, 0);
        MC_PACK_GET_INT32(request->data, "consumer_id", &consumer_id, 0);

        command_task task;
        task.command_no = command_no;
        task.server_id  = server_id;
        task.oper_type  = (COMMAND_OPERATOR_TYPE)request->head.id;
        task.command_type = (COMMAND_TYPE)consumer_type;
        task.consumer_id = consumer_id;

        g_command_task_list_mutex.lock();
        g_command_task_list.push_back(task);
        g_command_task_list_mutex.unlock();
    }
    else if(request->head.id == SKIP_COMMAND)
    {
        mc_uint64_t command_no;
        mc_uint32_t command_type;
        unit_conf_t u;
        MC_PACK_GET_UINT64(request->data, "command_no", &command_no, 0);
        MC_PACK_GET_UINT32(request->data, "command_type", &command_type, 0);
        u._command_no = command_no;
        u._oper_type = SKIP_COMMAND;
        u._command_type = (COMMAND_TYPE)command_type;

        g_unit_list_mutex.lock();
        g_unit_list.push_back(u);
        g_unit_list_mutex.unlock();
    }

    s_vt *response = request;
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
    free_a_svt(request);
    return 1;
}

int cscbk(const std::string& ip_port_str, int state)
{

}

int print_func(const  int print_level, const char* p_info)
{

  return 1;
}

void libio_start(const comcfg::ConfigUnit& conf)
{
    int listen_port = conf[LIBIO_CONF_SEG]["Listen_Port"].to_int32();
    int io_thread_num = conf[LIBIO_CONF_SEG]["IOThreadNum"].to_int32();
    int work_thread_num = conf[LIBIO_CONF_SEG]["WorkThreadNum"].to_int32();

    libio_create_listen_socket("0.0.0.0", listen_port);
    libio_set_io_thread(io_thread_num);
    libio_set_work_thread(work_thread_num, work_func);
    libio_set_print_callback(print_func);
    libio_set_peer_timeout(600000);
    TMQ_CONSUMER_NOTICE("[libio_start port:%d]", listen_port);
    libio_init_and_start();
}

int main(int argc, char const *const *argv)
{
    comcfg::Configure conf;

    if (-1 == conf.load(CONFIG_DIR, CONFIG_FILE, NULL))
    {
        std::cerr << "load config error." << std::endl;
        return -1;
    }

    try
    {
        //初始化日志
        if (-1 == comlog_init(conf["Log"]))
        {
            std::cerr << "comlog_init log error." << std::endl;
            return -1;
        }
    }
    catch (...)
    {
        std::cerr << "Log config error" << std::endl;
        return -1;
    }

    RabbitmqConsumer *r = new RabbitmqConsumer();
    r->Init(conf);
    r->Start();
    libio_start(conf);

    while (1)
    {
        sleep(10);
    }

    delete r;
    return 0;
}
