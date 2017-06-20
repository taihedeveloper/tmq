/*
 * session.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: root
 */

#include "session.h"

#include <boost/format.hpp>
#include <string>
#include <algorithm>
#include <fstream>
#include <set>
#include <iostream>
using std::cout;
using std::endl;
using std::set;
using std::string;
using std::fstream;
http_session::http_session(boost::asio::io_service& io_service)
:socket_(io_service)
,data_()
,recv_times_(0)
{
}

http_session::~http_session()
{
	socket_.close();
}
void http_session::load_filter_ip()
{
    //open file
    std::ifstream fin("/home/caoge/filter_ip.conf");
    string str;
    while (getline(fin,str))
    {
        filter_ip.insert(str);
    }
    fin.close();
}
bool http_session::filter()
{
    //get ip
    string remote_ip = socket_.remote_endpoint().address().to_string();
    //compare with filter_ip
   if (!filter_ip.count(socket_.remote_endpoint().address().to_string()))
   {
       return false;
   } 
    return true;
}
void http_session::handle_read(const boost::system::error_code& err_code, std::size_t bytes_transferred)
{
	if(!err_code)
	{
		recv_times_++;
//		boost::format fmt("recvtimes:%1%\n recv_data:%2%");
//		fmt % recv_times_;
//		fmt % d
//
//		ata_;
//		std::cout << fmt.str() << std::endl;
		reply rep = reply::stock_reply(reply::ok);
        bool filter_res = filter();
        if(!filter_res)
        {
            reply::stock_reply(reply::forbidden);
            socket_.close();
            ///delete this;
        }
		boost::asio::async_write(socket_, rep.to_buffers(),
			boost::bind(&http_session::handle_write, this, boost::asio::placeholders::error));
	}
	else if (err_code != boost::asio::error::operation_aborted)
	{
		std::cout << "error_code"<< err_code << std::endl;
		socket_.close();
		delete this;
	}
}

void http_session::handle_write(const boost::system::error_code& err_code)
{
	if (!err_code)
	{
		boost::system::error_code ignored_ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		socket_.close();
	}
	else if (err_code != boost::asio::error::operation_aborted)
	{
		std::cout << "error_code"<< err_code << std::endl;
		socket_.close();
		delete this;
	}
}

reply reply::stock_reply(reply::status_type status)
{
	reply rep;
	rep.status = status;
	rep.content = stock_replies::to_string(status);
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = "text/html";
	return rep;
}

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
	std::vector<boost::asio::const_buffer> buffers;
	buffers.push_back(status_strings::to_buffer(status));
	for (std::size_t i = 0; i < headers.size(); ++i)
	{
		header& h = headers[i];
		buffers.push_back(boost::asio::buffer(h.name));
		buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
		buffers.push_back(boost::asio::buffer(h.value));
		buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	}
	buffers.push_back(boost::asio::buffer(misc_strings::crlf));
	buffers.push_back(boost::asio::buffer(content));
	return buffers;
}
