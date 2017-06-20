/*
 * httpserver.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: root
 */

#include "http_server.h"

#include <boost/lexical_cast.hpp>

http_server::http_server(const std::string& host, uint16_t port, uint16_t thread_num)
//:io_service_()
:service_poll_(thread_num)
//,acceptor_(io_service_)
//,acceptor_(service_poll_.get_io_service(), tcp::endpoint(tcp::v4(), port))
,acceptor_(service_poll_.get_io_service())
,host_(host)
,session_list_(new std::list<http_session *>())
{
	boost::asio::ip::tcp::resolver resolver(service_poll_.get_io_service());
	boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
	boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor_.bind(endpoint);
	acceptor_.listen();

	start_accept();
}

http_server::~http_server()
{
	if(session_list_ != NULL)
	{
		std::list<http_session*>::iterator iter = session_list_->begin();
		for(; iter != session_list_->end(); iter ++)
		{
			if(*iter != NULL)
			{
				delete *iter;
				*iter = NULL;
			}
		}
		session_list_->clear();
		delete session_list_;
		session_list_ = NULL;
	}
}

void http_server::allocate_sessions(int thread_num)
{
	http_session *s = NULL;
	for(int i = 0; i < thread_num; thread_num++)
	{
		s = new http_session(service_poll_.get_io_service());
		session_list_->push_back(s);
	}
}

void http_server::start_accept()
{
	http_session *p_session = new http_session(service_poll_.get_io_service());
	acceptor_.async_accept(p_session->socket(), \
			boost::bind(&http_server::handle_accept, this, \
			boost::asio::placeholders::error, p_session));
}

void http_server::handle_accept(const boost::system::error_code& err_code, http_session *p_session)
{
	if(!err_code)
	{
		p_session->start();
	}
	else
	{
		std::cout << "handle_accept error" << "error_code:"<< err_code.message() << std::endl;
		delete p_session;
	}

	start_accept();
}
