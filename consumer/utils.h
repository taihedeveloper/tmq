#ifndef __UTILS_HEAD_FILE__
#define __UTILS_HEAD_FILE__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <pthread.h>

#define CONFIG_SAFE_DELETE_MAP(m) do { \
    for (BOOST_AUTO(iter, m.begin()); iter != m.end();) \
    { \
        if (iter->second != NULL) \
        { \
            delete (iter->second); \
            iter->second = NULL; \
        } \
        m.erase(iter++); \
    } \
} while (0);

#define CONFIG_SAFE_DELETE_VECTOR(vec) do { \
    for (BOOST_AUTO(iter, vec.begin()); iter != vec.end();) \
    { \
        if (*iter != NULL) \
        { \
            delete (*iter); \
            *iter = NULL; \
            iter = vec.erase(iter); \
        } \
    } \
} while (0);

class AutoLock
{
public:
    inline AutoLock(pthread_mutex_t* mutex)
    {
        _mutex = mutex;
        pthread_mutex_lock(_mutex);
    }

    inline ~AutoLock()
    {
        pthread_mutex_unlock(_mutex);
    }

private:
    pthread_mutex_t* _mutex;
};

enum RW_TYPE
{
    T_R = 1,
    T_W = 2,
};

class AutoRwLock
{
public:
    inline AutoRwLock(pthread_rwlock_t* rw_lock, RW_TYPE t)
    {
        _rw_lock = rw_lock;
        if (t == T_R)
        {
            pthread_rwlock_rdlock(_rw_lock);
        }
        else if (t == T_W)
        {
            pthread_rwlock_wrlock(_rw_lock);
        }
    }

    inline ~AutoRwLock()
    {
        pthread_rwlock_unlock(_rw_lock);
    }

private:
    pthread_rwlock_t* _rw_lock;
};

static void str_replace(std::string& str_base, std::string src, std::string dest)
{
    std::string::size_type pos = 0;
    std::string::size_type src_len = src.size();
    std::string::size_type dest_len = dest.size();
    pos = str_base.find(src, pos);
    while ((pos != std::string::npos))
    {
        str_base.replace(pos, src_len, dest);
        pos = str_base.find(src, (pos + dest_len));
    }
}

#endif
