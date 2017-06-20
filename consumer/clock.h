#ifndef BTLOCK_H
#define BTLOCK_H

#include <pthread.h>

class CLock
{
	private:
		pthread_mutex_t	m_mutex;
	public:
		CLock();
		~CLock();
		void lock(){ pthread_mutex_lock(&m_mutex); }
		int trylock() { return pthread_mutex_trylock(&m_mutex);}
		void unlock(){ pthread_mutex_unlock(&m_mutex); }
	private:
		CLock(const CLock & lock);
		CLock & operator=(const CLock & lock);
};

inline CLock::CLock()
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&m_mutex,&mutexattr);

	pthread_mutexattr_destroy(&mutexattr);
}

inline CLock::~CLock()
{
	pthread_mutex_destroy(&m_mutex);
}

class CAutoLock
{
	private:
		CLock * m_pLock;
	public:
		inline CAutoLock(CLock * pLock) {m_pLock = pLock; m_pLock->lock();}
		inline ~CAutoLock() {m_pLock->unlock();}
	private:
		CAutoLock(const CAutoLock & lock);
		CAutoLock & operator=(const CAutoLock & lock);

};

#endif

