#ifndef __MYSQLDB_H__
#define __MYSQLDB_H__

#include <mysql.h>

class MySqlDB
{
public:
    MySqlDB()
    {
        _handle_mysql = NULL;
        _mysql_res = NULL;
        _mysql_row = NULL;
        _handle_mysql = mysql_init(NULL);
        if (_handle_mysql == NULL)
        {
            exit(1);
        }
        my_bool reconn = 1;
        mysql_options(_handle_mysql, MYSQL_OPT_RECONNECT, (const char*)&reconn);
    }

    ~MySqlDB()
    {
        free_result();
        close();
    }

    void close()
    {
       if (_handle_mysql != NULL)
        {
            _mysql_res = NULL;
            _mysql_row = NULL;
            mysql_close(_handle_mysql);
            _handle_mysql = NULL;
        }
    }

    bool connect(const char* host, const char* user, const char* passwd, const char* db, uint32_t port, const char* unix_socket = NULL, uint64_t clientflag = 0)
    {
        if (host == NULL || port == 0)
        {
            return false;
        }
        if (_handle_mysql == NULL)
        {
            _handle_mysql = mysql_init(NULL);
            my_bool reconn = 1;
            mysql_options(_handle_mysql, MYSQL_OPT_RECONNECT, (const char*)&reconn);
        }
        return (mysql_real_connect(_handle_mysql, host, user, passwd, db, port, unix_socket, clientflag) != NULL);
    }

    bool query(const char* sql, int32_t length = -1)
    {
        if (length == -1 && sql != NULL)
        {
            length = strlen(sql);
        }

        return (mysql_real_query(_handle_mysql, sql, (uint64_t)length) == 0);
    }

    bool store_result()
    {
        _mysql_res = mysql_store_result(_handle_mysql);
        return (_mysql_res != NULL);
    }

    bool use_result()
    {
        _mysql_res = mysql_use_result(_handle_mysql);
        return (_mysql_res != NULL);
    }

    uint64_t get_row_count()
    {
        return (uint64_t)mysql_num_rows(_mysql_res);
    }

    uint32_t get_field_count()
    {
        return (uint32_t)mysql_num_fields(_mysql_res);
    }

    bool fetch_row()
    {
        _mysql_row = mysql_fetch_row(_mysql_res);
        return (_mysql_row != NULL);
    }

    const char* fetch_fields_in_row(int field_index)
    {
        if (field_index < 0 || (uint32_t)field_index >= get_field_count() || _mysql_row == NULL)
        {
            return NULL;
        }
        else
        {
            return (const char*)_mysql_row[field_index];
        }
    }

    void free_result()
    {
        if (_mysql_res != NULL)
        {
            mysql_free_result(_mysql_res);
            _mysql_res = NULL;
            _mysql_row = NULL;
        }
    }

    int32_t get_last_error_no()
    {
        return mysql_errno(_handle_mysql);
    }

    const char* get_last_error()
    {
        if (mysql_errno(_handle_mysql))
        {
            return mysql_error(_handle_mysql);
        }
        return "";
    }

private:
    MYSQL* _handle_mysql;
    MYSQL_RES* _mysql_res;
    MYSQL_ROW _mysql_row;
};

#endif // __MYSQLDB_H__
