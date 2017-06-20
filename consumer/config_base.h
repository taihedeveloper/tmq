#ifndef __CONFIG_BASE_HEAD_FILE__
#define __CONFIG_BASE_HEAD_FILE__

#include "utils.h"
#include "mc_pack.h"
#include "json/json.h"
#include "json/json_tokener.h"
#include "common.h"
#include "mysql_db.h"
#include "consumer.h"

//模板配置
typedef struct tpl_conf_t
{
    uint32_t _tpl_id;
    uint32_t _consumer_id;
    std::pair<std::string, std::set<std::string> > _tpl_detail;
}tpl_conf_t;

//模板基本配置
typedef struct command_conf_t
{
public:
    ~command_conf_t()
    {
        CONFIG_SAFE_DELETE_VECTOR(_tpl_confs);
    }

public:
    std::string _command_name;
    COMMAND_TYPE _command_type;
    COMMAND_ENCODE_TYPE _command_encode_type;
    int32_t _server_id;
    std::string _vhost;
    uint64_t _last_update_time;
    uint32_t _retry_counts;
    std::vector<struct tpl_conf_t*> _tpl_confs; //根据_command_type来判断是http还是mysql模板
}command_conf_t;


//返回给调用层用来遍历的数据结构
typedef struct unit_conf_t
{
    unit_conf_t():_command_no(0ULL),
            _command_encode_type(ENC_MCPACK),
            _vhost(""),
            _oper_type(ADD_COMMAND),
            _command_type(T_MYSQL)
    {
    }
    uint64_t _command_no;
    COMMAND_ENCODE_TYPE _command_encode_type;
    std::string _vhost;
    COMMAND_OPERATOR_TYPE _oper_type;
    COMMAND_TYPE _command_type;
}unit_conf_t;

//返回给调用层的数据结构
typedef struct result_conf_t
{
    uint64_t _command_no;
    COMMAND_TYPE _command_type;
    uint32_t    _retry_counts;
    std::vector<std::pair<void*, std::string> > _tpl_confs; //根据_command_type来判断是http还是mysql模板
}result_conf_t;

const int32_t CONFIG_RW_LOCK_COUNT = 128;

class ConfigBase
{
public:
    ConfigBase();
    virtual ~ConfigBase();

public:
    virtual int32_t load(const comcfg::Configure& conf, int32_t server_id = -1);
    virtual int32_t get_command_list(std::vector<struct unit_conf_t>& cmd_list);
    virtual int32_t get_message(struct result_conf_t& result, mc_pack_t* data);
    virtual int32_t get_message(struct result_conf_t& result, json_object* data);
public:
    virtual int32_t add_single_command(const command_task& task);
    virtual int32_t delete_single_command(const command_task& task);
    virtual int32_t add_single_consumer(const command_task& task);
    virtual int32_t delete_single_consumer(const command_task& task);


private:
    int32_t get_message(struct result_conf_t& result, mc_pack_t* data_mcpack, json_object* data_json);

protected:
    //互斥锁，锁整个command_no列表
    pthread_mutex_t _lock;
    //互斥锁，锁整个db_conf列表
    pthread_rwlock_t _db_conf_lock;
    //互斥锁，锁整个http_conf列表
    pthread_rwlock_t _http_conf_lock;
    //互斥锁，锁整个command_conf列表
    pthread_rwlock_t _command_conf_lock;
    //读写锁，针对command_no打散的锁
    pthread_rwlock_t _rw_lock[CONFIG_RW_LOCK_COUNT];
    //command_no到模板配置的映射
    std::map<uint64_t, struct command_conf_t*> _cmd_confs;
    //http配置id到http配置详情的映射
    std::map<uint32_t, struct http_conf_t*> _http_confs;
    //mysql配置id到mysql配置详情的映射
    std::map<uint32_t, struct db_conf_t*> _db_confs;
    //数据库对象
    MySqlDB* _sql_db;
protected:
    std::string _username;
    std::string _password;
    std::string _db_name;
    std::string _host;
    int32_t     _port;
};

#endif
