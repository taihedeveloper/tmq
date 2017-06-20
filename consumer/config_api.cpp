/** $Id: config_api.cpp, 2017/03/17 11:13:30 xfy Exp $ */

#include "config_api.h"
#include "common.h"

extern CLock              g_command_task_list_mutex;
extern std::list<command_task>   g_command_task_list;

ConfigApi::ConfigApi():
 is_run(true)
{
    _conf_base = new ConfigBase();
    if (_conf_base == NULL)
    {
        TMQ_CONSUMER_FATAL("new config failed!");
        exit(1);
    }
}

ConfigApi::~ConfigApi()
{
    is_run = false;
    _thread_group.join_all();

    if (_conf_base != NULL)
    {
        delete _conf_base;
        _conf_base = NULL;
    }
}

int32_t ConfigApi::load(const comcfg::Configure& conf, int32_t server_id)
{
    _conf_base->load(conf, server_id);
    _thread_group.create_thread(boost::bind(&ConfigApi::config_task_thread, this));
    return 0;
}

int32_t ConfigApi::get_message(struct result_conf_t& result, const char* buf, const int32_t& buf_len, const COMMAND_ENCODE_TYPE encode_type)
{
    int32_t ret = 0;
    if (encode_type == ENC_MCPACK)
    {
        //mcpack
        char tmp_buf[DEFAULT_MC_PACK_LENGTH] = { 0 };
        mc_pack_t* data = mc_pack_open_r(buf, DEFAULT_MC_PACK_LENGTH, tmp_buf, sizeof(tmp_buf));
        ret = MC_PACK_PTR_ERR(data);
        if (0 != ret)
        {
            TMQ_CONSUMER_FATAL("get_message mcpack data pointer is error...");
            return (-1);
        }
        ret = mc_pack_valid(data);
        if (1 != ret)
        {
            TMQ_CONSUMER_FATAL("get_message mcpack data is error...");
            return (-1);
        }

        ret = _conf_base->get_message(result, data);

        mc_pack_close(data);
    }
    else if (encode_type == ENC_JSON)
    {
        //json
        json_object* data = json_tokener_parse(buf);
        if (is_error(data))
        {
            TMQ_CONSUMER_FATAL("get_message data json error:%u, json:%s", is_error(data), buf);
            return (-1);
        }
        if(!json_object_is_type(data, json_type_object))
        {
            TMQ_CONSUMER_FATAL("get_message data type json error, json:%s", buf);
            json_object_put(data);
            return (-1);
        }

        ret = _conf_base->get_message(result, data);

        json_object_put(data);
    }
    else
    {
        TMQ_CONSUMER_FATAL("get_message data type is error...");
        return (-1);
    }
    if (0 != ret)
    {
        return (-1);
    }
    return 0;
}

int32_t ConfigApi::get_command_list(std::vector<struct unit_conf_t>& cmd_list)
{
    return _conf_base->get_command_list(cmd_list);
}

void ConfigApi::config_task_thread()
{
    while(is_run)
    {
        g_command_task_list_mutex.lock();

        if(g_command_task_list.empty())
        {
            g_command_task_list_mutex.unlock();
            sleep(1);
            continue;
        }

        std::list<command_task> task_list;
        g_command_task_list.swap(task_list);
        g_command_task_list_mutex.unlock();

        std::list<command_task>::iterator itor = task_list.begin();
        for(; itor != task_list.end(); itor++)
        {
            switch (itor->oper_type) {
                case ADD_COMMAND:
                {
                    _conf_base->add_single_command(*itor);
                }
                break;
                case DELETE_COMMAND:
                {
                    _conf_base->delete_single_command(*itor);
                }
                break;
                case ADD_CONSUMER:
                {
                    _conf_base->add_single_consumer(*itor);
                }
                break;
                case DELETE_CONSUMER:
                {
                    _conf_base->delete_single_consumer(*itor);
                }
                break;
                default:
                    break;
            }
        }
    }
}

