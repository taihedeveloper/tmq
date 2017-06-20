#ifndef __CONFIG_API_HEAD_FILE__
#define __CONFIG_API_HEAD_FILE__
#include <boost/thread.hpp>
#include "config_base.h"

class ConfigApi
{
public:
    ConfigApi();
    virtual ~ConfigApi();
public:
    int32_t load(const comcfg::Configure& conf, int32_t server_id = -1);
    int32_t get_message(struct result_conf_t& result, const char* buf, const int32_t& buf_len, const COMMAND_ENCODE_TYPE encode_type);
    int32_t get_command_list(std::vector<struct unit_conf_t>& cmd_list);
private:
    void config_task_thread();
private:
    ConfigBase* _conf_base;
    boost::thread_group _thread_group;
    bool         is_run;
};

#endif
