/*
 * httpserver.h
 *
 *  Created on: Apr 27, 2015
 *      Author: root
 */

#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#include <boost/noncopyable.hpp>

#include "io_service_poll.h"
#include "session.h"

#include <string>
#include <vector>
#include <list>

class http_server : public boost::noncopyable
{
public:
	http_server(const std::string& address, uint16_t port, uint16_t thread_num);
	virtual ~http_server();
public:
	void handle_accept(const boost::system::error_code& err_code, http_session *p_session);
	void start_accept();
public:
	inline void run();
	inline void stop();
private:
	void allocate_sessions(int thread_num);
private:
	boost::asio::io_service io_service_;
	io_service_poll service_poll_;
	tcp::acceptor   acceptor_;
	std::string     host_;
	std::list<http_session *> *session_list_;
};

inline void http_server::run()
{
	service_poll_.start();
	service_poll_.join();
}

inline void http_server::stop()
{
	service_poll_.stop();
}

#endif /* HTTP_SERVER_H_ */
