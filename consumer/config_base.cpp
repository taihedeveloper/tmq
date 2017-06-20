/** $Id: config_base.cpp, 2017/03/16 11:13:30 xfy Exp $ */

#include "config_base.h"
#include "common.h"

#include <boost/typeof/typeof.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

const std::string HTTP_PREFIX = "{{#";
const std::string HTTP_SUFFIX = "}}";
const std::string MYSQL_PREFIX = "$";
const std::string MYSQL_SUFFIX = "$";

std::map<uint32_t, int32_t> http_conf_ref;
std::map<uint32_t, int32_t> db_conf_ref;

CLock                    g_unit_list_mutex;
std::list<unit_conf_t>   g_unit_list;

bool update_conf_ref(uint32_t conf_id, COMMAND_TYPE command_type, int delta);

ConfigBase::ConfigBase()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_rwlock_init(&_db_conf_lock, NULL);
    pthread_rwlock_init(&_http_conf_lock, NULL);
    pthread_rwlock_init(&_command_conf_lock, NULL);

    int32_t res = 0;
    for (int32_t i = 0; i < CONFIG_RW_LOCK_COUNT; ++i)
    {
        res = pthread_rwlock_init(&_rw_lock[i], NULL);
        if (res != 0)
        {
            TMQ_CONSUMER_FATAL("Init rwlock failed!");
            exit(1);
        }
    }
    _sql_db = new MySqlDB();
}

ConfigBase::~ConfigBase()
{
    CONFIG_SAFE_DELETE_MAP(_http_confs);
    CONFIG_SAFE_DELETE_MAP(_db_confs);
    CONFIG_SAFE_DELETE_MAP(_cmd_confs);
    for (int32_t i = 0; i < CONFIG_RW_LOCK_COUNT; ++i)
    {
        pthread_rwlock_destroy(&_rw_lock[i]);
    }
    pthread_mutex_destroy(&_lock);
    if (_sql_db != NULL)
    {
        delete _sql_db;
        _sql_db = NULL;
    }

    pthread_rwlock_destroy(&_db_conf_lock);
    pthread_rwlock_destroy(&_http_conf_lock);
    pthread_rwlock_destroy(&_command_conf_lock);
}

//加载模板配置
int32_t load_tpl_conf(std::map<uint64_t, std::vector<struct tpl_conf_t*> >& tpl_map, COMMAND_TYPE ctype,
                    MySqlDB* sql_db,
                    const std::string& sql,
                    const std::string& prefix,
                    const std::string& suffix)
{
    char* end_ptr = NULL;
    tpl_map.clear();
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }
    while (sql_db->fetch_row())
    {
        struct tpl_conf_t* conf_ptr = new struct tpl_conf_t;
        conf_ptr->_tpl_id      = strtoul(sql_db->fetch_fields_in_row(1), &end_ptr, 10);
        conf_ptr->_consumer_id = strtoul(sql_db->fetch_fields_in_row(3), &end_ptr, 10);;
        std::string str_tpl = sql_db->fetch_fields_in_row(2);
        std::set<std::string> key_set;
        std::size_t pre_pos = str_tpl.find_first_of(prefix);
        int32_t pre_len = prefix.size();
        int32_t suf_len = suffix.size();
        while (pre_pos != std::string::npos)
        {
            std::size_t suf_pos = str_tpl.find_first_of(suffix, pre_pos + pre_len);
            if (suf_pos == std::string::npos)
            {
                break;
            }
            std::string key = str_tpl.substr(pre_pos + pre_len, suf_pos - pre_pos - pre_len);
            key_set.insert(key);
            pre_pos = str_tpl.find_first_of(prefix, suf_pos + suf_len);
        }
        conf_ptr->_tpl_detail = make_pair(str_tpl, key_set);

        uint64_t command_no = strtoull(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        std::map<uint64_t, std::vector<struct tpl_conf_t*> >::iterator iter = tpl_map.find(command_no);
        if (iter == tpl_map.end())
        {
            std::vector<struct tpl_conf_t*> tpl_vec;
            tpl_vec.push_back(conf_ptr);
            tpl_map[command_no] = tpl_vec;
        }
        else
        {
            tpl_map[command_no].push_back(conf_ptr);
        }
        update_conf_ref(conf_ptr->_tpl_id, ctype, 1);
    }
    return 0;
}

void clear_tpl_conf_map(std::map<uint64_t, std::vector<struct tpl_conf_t*> >& tpl_map)
{
    for (BOOST_AUTO(iter, tpl_map.begin()); iter != tpl_map.end();)
    {
        if (iter->second.size() > 0)
        {
            std::vector<struct tpl_conf_t*>& vec = iter->second;
            CONFIG_SAFE_DELETE_VECTOR(vec);
        }
        tpl_map.erase(iter++);
    }
}

//加载command配置
int32_t load_command_conf(std::map<uint64_t, struct command_conf_t*>& cmd_confs,
                        MySqlDB* sql_db,
                        const std::string& sql,
                        std::map<uint64_t, std::vector<struct tpl_conf_t*> >& http_tpl_map,
                        std::map<uint64_t, std::vector<struct tpl_conf_t*> >& db_tpl_map)
{
    char* end_ptr = NULL;
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }
    while (sql_db->fetch_row())
    {
        struct command_conf_t* conf_ptr = new struct command_conf_t;
        conf_ptr->_command_name = sql_db->fetch_fields_in_row(1);
        int32_t command_type = atoi(sql_db->fetch_fields_in_row(2));
        if (command_type == T_HTTP)
        {
            conf_ptr->_command_type = T_HTTP;
        }
        else
        {
            conf_ptr->_command_type = T_MYSQL;
        }
        int32_t command_encode_type = atoi(sql_db->fetch_fields_in_row(3));
        if (command_encode_type == 0)
        {
            conf_ptr->_command_encode_type = ENC_MCPACK;
        }
        else
        {
            conf_ptr->_command_encode_type = ENC_JSON;
        }
        conf_ptr->_server_id = atoi(sql_db->fetch_fields_in_row(4));
        conf_ptr->_vhost = sql_db->fetch_fields_in_row(5);
        conf_ptr->_retry_counts = atoi(sql_db->fetch_fields_in_row(6));
        conf_ptr->_last_update_time = strtoull(sql_db->fetch_fields_in_row(7), &end_ptr, 10);

        uint64_t command_no = strtoull(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        std::vector<struct tpl_conf_t*> tpl_confs;
        conf_ptr->_tpl_confs = tpl_confs;
        if (command_type == T_HTTP)
        {
            std::swap(conf_ptr->_tpl_confs, http_tpl_map[command_no]);
        }
        else
        {
            std::swap(conf_ptr->_tpl_confs, db_tpl_map[command_no]);
        }
        cmd_confs[command_no] = conf_ptr;
    }

    //清理没有swap到cmd_confs里的模板配置，避免占用内存且退出时内存泄露
    clear_tpl_conf_map(http_tpl_map);
    clear_tpl_conf_map(db_tpl_map);
    return 0;
}

//加载所有模板的http配置
int32_t load_http_conf(MySqlDB* sql_db, const std::string& sql, std::map<uint32_t, struct http_conf_t*>& http_confs)
{
    char* end_ptr = NULL;
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }
    while (sql_db->fetch_row())
    {
        uint32_t tpl_id = strtoul(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        struct http_conf_t* conf_ptr = new struct http_conf_t;
        conf_ptr->_host = sql_db->fetch_fields_in_row(1);
        conf_ptr->_port = atoi(sql_db->fetch_fields_in_row(2));
        conf_ptr->_max_retrytime = atoi(sql_db->fetch_fields_in_row(3));
        conf_ptr->_last_update_time = strtoull(sql_db->fetch_fields_in_row(4), &end_ptr, 10);
        http_confs[tpl_id] = conf_ptr;
    }
    return 0;
}

//加载所有模板的mysql配置
int32_t load_db_conf(MySqlDB* sql_db, const std::string& sql, std::map<uint32_t, struct db_conf_t*>& db_confs)
{
    char* end_ptr = NULL;
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }
    while (sql_db->fetch_row())
    {
        struct db_conf_t* conf_ptr = new struct db_conf_t;

        conf_ptr->_db_name = sql_db->fetch_fields_in_row(1);
        conf_ptr->_username = sql_db->fetch_fields_in_row(2);
        conf_ptr->_password = sql_db->fetch_fields_in_row(3);
        conf_ptr->_host = sql_db->fetch_fields_in_row(4);
        conf_ptr->_port = atoi(sql_db->fetch_fields_in_row(5));
        conf_ptr->_charset = sql_db->fetch_fields_in_row(6);
        conf_ptr->_max_connection = atoi(sql_db->fetch_fields_in_row(7));
        conf_ptr->_min_connection = atoi(sql_db->fetch_fields_in_row(8));
        conf_ptr->_last_update_time = strtoull(sql_db->fetch_fields_in_row(9), &end_ptr, 10);
        uint32_t tpl_id = strtoul(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        db_confs[tpl_id] = conf_ptr;
    }
    return 0;
}


//加载command配置
int32_t add_to_command_conf(std::map<uint64_t, struct command_conf_t*>& cmd_confs,
                        MySqlDB* sql_db,
                        const std::string& sql,
                        std::map<uint64_t, std::vector<struct tpl_conf_t*> >& http_tpl_map,
                        std::map<uint64_t, std::vector<struct tpl_conf_t*> >& db_tpl_map,
                        pthread_rwlock_t* _command_rw_lock)
{
    char* end_ptr = NULL;
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }
    while (sql_db->fetch_row())
    {
        AutoRwLock lock(_command_rw_lock, T_W);
        uint64_t command_no = strtoull(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        if(cmd_confs.find(command_no) != cmd_confs.end())
        {
            continue;
        }
        struct command_conf_t* conf_ptr = new struct command_conf_t;
        conf_ptr->_command_name = sql_db->fetch_fields_in_row(1);
        int32_t command_type = atoi(sql_db->fetch_fields_in_row(2));
        if (command_type == T_HTTP)
        {
            conf_ptr->_command_type = T_HTTP;
        }
        else
        {
            conf_ptr->_command_type = T_MYSQL;
        }
        int32_t command_encode_type = atoi(sql_db->fetch_fields_in_row(3));
        if (command_encode_type == 0)
        {
            conf_ptr->_command_encode_type = ENC_MCPACK;
        }
        else
        {
            conf_ptr->_command_encode_type = ENC_JSON;
        }
        conf_ptr->_server_id = atoi(sql_db->fetch_fields_in_row(4));
        conf_ptr->_vhost = sql_db->fetch_fields_in_row(5);
        conf_ptr->_retry_counts = atoi(sql_db->fetch_fields_in_row(6));
        conf_ptr->_last_update_time = strtoull(sql_db->fetch_fields_in_row(7), &end_ptr, 10);

        unit_conf_t u;
        u._command_encode_type = conf_ptr->_command_encode_type;
        u._command_no          = command_no;
        u._vhost               = conf_ptr->_vhost;
        u._command_type        = conf_ptr->_command_type;
        u._oper_type           = ADD_COMMAND;

        g_unit_list_mutex.lock();
        g_unit_list.push_back(u);
        g_unit_list_mutex.unlock();

        std::vector<struct tpl_conf_t*> tpl_confs;
        conf_ptr->_tpl_confs = tpl_confs;
        if (command_type == T_HTTP)
        {
            std::swap(conf_ptr->_tpl_confs, http_tpl_map[command_no]);
        }
        else
        {
            std::swap(conf_ptr->_tpl_confs, db_tpl_map[command_no]);
        }
        cmd_confs[command_no] = conf_ptr;
    }

    //清理没有swap到cmd_confs里的模板配置，避免占用内存且退出时内存泄露
    clear_tpl_conf_map(http_tpl_map);
    clear_tpl_conf_map(db_tpl_map);
    return 0;
}

int32_t add_to_db_conf(MySqlDB* sql_db, const std::string& sql, std::map<uint32_t, struct db_conf_t*>& db_confs, pthread_rwlock_t *db_mutex)
{
    char* end_ptr = NULL;
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }

    while (sql_db->fetch_row())
    {
        AutoRwLock(db_mutex, T_W);
        uint32_t tpl_id = strtoul(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        if(db_confs.find(tpl_id) != db_confs.end())
        {
            continue;
        }
        struct db_conf_t* conf_ptr = new struct db_conf_t;
        conf_ptr->_db_name = sql_db->fetch_fields_in_row(1);
        conf_ptr->_username = sql_db->fetch_fields_in_row(2);
        conf_ptr->_password = sql_db->fetch_fields_in_row(3);
        conf_ptr->_host = sql_db->fetch_fields_in_row(4);
        conf_ptr->_port = atoi(sql_db->fetch_fields_in_row(5));
        conf_ptr->_charset = sql_db->fetch_fields_in_row(6);
        conf_ptr->_max_connection = atoi(sql_db->fetch_fields_in_row(7));
        conf_ptr->_min_connection = atoi(sql_db->fetch_fields_in_row(8));
        conf_ptr->_last_update_time = strtoull(sql_db->fetch_fields_in_row(9), &end_ptr, 10);
        db_confs[tpl_id] = conf_ptr;
    }
    return 0;
}

int32_t add_to_http_conf(MySqlDB* sql_db, const std::string& sql, std::map<uint32_t, struct http_conf_t*>& http_confs, pthread_rwlock_t *http_mutex)
{
    char* end_ptr = NULL;
    if (!sql_db->query(sql.c_str()))
    {
        return (-1);
    }
    if (!sql_db->use_result())
    {
        return (-1);
    }

    while (sql_db->fetch_row())
    {
        AutoRwLock(http_mutex, T_W);
        uint32_t tpl_id = strtoul(sql_db->fetch_fields_in_row(0), &end_ptr, 10);
        if(http_confs.find(tpl_id) != http_confs.end())
        {
            continue;
        }
        struct http_conf_t* conf_ptr = new struct http_conf_t;
        conf_ptr->_host = sql_db->fetch_fields_in_row(1);
        conf_ptr->_port = atoi(sql_db->fetch_fields_in_row(2));
        conf_ptr->_max_retrytime = atoi(sql_db->fetch_fields_in_row(3));
        conf_ptr->_last_update_time = strtoull(sql_db->fetch_fields_in_row(4), &end_ptr, 10);
        http_confs[tpl_id] = conf_ptr;
    }
    return 0;
}

//true :delete config, false
bool update_conf_ref(uint32_t conf_id, COMMAND_TYPE command_type, int delta)
{
    int count = 0;

    if(T_HTTP == command_type)
    {
        std::map<uint32_t, int>::iterator itor = http_conf_ref.find(conf_id);
        if(itor != http_conf_ref.end())
        {
            itor->second+=delta;
            count = itor->second;
        }
        else
        {
            http_conf_ref[conf_id] = delta;
            count = delta;
        }

        if(0 >= count)
            return true;
    }
    else if(T_MYSQL == command_type)
    {
        std::map<uint32_t, int>::iterator itor = db_conf_ref.find(conf_id);
        if(itor != db_conf_ref.end())
        {
            itor->second+=delta;
            count = itor->second;
        }
        else
        {
            db_conf_ref[conf_id] = delta;
            count = delta;
        }

        if(0 >= count)
            return true;
    }

    return false;
}

int32_t ConfigBase::load(const comcfg::Configure& conf, int32_t server_id)
{
    const comcfg::ConfigUnit& mysql_conf =  conf["MySQLConf"];
    _host = mysql_conf["Host"].to_cstr();
    _username = mysql_conf["Username"].to_cstr();
    _password = mysql_conf["Password"].to_cstr();
    _db_name = mysql_conf["DBName"].to_cstr();
    _port = mysql_conf["Port"].to_int32();
    if (!_sql_db->connect(_host.c_str(), _username.c_str(), _password.c_str(), _db_name.c_str(), _port))
    {
        TMQ_CONSUMER_FATAL("[ConfigBase]:mysql cannot connected, username:%s, password:%s, dbname:%s", _username.c_str(), _password.c_str(), _db_name.c_str())
        exit(1);
    }

    char tmp_buf[256] = { 0 };
    char tmp_buf_server_id[64] = { 0 };
    if (server_id >= 0)
    {
        sprintf(tmp_buf_server_id, " and server_id = %d", server_id);
    }
    sprintf(tmp_buf, "select command_no, command_name, command_type, command_encode_type, server_id, vhost, retry_counts, last_update_time from command_conf where delete_flag = 0%s", tmp_buf_server_id);
    const std::string sql_cmd_conf = tmp_buf;
    const std::string sql_cmd_http_conf = "select command_no, http_id, http_template, id from command_http_conf";
    const std::string sql_cmd_db_conf = "select command_no, db_id, sql_template, id from command_db_conf";
    const std::string sql_http_conf = "select id, http_host, http_port, http_max_retrytime, last_update_time from http_conf";
    const std::string sql_db_conf = "select id, db_name, db_username, db_password, db_host, db_port, db_charset, db_max_connection, db_min_connection, last_update_time from db_conf";

    //加载http模板配置
    std::map<uint64_t, std::vector<struct tpl_conf_t*> > http_tpl_map;
    int32_t ret = load_tpl_conf(http_tpl_map, T_HTTP, _sql_db, sql_cmd_http_conf,  HTTP_PREFIX, HTTP_SUFFIX);
    if (ret < 0)
    {
        _sql_db->close();
        return ret;
    }

    //加载mysql模板配置
    std::map<uint64_t, std::vector<struct tpl_conf_t*> > db_tpl_map;
    ret = load_tpl_conf(db_tpl_map, T_MYSQL, _sql_db, sql_cmd_db_conf, MYSQL_PREFIX, MYSQL_SUFFIX);
    if (ret < 0)
    {
        _sql_db->close();
        return ret;
    }

    //加载command配置
    ret = load_command_conf(_cmd_confs, _sql_db, sql_cmd_conf, http_tpl_map, db_tpl_map);
    if (ret < 0)
    {
        _sql_db->close();
        return ret;
    }

    //加载所有模板的http配置
    ret = load_http_conf(_sql_db, sql_http_conf, _http_confs);
    if (ret < 0)
    {
        _sql_db->close();
        return ret;
    }

    //加载所有模板的mysql配置
    ret = load_db_conf(_sql_db, sql_db_conf, _db_confs);
    if (ret < 0)
    {
        _sql_db->close();
        return ret;
    }

    _sql_db->close();
    return 0;
}

int32_t ConfigBase::get_command_list(std::vector<struct unit_conf_t>& cmd_list)
{
    //AutoLock mutex(&_lock);
    cmd_list.clear();
    struct unit_conf_t u;
    for (BOOST_AUTO(iter, _cmd_confs.begin()); iter != _cmd_confs.end(); ++iter)
    {
        u._command_no = iter->first;
        u._command_encode_type = iter->second->_command_encode_type;
        u._vhost = iter->second->_vhost;
        u._command_type = iter->second->_command_type;
        cmd_list.push_back(u);
    }
    return 0;
}

int32_t ConfigBase::add_single_command(const command_task& task)
{
    if (!_sql_db->connect(_host.c_str(), _username.c_str(), _password.c_str(), _db_name.c_str(), _port))
    {
        TMQ_CONSUMER_FATAL("[ConfigBase]:mysql cannot connected, username:%s, password:%s, dbname:%s", _username.c_str(), _password.c_str(), _db_name.c_str())
        exit(1);
    }

    std::map<uint64_t, std::vector<struct tpl_conf_t*> > http_tpl_map, sql_tpl_map;
    if(T_HTTP == task.command_type)
    {
        boost::format f("select command_no, http_id, http_template, id from command_http_conf where command_no = %1%");
        f % task.command_no;
        int32_t ret = load_tpl_conf(http_tpl_map, T_HTTP, _sql_db, f.str(), HTTP_PREFIX, HTTP_SUFFIX);

        boost::format http_format("select id, http_host, http_port, http_max_retrytime, last_update_time from  http_conf where id in (select distinct http_id from command_http_conf where command_no = %1%)");
        http_format % task.command_no;
        add_to_http_conf(_sql_db, http_format.str(), _http_confs, &_http_conf_lock);
    }
    else if(T_MYSQL == task.command_type)
    {
        boost::format f("select command_no, db_id, sql_template, id from command_db_conf where command_no = %1%");
        f % task.command_no;
        int32_t ret = load_tpl_conf(sql_tpl_map, T_MYSQL, _sql_db, f.str(),  MYSQL_PREFIX, MYSQL_SUFFIX);

        boost::format sql_format("select id, db_name, db_username, db_password, db_host, db_port, db_charset, db_max_connection, db_min_connection, last_update_time from db_conf where id in \
                (select distinct db_id from command_db_conf where command_no = %1%)");
        sql_format % task.command_no;
        add_to_db_conf(_sql_db, sql_format.str(),  _db_confs, &_db_conf_lock);
    }
    else
    {
        return -1;
    }

    boost::format cmd_format("select command_no, command_name, command_type, command_encode_type, server_id, vhost, retry_counts, last_update_time from command_conf where delete_flag = 0 and command_no = %1% and server_id = %2%");
    cmd_format % task.command_no;
    cmd_format % task.server_id;

    int ret = add_to_command_conf(_cmd_confs, _sql_db, cmd_format.str(), http_tpl_map, sql_tpl_map, &_command_conf_lock);
    if(ret < 0)
    {
        _sql_db->close();
        return ret;
    }

    _sql_db->close();
    return 0;
}

int32_t ConfigBase::add_single_consumer(const command_task& task)
{
    TMQ_CONSUMER_DEBUG("[add_single_consumer]");
    if (!_sql_db->connect(_host.c_str(), _username.c_str(), _password.c_str(), _db_name.c_str(), _port))
    {
        TMQ_CONSUMER_FATAL("[ConfigBase]:mysql cannot connected, username:%s, password:%s, dbname:%s", _username.c_str(), _password.c_str(), _db_name.c_str())
        exit(1);
    }

    std::map<uint64_t, std::vector<struct tpl_conf_t*> > http_tpl_map, sql_tpl_map;
    if(T_HTTP == task.command_type)
    {
        boost::format f("select command_no, http_id, http_template, id from command_http_conf where id = %1%");
        f % task.consumer_id;
        int32_t ret = load_tpl_conf(http_tpl_map, T_HTTP, _sql_db, f.str(), HTTP_PREFIX, HTTP_SUFFIX);

        boost::format http_format("select id, http_host, http_port, http_max_retrytime, last_update_time from http_conf where id in (select http_id from command_http_conf where id = %1%)");
        http_format % task.consumer_id;
        add_to_http_conf(_sql_db, http_format.str(), _http_confs, &_http_conf_lock);

        AutoRwLock(&_command_conf_lock, T_W);
        std::map<uint64_t, struct command_conf_t*>::iterator iter = _cmd_confs.find(task.command_no);
        std::map<uint64_t, std::vector<struct tpl_conf_t*> >::iterator iter_tmp = http_tpl_map.find(task.command_no);
        if((iter != _cmd_confs.end()) && (iter_tmp != http_tpl_map.end()))
        {
            iter->second->_tpl_confs.insert(iter->second->_tpl_confs.end(), iter_tmp->second.begin(), iter_tmp->second.end());
            TMQ_CONSUMER_DEBUG("[load_tpl_conf size:%d, vector size %d, tpl_confs size:%d]", http_tpl_map.size(), iter_tmp->second.size(), iter->second->_tpl_confs.size());
            return 0;
        }
    }
    else if(T_MYSQL == task.command_type)
    {
        boost::format f("select command_no, db_id, sql_template, id from command_db_conf where  where id = %1%");
        f % task.consumer_id;
        int32_t ret = load_tpl_conf(sql_tpl_map, T_MYSQL, _sql_db, f.str(),  MYSQL_PREFIX, MYSQL_SUFFIX);

        boost::format sql_format("select id, db_name, db_username, db_password, db_host, db_port, db_charset, db_max_connection, db_min_connection, last_update_time from db_conf where id in (select distinct db_id from command_db_conf where id = %1%");
        sql_format % task.consumer_id;
        add_to_db_conf(_sql_db, sql_format.str(),  _db_confs, &_db_conf_lock);

        AutoRwLock(&_command_conf_lock, T_W);
        std::map<uint64_t, struct command_conf_t*>::iterator iter = _cmd_confs.find(task.command_no);
        std::map<uint64_t, std::vector<struct tpl_conf_t*> >::iterator iter_tmp = sql_tpl_map.find(task.command_no);
        if((iter != _cmd_confs.end()) && (iter_tmp != sql_tpl_map.end()))
        {
            iter->second->_tpl_confs.insert(iter->second->_tpl_confs.end(), iter_tmp->second.begin(), iter_tmp->second.end());
            return 0;
        }
    }
    else
    {
        return -1;
    }

    _sql_db->close();
    return 0;
}

int32_t ConfigBase::delete_single_command(const command_task& task)
{
    std::map<uint64_t, struct command_conf_t*>::iterator itor = _cmd_confs.find(task.command_no);

    if(itor == _cmd_confs.end())
    {
        return 0;
    }

    BOOST_FOREACH(tpl_conf_t *c, itor->second->_tpl_confs)
    {
        command_task consumer_task;
        consumer_task.consumer_id  = c->_consumer_id;
        consumer_task.command_no   = task.command_no;
        consumer_task.oper_type    = DELETE_CONSUMER;
        consumer_task.command_type = task.command_type;

        delete_single_consumer(consumer_task);
    }

    {
        AutoRwLock(&_command_conf_lock, T_W);
        if(NULL != itor->second)
        {
//            unit_conf_t u;
//            u._command_encode_type = itor->second->_command_encode_type;
//            u._command_no = task.command_no;
//            u._vhost      = itor->second->_vhost;
//            u.oper_type   = DELETE_COMMAND;
//            g_unit_list_mutex.lock();
//            g_unit_list.push_back(u);
//            g_unit_list_mutex.unlock();

            delete itor->second;
            itor->second = NULL;
            _cmd_confs.erase(itor);
        }
    }
    return 0;
}

int32_t ConfigBase::delete_single_consumer(const command_task& task)
{
    uint32_t tpl_id;
    {
        AutoRwLock(&_command_conf_lock, T_W);
        struct command_conf_t * command_conf = _cmd_confs[task.command_no];
        std::vector<struct tpl_conf_t*>::iterator iter = command_conf->_tpl_confs.begin();
        for(; iter != command_conf->_tpl_confs.end(); iter++)
        {
            if(task.consumer_id == (*iter)->_consumer_id)
            {
                tpl_id = (*iter)->_tpl_id;
                iter = command_conf->_tpl_confs.erase(iter);
                break;
            }
        }
    }

    if(T_HTTP == task.command_type)
    {
        AutoRwLock(&_http_conf_lock, T_W);
        std::map<uint32_t, struct http_conf_t*>::iterator itor = _http_confs.find(task.consumer_id);
        if(itor == _http_confs.end())
        {
            return 0;
        }

        if(update_conf_ref(tpl_id, task.command_type, -1))
        {
            delete itor->second;
            itor->second = NULL;
            _http_confs.erase(itor);
        }
    }
    else if(T_MYSQL == task.command_type)
    {
        AutoRwLock(&_db_conf_lock, T_W);
        std::map<uint32_t, struct db_conf_t*>::iterator itor = _db_confs.find(task.consumer_id);
        if(itor == _db_confs.end())
        {
            return 0;
        }

        if(update_conf_ref(tpl_id, task.command_type, -1))
        {
            delete itor->second;
            itor->second = NULL;
            _db_confs.erase(itor);
        }
    }
    return 0;
}

int32_t ConfigBase::get_message(struct result_conf_t& result, mc_pack_t* data)
{
    return get_message(result, data, NULL);
}

int32_t ConfigBase::get_message(struct result_conf_t& result, json_object* data)
{
    return get_message(result, NULL, data);
}

int32_t ConfigBase::get_message(struct result_conf_t& result, mc_pack_t* data_mcpack, json_object* data_json)
{
    //AutoLock mutex(&_lock);
    if ((data_mcpack == NULL && data_json == NULL) ||
        (data_mcpack != NULL && data_json != NULL))
    {
        return (-1);
    }

    COMMAND_ENCODE_TYPE _encode_type = ENC_MCPACK;

    uint64_t command_no = 0;
    if (data_mcpack != NULL)
    {
        mc_uint64_t mc_command_no;
        MC_PACK_GET_UINT64(data_mcpack, "command_no", &mc_command_no, 0);
        if (mc_command_no == 0)
        {
            return (-1);
        }
        command_no = (uint64_t)mc_command_no;
        _encode_type = ENC_MCPACK;

    }
    else if(data_json != NULL)
    {
        double json_command_no = 0.0;
        JSON_GET_DOUBLE(data_json, "command_no", &json_command_no, 0.0);
        if (json_command_no == 0)
        {
            return (-1);
        }
        command_no = (uint64_t)json_command_no;
        _encode_type = ENC_JSON;
    }

    int32_t mod = (int32_t)(command_no % CONFIG_RW_LOCK_COUNT);
    AutoRwLock lock(&_rw_lock[mod], T_R);

    std::map<uint64_t, struct command_conf_t*>::iterator iter_cmd = _cmd_confs.find(command_no);
    if (iter_cmd == _cmd_confs.end())
    {
        return (-1);
    }

    result._command_no = command_no;
    result._command_type = iter_cmd->second->_command_type;
    result._retry_counts = iter_cmd->second->_retry_counts;
    std::string prefix = "";
    std::string suffix = "";
    if (result._command_type == T_HTTP)
    {
        prefix = HTTP_PREFIX;
        suffix = HTTP_SUFFIX;
    }
    else if (result._command_type == T_MYSQL)
    {
        prefix = MYSQL_PREFIX;
        suffix = MYSQL_SUFFIX;
    }
    else
    {
        return (-1);
    }

    const char* tmp_ptr = NULL;
    for (BOOST_AUTO(iter_tpl, iter_cmd->second->_tpl_confs.begin()); iter_tpl != iter_cmd->second->_tpl_confs.end(); ++iter_tpl)
    {
        std::string str_tpl = (*iter_tpl)->_tpl_detail.first;
        std::string str_real = str_tpl;
        for (BOOST_AUTO(iter_key, (*iter_tpl)->_tpl_detail.second.begin()); iter_key != (*iter_tpl)->_tpl_detail.second.end(); ++iter_key)
        {
            char ch_value[1024] = { 0 };
            if(ENC_JSON == _encode_type)
            {
                const char *p = NULL;
                JSON_GET_STR(p, data_json, iter_key->c_str(), ch_value);
            }
            else if(ENC_MCPACK == _encode_type)
            {
                get_mc_pack_str_value(data_mcpack, iter_key->c_str(), ch_value, 1024);
            }
            std::string str_key = prefix + (*iter_key) + suffix;
            str_replace(str_real, str_key, ch_value);
        }

        std::pair<void*, std::string> tpl_pair;
        if (result._command_type == T_HTTP)
        {
            AutoRwLock db_lock(&_http_conf_lock, T_R);
            std::map<uint32_t, struct http_conf_t*>::iterator iter_http = _http_confs.find((*iter_tpl)->_tpl_id);
            if (iter_http == _http_confs.end())
            {
                TMQ_CONSUMER_NOTICE("can not found http id %u", (*iter_tpl)->_tpl_id);
                return (-1);
            }
            tpl_pair = make_pair(iter_http->second, str_real);
        }
        else if (result._command_type == T_MYSQL)
        {
            AutoRwLock db_lock(&_db_conf_lock, T_R);
            std::map<uint32_t, struct db_conf_t*>::iterator iter_db = _db_confs.find((*iter_tpl)->_tpl_id);
            if (iter_db == _db_confs.end())
            {
                TMQ_CONSUMER_NOTICE("can not found http id %u", (*iter_tpl)->_tpl_id);
                return (-1);
            }
            tpl_pair = make_pair(iter_db->second, str_real);
        }
        result._tpl_confs.push_back(tpl_pair);
    }

    return 0;
}
